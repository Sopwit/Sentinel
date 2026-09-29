// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "sentinel/core/agent/IAgentRunStore.h"
#include "sentinel/core/chat/IChatHistoryStore.h"
#include "sentinel/core/chat/IConversationStore.h"

#include <QJsonObject>
#include <QDateTime>
#include <QStringList>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>

namespace sentinel::core {

// Typed recovery status plus read-only projections from owning stores. Inspection never
// restarts a provider, tool, task, download, or speech operation.
class RecoveryService final {
public:
    static QString statusPath() {
        return QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
            .filePath(QStringLiteral("recovery-status.json"));
    }

    // Codes and action IDs are product identifiers. Never persist raw exception text.
    static bool recordCondition(const QString& category, const QString& domain,
                                const QString& code, const QString& action,
                                const QString& severity = QStringLiteral("RecoveryRequired")) {
        const QRegularExpression id(QStringLiteral("^[A-Za-z0-9._-]{1,80}$"));
        if (!id.match(category).hasMatch() || !id.match(domain).hasMatch() ||
            !id.match(code).hasMatch() || !id.match(action).hasMatch() ||
            !QStringList{QStringLiteral("Recoverable"), QStringLiteral("Degraded"),
                         QStringLiteral("RecoveryRequired")}.contains(severity)) return false;
        auto document = readStatus();
        if (document.value(QStringLiteral("version")).toInt(-1) != 1) return false;
        auto conditions = document.value(QStringLiteral("conditions")).toArray();
        QJsonArray next;
        for (const auto& value : conditions) {
            const auto item = value.toObject();
            if (item.value(QStringLiteral("category")) != category ||
                item.value(QStringLiteral("domain")) != domain) next.append(item);
        }
        if (next.size() >= 32) return false;
        next.append(QJsonObject{{QStringLiteral("category"), category},
            {QStringLiteral("domain"), domain}, {QStringLiteral("code"), code},
            {QStringLiteral("action"), action}, {QStringLiteral("severity"), severity},
            {QStringLiteral("timestampUtc"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)}});
        document.insert(QStringLiteral("conditions"), next);
        return writeStatus(document);
    }

    static bool clearCondition(const QString& category, const QString& domain) {
        auto document = readStatus();
        if (document.value(QStringLiteral("version")).toInt(-1) != 1) return false;
        QJsonArray next;
        for (const auto& value : document.value(QStringLiteral("conditions")).toArray()) {
            const auto item = value.toObject();
            if (item.value(QStringLiteral("category")) != category ||
                item.value(QStringLiteral("domain")) != domain) next.append(item);
        }
        document.insert(QStringLiteral("conditions"), next);
        return writeStatus(document);
    }

    static QString captureDirectory() {
        return QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
            .filePath(QStringLiteral("sentinel-capture"));
    }

    // Only stale private capture files with Sentinel's exact managed naming pattern.
    static QJsonObject cleanupTemporaryAudioDetailed() {
        const QFileInfo root(captureDirectory());
        if (!root.exists()) return {{QStringLiteral("removed"), 0}, {QStringLiteral("failed"), 0}};
        if (!root.isDir() || root.isSymLink())
            return {{QStringLiteral("removed"), 0}, {QStringLiteral("failed"), 1}};
        const QDir directory(root.absoluteFilePath());
        const QRegularExpression name(QStringLiteral("^sentinel-capture-[A-Za-z0-9]{6}\\.wav$"));
        int removed = 0;
        int failed = 0;
        for (const auto& file : directory.entryInfoList(QDir::Files | QDir::NoSymLinks)) {
            if (!name.match(file.fileName()).hasMatch() || file.isSymLink() ||
                file.lastModified().secsTo(QDateTime::currentDateTime()) < 24 * 60 * 60)
                continue;
            if (QFile::remove(file.absoluteFilePath())) ++removed; else ++failed;
        }
        return {{QStringLiteral("removed"), removed}, {QStringLiteral("failed"), failed}};
    }

