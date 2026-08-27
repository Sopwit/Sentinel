// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJsonObject>
#include <QMap>
#include <QString>

namespace sentinel::core {

struct ThemeColors {
    QString background;
    QString foreground;
    QString primary;
    QString secondary;
    QString accent;
    QString error;
    QString warning;
    QString success;
    QString border;
    QString surface;
};

struct Theme {
    QString name;
    QString displayName;
    bool isDark{true};
    ThemeColors colors;
};

class IThemeService {
public:
    virtual ~IThemeService() = default;

    virtual void registerTheme(const Theme& theme) = 0;
    virtual Theme currentTheme() const = 0;
    virtual void setCurrentTheme(const QString& name) = 0;
    virtual QList<Theme> themes() const = 0;
    virtual Theme themeByName(const QString& name) const = 0;
    virtual void setDarkMode(bool dark) = 0;
    virtual bool isDarkMode() const = 0;
};

} // namespace sentinel::core
