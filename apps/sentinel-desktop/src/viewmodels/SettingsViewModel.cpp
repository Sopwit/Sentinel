// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <sentinel/desktop/viewmodels/SettingsViewModel.h>

namespace sentinel::desktop::viewmodels {

SettingsViewModel::SettingsViewModel(QObject* parent)
    : QObject(parent) {
}

void SettingsViewModel::setOllamaEndpoint(const QString& endpoint) {
    if (m_ollamaEndpoint != endpoint) {
        m_ollamaEndpoint = endpoint;
        Q_EMIT ollamaEndpointChanged();
    }
}

void SettingsViewModel::setWorkspacePath(const QString& path) {
    if (m_workspacePath != path) {
        m_workspacePath = path;
        Q_EMIT workspacePathChanged();
    }
}

void SettingsViewModel::setActiveTheme(const QString& theme) {
    if (m_activeTheme != theme) {
        m_activeTheme = theme;
        Q_EMIT activeThemeChanged();
    }
}

} // namespace sentinel::desktop::viewmodels
