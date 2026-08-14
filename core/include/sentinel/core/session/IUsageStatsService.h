// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QJsonObject>
#include <QDateTime>
#include <QMap>

namespace sentinel::core {

struct UsageStats {
    int totalSessions{0};
    int totalMessages{0};
    qint64 totalTokens{0};
    double totalCost{0.0};
    QMap<QString, int> toolUsageCounts;
    QMap<QString, qint64> modelTokens;
    QDateTime dateRangeStart;
    QDateTime dateRangeEnd;
};

class IUsageStatsService {
public:
    virtual ~IUsageStatsService() = default;

    virtual void recordMessage(const QString& model, qint64 tokens, double cost = 0.0) = 0;
    virtual void recordToolUse(const QString& toolName) = 0;
    virtual void recordSession() = 0;
    virtual UsageStats stats(int days = 0) const = 0;
    virtual QJsonObject statsJson(int days = 0) const = 0;
    virtual QString formattedStats(int days = 0) const = 0;
    virtual void reset() = 0;
};

} // namespace sentinel::core
