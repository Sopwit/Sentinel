// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/notification/ExternalEditorService.h"
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QTemporaryFile>
#include <QTextStream>

namespace sentinel::core {

ExternalEditorService::ExternalEditorService(QObject* parent) : QObject(parent) {}
ExternalEditorService::~ExternalEditorService() = default;

QString ExternalEditorService::detectEditor() const {
    QString editor = qgetenv("VISUAL");
    if (editor.isEmpty())
        editor = qgetenv("EDITOR");
    if (editor.isEmpty())
        editor = "vi";
    return editor;
}

QString ExternalEditorService::editContent(const QString& content) const {
    QString editor = m_configuredEditor.isEmpty() ? detectEditor() : m_configuredEditor;

    QTemporaryFile tempFile(QDir::tempPath() + "/sentinel_edit_XXXXXX.md");
    tempFile.setAutoRemove(false);

    if (tempFile.open()) {
        QTextStream stream(&tempFile);
        stream << content;
        tempFile.close();

        QProcess process;
        process.start("sh", {"-c", QStringLiteral("%1 \"%2\"").arg(editor, tempFile.fileName())});
        process.waitForFinished(-1);

        QFile result(tempFile.fileName());
        QString output;
        if (result.open(QIODevice::ReadOnly | QIODevice::Text)) {
            output = QTextStream(&result).readAll();
        }

        QFile::remove(tempFile.fileName());
        return output;
    }
    return content;
}

void ExternalEditorService::setEditor(const QString& editor) {
    m_configuredEditor = editor;
}
QString ExternalEditorService::configuredEditor() const {
    return m_configuredEditor;
}
bool ExternalEditorService::isAvailable() const {
    return true;
}

} // namespace sentinel::core
