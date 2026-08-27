// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/codemode/CodeModeInterpreter.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

namespace sentinel::core {

CodeModeResult CodeModeInterpreter::execute(const CodeModeScript& script) const {
    CodeModeResult result;
    QString validationError;
    if (!validateScript(script.script, validationError)) {
        result.error = validationError;
        return result;
    }

    QString cleanedScript = script.script;
    cleanedScript.remove(QRegularExpression("^\\s*//.*$", QRegularExpression::MultilineOption));

    QRegularExpression toolCallRx(
        "([a-zA-Z_][a-zA-Z0-9_]*)\\.([a-zA-Z_][a-zA-Z0-9_]*)\\(([^)]*)\\)");
    QRegularExpressionMatchIterator it = toolCallRx.globalMatch(cleanedScript);

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QJsonObject toolCall;
        toolCall["tool"] = match.captured(1);
        toolCall["method"] = match.captured(2);
        toolCall["args"] = match.captured(3);
        if (!script.toolCatalog.isEmpty() && !script.toolCatalog.contains(match.captured(1))) {
            result.error =
                QStringLiteral("CodeMode tool is not present in the approved catalog: %1")
                    .arg(match.captured(1));
            result.toolCalls.clear();
            return result;
        }
        result.toolCalls.append(toolCall);
        if (result.toolCalls.size() >= qMax(1, script.maxToolCalls)) {
            result.success = true;
            result.output =
                QStringLiteral("CodeMode call limit reached; execution stopped safely.");
            return result;
        }
    }

    result.success = true;
    return result;
}

QMap<QString, QString> CodeModeInterpreter::buildToolCatalog(const QStringList& tools) const {
    QMap<QString, QString> catalog;
    for (const auto& tool : tools) {
        catalog[tool] = tool;
    }
    return catalog;
}

bool CodeModeInterpreter::validateScript(const QString& script, QString& error) const {
    error.clear();
    if (script.size() > 64 * 1024) {
        error = QStringLiteral("CodeMode script exceeds the 64 KiB limit.");
        return false;
    }
    static const QStringList forbidden{
        QStringLiteral("#include"), QStringLiteral("import "),    QStringLiteral("require("),
        QStringLiteral("exec("),    QStringLiteral("eval("),      QStringLiteral("system("),
        QStringLiteral("process"),  QStringLiteral("filesystem"), QStringLiteral("while"),
        QStringLiteral("for("),     QStringLiteral("for ("),      QStringLiteral("function "),
    };
    const QString lower = script.toLower();
    for (const QString& token : forbidden) {
        if (lower.contains(token)) {
            error = QStringLiteral("CodeMode construct is not allowed: %1").arg(token);
            return false;
        }
    }
    const int open = script.count(QLatin1Char('('));
    const int close = script.count(QLatin1Char(')'));
    if (open != close) {
        error = QStringLiteral("CodeMode script contains unbalanced parentheses.");
        return false;
    }
    return true;
}

QString CodeModeInterpreter::resolveToolCall(const QString& toolCall) const {
    return toolCall;
}

} // namespace sentinel::core
