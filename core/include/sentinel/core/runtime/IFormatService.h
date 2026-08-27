// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QMap>
#include <QString>

namespace sentinel::core {

struct FormatterConfig {
    bool enabled{false};
    QMap<QString, QString> formattersByExtension;
    QString defaultFormatter;
};

class IFormatService {
public:
    virtual ~IFormatService() = default;

    virtual void configure(const FormatterConfig& config) = 0;
    virtual FormatterConfig config() const = 0;
    virtual QString detectFormatter(const QString& filePath) const = 0;
    virtual bool formatFile(const QString& filePath) = 0;
    virtual bool isAvailable() const = 0;
};

} // namespace sentinel::core
