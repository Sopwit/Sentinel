// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/session/IUsageStatsService.h"
#include <QObject>

namespace sentinel::core {

class UsageStatsService : public QObject, public IUsageStatsService {
    Q_OBJECT
public:
    explicit UsageStatsService(QObject* parent = nullptr);
    ~UsageStatsService() override;

    void recordMessage(const QString& model, qint64 tokens, double cost = 0.0) override;
    void recordToolUse(const QString& toolName) override;
    void recordSession() override;
    UsageStats stats(int days = 0) const override;
    QJsonObject statsJson(int days = 0) const override;
    QString formattedStats(int days = 0) const override;
    void reset() override;

private:
    UsageStats m_stats;
};

} // namespace sentinel::core
