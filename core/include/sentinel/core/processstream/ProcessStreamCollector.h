// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QByteArray>
#include <QString>
#include <functional>

namespace sentinel::core {

class ProcessStreamCollector {
public:
    using LineCallback = std::function<void(const QString& line)>;

    void collectStdout(const QByteArray& data);
    void collectStderr(const QByteArray& data);
    QString stdoutOutput() const;
    QString stderrOutput() const;
    void setLineCallback(LineCallback callback);
    void clear();

private:
    QString m_stdout;
    QString m_stderr;
    LineCallback m_callback;
};

} // namespace sentinel::core
