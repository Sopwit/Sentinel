// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/processstream/ProcessStreamCollector.h"

namespace sentinel::core {

void ProcessStreamCollector::collectStdout(const QByteArray& data) {
    QString text = QString::fromUtf8(data);
    m_stdout += text;

    if (m_callback) {
        QStringList lines = text.split('\n', Qt::SkipEmptyParts);
        for (const auto& line : lines) {
            m_callback(line);
        }
    }
}

void ProcessStreamCollector::collectStderr(const QByteArray& data) {
    m_stderr += QString::fromUtf8(data);
}

QString ProcessStreamCollector::stdoutOutput() const { return m_stdout; }
QString ProcessStreamCollector::stderrOutput() const { return m_stderr; }
void ProcessStreamCollector::setLineCallback(LineCallback callback) { m_callback = callback; }
void ProcessStreamCollector::clear() { m_stdout.clear(); m_stderr.clear(); }

} // namespace sentinel::core
