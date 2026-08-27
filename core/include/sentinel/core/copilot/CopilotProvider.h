// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJsonObject>
#include <QString>

namespace sentinel::core {

struct CopilotConfig {
    QString apiKey;
    QString baseUrl{"https://api.githubcopilot.com"};
    QString model{"gpt-4o"};
    bool enabled{false};
};

class CopilotProvider {
public:
    CopilotProvider();

    void configure(const CopilotConfig& config);
    bool isConfigured() const;
    QString providerName() const;
    QString completeEndpoint() const;
    bool hasValidApiKey() const;

private:
    CopilotConfig m_config;
};

} // namespace sentinel::core
