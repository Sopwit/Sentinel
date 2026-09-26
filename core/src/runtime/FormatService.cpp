// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/runtime/FormatService.h"
#include <QFileInfo>

namespace sentinel::core {

FormatService::FormatService(QObject* parent) : QObject(parent) {}
FormatService::~FormatService() = default;

void FormatService::configure(const FormatterConfig& config) {
    m_config = config;
}

FormatterConfig FormatService::config() const {
    return m_config;
}

QString FormatService::detectFormatter(const QString& filePath) const {
    if (!m_config.enabled)
        return {};

    QFileInfo info(filePath);
    QString ext = info.suffix();
    if (m_config.formattersByExtension.contains(ext)) {
        return m_config.formattersByExtension[ext];
    }
    return m_config.defaultFormatter;
}

bool FormatService::formatFile(const QString& filePath) {
    Q_UNUSED(filePath);
    return false;
}

bool FormatService::isAvailable() const {
    return false;
}

} // namespace sentinel::core
