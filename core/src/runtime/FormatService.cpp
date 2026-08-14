// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/runtime/FormatService.h"
#include <QFileInfo>
#include <QProcess>

namespace sentinel::core {

FormatService::FormatService(QObject* parent) : QObject(parent) {}
FormatService::~FormatService() = default;

void FormatService::configure(const FormatterConfig& config) {
    m_config = config;
}

FormatterConfig FormatService::config() const { return m_config; }

QString FormatService::detectFormatter(const QString& filePath) const {
    if (!m_config.enabled) return {};

    QFileInfo info(filePath);
    QString ext = info.suffix();
    if (m_config.formattersByExtension.contains(ext)) {
        return m_config.formattersByExtension[ext];
    }
    return m_config.defaultFormatter;
}

bool FormatService::formatFile(const QString& filePath) {
    QString formatter = detectFormatter(filePath);
    if (formatter.isEmpty()) return false;

    QProcess process;
    process.start("sh", {"-c", QStringLiteral("%1 \"%2\"").arg(formatter, filePath)});
    process.waitForFinished(10000);
    return process.exitCode() == 0;
}

bool FormatService::isAvailable() const {
    return m_config.enabled;
}

} // namespace sentinel::core
