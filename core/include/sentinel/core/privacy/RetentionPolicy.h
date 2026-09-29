// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "sentinel/core/app/AppSettings.h"
#include "sentinel/core/chat/IConversationStore.h"
#include "sentinel/core/chat/IChatHistoryStore.h"
#include "sentinel/core/agent/IAgentRunStore.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDateTime>
#include <QStringList>

namespace sentinel::core {

// Only classes with an owning cleanup path appear as configurable policies.
class RetentionPolicy final {
public:
    static QJsonObject defaults() {
        return {{QStringLiteral("version"), 1},
                {QStringLiteral("chat"), QStringLiteral("Keep")},
                {QStringLiteral("agentRuns"), QStringLiteral("Keep")},
                {QStringLiteral("diagnostics"), QStringLiteral("30d")},
                {QStringLiteral("modelSourceCache"), QStringLiteral("30d")},
                {QStringLiteral("modelOperations"), QStringLiteral("30d")},
                {QStringLiteral("temporaryAudio"), QStringLiteral("AfterUse")},
                {QStringLiteral("generatedSpeech"), QStringLiteral("AfterUse")}};
    }

    static bool valid(const QJsonObject& policy) {
        if (policy.value(QStringLiteral("version")).toInt(-1) != 1 || policy.size() != 8)
            return false;
        const QStringList durations{QStringLiteral("Keep"), QStringLiteral("1d"),
            QStringLiteral("7d"), QStringLiteral("30d"), QStringLiteral("90d")};
        for (const auto& key : {QStringLiteral("chat"), QStringLiteral("agentRuns"),
                                QStringLiteral("diagnostics"),
                                QStringLiteral("modelSourceCache"),
                                QStringLiteral("modelOperations")})
            if (!durations.contains(policy.value(key).toString())) return false;
        return policy.value(QStringLiteral("temporaryAudio")).toString() == QLatin1String("AfterUse") &&
               policy.value(QStringLiteral("generatedSpeech")).toString() == QLatin1String("AfterUse");
    }

    static QJsonObject effective(const AppSettings& settings) {
        if (settings.retentionPolicyJson().isEmpty()) return defaults();
        const auto document = QJsonDocument::fromJson(settings.retentionPolicyJson().toUtf8());
        return document.isObject() && valid(document.object()) ? document.object() : QJsonObject{};
    }

    static bool set(AppSettings& settings, const QString& domain, const QString& value) {
        auto policy = effective(settings);
        if (policy.isEmpty() || !policy.contains(domain)) return false;
        policy.insert(domain, value);
        if (!valid(policy)) return false;
        settings.setRetentionPolicyJson(QString::fromUtf8(
            QJsonDocument(policy).toJson(QJsonDocument::Compact)));
        return settings.storageErrorCode().isEmpty();
    }

    static int days(const QString& value) {
        if (value == QLatin1String("1d")) return 1;
        if (value == QLatin1String("7d")) return 7;
        if (value == QLatin1String("30d")) return 30;
        if (value == QLatin1String("90d")) return 90;
        return 0;
    }

    static QJsonObject maintain(const AppSettings& settings, IConversationStore* chat,
                                IChatHistoryStore* legacyChat, IAgentRunStore* runs) {
        const auto policy = effective(settings);
        QJsonObject result{{QStringLiteral("attempted"), 0},
                           {QStringLiteral("removed"), 0},
                           {QStringLiteral("partialFailure"), false}};
        if (policy.isEmpty()) {
            result.insert(QStringLiteral("error"), QStringLiteral("UnsupportedRetentionPolicy"));
            result.insert(QStringLiteral("partialFailure"), true);
            return result;
        }
        int attempted = 0;
        int removed = 0;
        QJsonArray failures;
        const int chatDays = days(policy.value(QStringLiteral("chat")).toString());
        if (chatDays) {
            if (!chat && !legacyChat) failures.append(QStringLiteral("chat"));
            if (chat) {
                ++attempted;
                const int count = chat->pruneCompletedBefore(
                    QDateTime::currentDateTimeUtc().addDays(-chatDays));
                if (count < 0) failures.append(QStringLiteral("chat")); else removed += count;
            }
            if (legacyChat) {
                ++attempted;
                const int legacyCount = legacyChat->pruneCompletedBefore(
                    QDateTime::currentDateTimeUtc().addDays(-chatDays));
                if (legacyCount < 0) failures.append(QStringLiteral("chatHistory"));
                else removed += legacyCount;
            }
        }
        const int runDays = days(policy.value(QStringLiteral("agentRuns")).toString());
        if (runDays) {
            ++attempted;
            const int count = runs ? runs->pruneCompletedBefore(
                QDateTime::currentDateTimeUtc().addDays(-runDays)) : -1;
            if (count < 0) failures.append(QStringLiteral("agentRuns")); else removed += count;
        }
        result.insert(QStringLiteral("attempted"), attempted);
        result.insert(QStringLiteral("removed"), removed);
        result.insert(QStringLiteral("failures"), failures);
        result.insert(QStringLiteral("partialFailure"), !failures.isEmpty());
        return result;
    }
};
} // namespace sentinel::core