    static QJsonObject cleanupGeneratedSpeechDetailed() {
        const auto rootPath = QDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation))
                                  .filePath(QStringLiteral("piper-tts"));
        const QFileInfo root(rootPath);
        if (!root.exists()) return {{QStringLiteral("removed"), 0}, {QStringLiteral("failed"), 0}};
        if (!root.isDir() || root.isSymLink())
            return {{QStringLiteral("removed"), 0}, {QStringLiteral("failed"), 1}};
        const QFileInfo output(QDir(rootPath).filePath(QStringLiteral("sentinel-piper-tts.wav")));
        if (!output.exists() || !output.isFile() || output.isSymLink() ||
            output.lastModified().secsTo(QDateTime::currentDateTime()) < 24 * 60 * 60)
            return {{QStringLiteral("removed"), 0}, {QStringLiteral("failed"), 0}};
        return QFile::remove(output.absoluteFilePath())
            ? QJsonObject{{QStringLiteral("removed"), 1}, {QStringLiteral("failed"), 0}}
            : QJsonObject{{QStringLiteral("removed"), 0}, {QStringLiteral("failed"), 1}};
    }

    RecoveryService(const IConversationStore* conversations = nullptr,
                    const IChatHistoryStore* chatHistory = nullptr,
                    const IAgentRunStore* agentRuns = nullptr)
        : conversations_(conversations), chatHistory_(chatHistory), agentRuns_(agentRuns) {}

    QJsonObject state() const {
        const auto persisted = readStatus();
        const auto conditions = persisted.value(QStringLiteral("conditions")).toArray();
        QString health = QStringLiteral("Healthy");
        if (persisted.value(QStringLiteral("version")).toInt(-1) != 1)
            health = QStringLiteral("RecoveryRequired");
        for (const auto& value : conditions) {
            const auto severity = value.toObject().value(QStringLiteral("severity")).toString();
            if (severity == QLatin1String("RecoveryRequired")) health = severity;
            else if (health != QLatin1String("RecoveryRequired") && severity == QLatin1String("Degraded"))
                health = severity;
            else if (health == QLatin1String("Healthy")) health = QStringLiteral("Recoverable");
        }
        int interruptedChat = 0;
        bool chatSampleTruncated = false;
        if (conversations_ && conversations_->status() == ConversationStoreStatus::Ready) {
            const auto conversations = conversations_->listConversations();
            chatSampleTruncated = conversations.size() > 100;
            int inspected = 0;
            for (const auto& conversation : conversations) {
                if (++inspected > 100) break;
                for (const auto& message : conversations_->loadMessages(conversation.id))
                    if (message.status == ChatMessageStatus::Interrupted) ++interruptedChat;
            }
        }
        if (chatHistory_ && chatHistory_->isAvailable())
            for (const auto& message : chatHistory_->recentMessages(1000))
                if (message.status == ChatMessageStatus::Interrupted) ++interruptedChat;
        int interruptedRuns = 0;
        if (agentRuns_)
            for (const auto& run : agentRuns_->recentRuns(1000))
                if (run.state == QLatin1String("Interrupted")) ++interruptedRuns;
        if (health == QLatin1String("Healthy") && (interruptedChat || interruptedRuns))
            health = QStringLiteral("Recoverable");
        return {{QStringLiteral("health"), health},
                {QStringLiteral("conditions"), conditions},
                {QStringLiteral("statusVersionSupported"),
                 persisted.value(QStringLiteral("version")).toInt(-1) == 1},
                {QStringLiteral("interruptedChatMessages"), interruptedChat},
                {QStringLiteral("interruptedAgentRuns"), interruptedRuns},
                {QStringLiteral("chatSampleTruncated"), chatSampleTruncated},
                {QStringLiteral("chatInspectable"), conversations_ || chatHistory_},
                {QStringLiteral("agentRunsInspectable"), agentRuns_ != nullptr},
                {QStringLiteral("automaticSideEffectReplay"), false},
                {QStringLiteral("modelOperations"), QStringLiteral("InterruptedJournalNoReplay")},
                {QStringLiteral("speechOperations"), QStringLiteral("EphemeralNoReplay")}};
    }

private:
    static QJsonObject readStatus() {
        QFile file(statusPath());
        if (!file.exists()) return {{QStringLiteral("version"), 1},
                                    {QStringLiteral("conditions"), QJsonArray{}}};
        if (!file.open(QIODevice::ReadOnly) || file.size() > 32768)
            return {{QStringLiteral("version"), -1}};
        const auto document = QJsonDocument::fromJson(file.readAll());
        if (!document.isObject() || !document.object().value(QStringLiteral("conditions")).isArray())
            return {{QStringLiteral("version"), -1}};
        const auto conditions = document.object().value(QStringLiteral("conditions")).toArray();
        if (conditions.size() > 32) return {{QStringLiteral("version"), -1}};
        const QRegularExpression id(QStringLiteral("^[A-Za-z0-9._-]{1,80}$"));
        for (const auto& value : conditions) {
            const auto item = value.toObject();
            for (const auto& key : {QStringLiteral("category"), QStringLiteral("domain"),
                                    QStringLiteral("code"), QStringLiteral("action")})
                if (!id.match(item.value(key).toString()).hasMatch())
                    return {{QStringLiteral("version"), -1}};
            if (!QStringList{QStringLiteral("Recoverable"), QStringLiteral("Degraded"),
                             QStringLiteral("RecoveryRequired")}.contains(
                    item.value(QStringLiteral("severity")).toString()) ||
                !QDateTime::fromString(item.value(QStringLiteral("timestampUtc")).toString(),
                                       Qt::ISODateWithMs).isValid())
                return {{QStringLiteral("version"), -1}};
        }
        return document.object();
    }

    static bool writeStatus(const QJsonObject& document) {
        const auto path = statusPath();
        if (!QDir().mkpath(QFileInfo(path).absolutePath())) return false;
        const auto bytes = QJsonDocument(document).toJson(QJsonDocument::Compact);
        if (bytes.size() > 32768) return false;
        QSaveFile file(path);
        return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size() && file.commit();
    }

    const IConversationStore* conversations_;
    const IChatHistoryStore* chatHistory_;
    const IAgentRunStore* agentRuns_;
};

} // namespace sentinel::core
