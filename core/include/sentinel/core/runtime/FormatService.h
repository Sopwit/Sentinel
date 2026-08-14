// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/runtime/IFormatService.h"
#include <QObject>
#include <QProcess>

namespace sentinel::core {

class FormatService : public QObject, public IFormatService {
    Q_OBJECT
public:
    explicit FormatService(QObject* parent = nullptr);
    ~FormatService() override;

    void configure(const FormatterConfig& config) override;
    FormatterConfig config() const override;
    QString detectFormatter(const QString& filePath) const override;
    bool formatFile(const QString& filePath) override;
    bool isAvailable() const override;

private:
    FormatterConfig m_config;
};

} // namespace sentinel::core
