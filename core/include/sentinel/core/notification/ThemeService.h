// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/notification/IThemeService.h"
#include <QObject>
#include <QList>

namespace sentinel::core {

class ThemeService : public QObject, public IThemeService {
    Q_OBJECT
public:
    explicit ThemeService(QObject* parent = nullptr);
    ~ThemeService() override;

    void registerTheme(const Theme& theme) override;
    Theme currentTheme() const override;
    void setCurrentTheme(const QString& name) override;
    QList<Theme> themes() const override;
    Theme themeByName(const QString& name) const override;
    void setDarkMode(bool dark) override;
    bool isDarkMode() const override;

signals:
    void themeChanged(const QString& themeName);

private:
    QList<Theme> m_themes;
    QString m_currentTheme;
    bool m_darkMode{true};
};

} // namespace sentinel::core
