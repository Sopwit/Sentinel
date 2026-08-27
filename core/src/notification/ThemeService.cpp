// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/notification/ThemeService.h"

namespace sentinel::core {

ThemeService::ThemeService(QObject* parent) : QObject(parent) {
    Theme dark;
    dark.name = "dark";
    dark.displayName = "Dark";
    dark.isDark = true;
    dark.colors = {"#1e1e2e", "#cdd6f4", "#89b4fa", "#74c7ec", "#f38ba8",
                   "#f38ba8", "#a6e3a1", "#f9e2af", "#585b70", "#313244"};
    m_themes.append(dark);

    Theme light;
    light.name = "light";
    light.displayName = "Light";
    light.isDark = false;
    light.colors = {"#eff1f5", "#4c4f69", "#1e66f5", "#209fb5", "#d20f39",
                    "#d20f39", "#40a02b", "#df8e1d", "#9ca0b0", "#ccd0da"};
    m_themes.append(light);

    m_currentTheme = "dark";
}

ThemeService::~ThemeService() = default;

void ThemeService::registerTheme(const Theme& theme) {
    for (int i = 0; i < m_themes.size(); ++i) {
        if (m_themes[i].name == theme.name) {
            m_themes[i] = theme;
            return;
        }
    }
    m_themes.append(theme);
}

Theme ThemeService::currentTheme() const {
    return themeByName(m_currentTheme);
}

void ThemeService::setCurrentTheme(const QString& name) {
    m_currentTheme = name;
    emit themeChanged(name);
}

QList<Theme> ThemeService::themes() const {
    return m_themes;
}

Theme ThemeService::themeByName(const QString& name) const {
    for (const auto& theme : m_themes) {
        if (theme.name == name)
            return theme;
    }
    return m_themes.isEmpty() ? Theme{} : m_themes.first();
}

void ThemeService::setDarkMode(bool dark) {
    m_darkMode = dark;
    setCurrentTheme(dark ? "dark" : "light");
}

bool ThemeService::isDarkMode() const {
    return m_darkMode;
}

} // namespace sentinel::core
