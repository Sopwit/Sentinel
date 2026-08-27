// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJsonObject>
#include <QMap>
#include <QString>

namespace sentinel::core {

struct CodeModeScript {
    QString script;
    QMap<QString, QString> toolCatalog;
    QString namespace_;
    int maxToolCalls{32};
    int maxScriptLength{64 * 1024};
};

struct CodeModeResult {
    bool success{false};
    QString output;
    QString error;
    QList<QJsonObject> toolCalls;
};

class CodeModeInterpreter {
public:
    CodeModeResult execute(const CodeModeScript& script) const;
    QMap<QString, QString> buildToolCatalog(const QStringList& tools) const;
    bool validateScript(const QString& script, QString& error) const;

private:
    QString resolveToolCall(const QString& toolCall) const;
};

} // namespace sentinel::core
