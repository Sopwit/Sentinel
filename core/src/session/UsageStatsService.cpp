// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/session/UsageStatsService.h"
#include <QJsonDocument>
#include <QJsonObject>

namespace sentinel::core {

UsageStatsService::UsageStatsService(QObject* parent) : QObject(parent) {
    m_stats.dateRangeStart = QDateTime::currentDateTime();
}

UsageStatsService::~UsageStatsService() = default;

void UsageStatsService::recordMessage(const QString& model, qint64 tokens, double cost) {
    m_stats.totalMessages++;
    m_stats.totalTokens += tokens;
    m_stats.totalCost += cost;
    m_stats.modelTokens[model] += tokens;
    m_stats.dateRangeEnd = QDateTime::currentDateTime();
}

void UsageStatsService::recordToolUse(const QString& toolName) {
    m_stats.toolUsageCounts[toolName]++;
}

void UsageStatsService::recordSession() {
    m_stats.totalSessions++;
}

UsageStats UsageStatsService::stats(int days) const {
    Q_UNUSED(days)
    return m_stats;
}

QJsonObject UsageStatsService::statsJson(int days) const {
    auto s = stats(days);
    QJsonObject obj;
    obj["totalSessions"] = s.totalSessions;
    obj["totalMessages"] = s.totalMessages;
    obj["totalTokens"] = s.totalTokens;
    obj["totalCost"] = s.totalCost;
    return obj;
}

QString UsageStatsService::formattedStats(int days) const {
    auto s = stats(days);
    return QStringLiteral("Sessions: %1 | Messages: %2 | Tokens: %3 | Cost: $%4")
        .arg(s.totalSessions).arg(s.totalMessages).arg(s.totalTokens).arg(s.totalCost, 0, 'f', 2);
}

void UsageStatsService::reset() {
    m_stats = UsageStats{};
    m_stats.dateRangeStart = QDateTime::currentDateTime();
}

} // namespace sentinel::core
