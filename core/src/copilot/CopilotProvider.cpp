// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/copilot/CopilotProvider.h"

namespace sentinel::core {

CopilotProvider::CopilotProvider() = default;

void CopilotProvider::configure(const CopilotConfig& config) { m_config = config; }
bool CopilotProvider::isConfigured() const { return m_config.enabled && !m_config.apiKey.isEmpty(); }
QString CopilotProvider::providerName() const { return "github-copilot"; }
QString CopilotProvider::completeEndpoint() const { return m_config.baseUrl + "/v1/chat/completions"; }
bool CopilotProvider::hasValidApiKey() const { return !m_config.apiKey.isEmpty(); }

} // namespace sentinel::core
