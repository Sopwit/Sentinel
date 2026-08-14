// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QJsonObject>
#include <QMap>
#include <memory>

namespace sentinel::core {

struct LspServerConfig {
    QString name;
    QString command;
    QStringList arguments;
    QStringList fileExtensions; // e.g., {".cpp", ".h", ".hpp"}
    bool enabled{true};
};

struct LspLocation {
    QString uri;
    int line{0};
    int character{0};
};

struct LspSymbol {
    QString name;
    QString kind;
    QString containerName;
    int line{0};
    int character{0};
};

struct LspDiagnostic {
    int line{0};
    int character{0};
    int severity{0}; // 1=Error, 2=Warning, 3=Info, 4=Hint
    QString message;
    QString source;
};

enum class LspOperation {
    GoToDefinition,
    FindReferences,
    Hover,
    DocumentSymbol,
    WorkspaceSymbol,
    GoToImplementation,
    PrepareCallHierarchy,
    IncomingCalls,
    OutgoingCalls
};

class ILspService {
public:
    virtual ~ILspService() = default;

    // Server management
    virtual bool addServer(const LspServerConfig& config) = 0;
    virtual bool removeServer(const QString& serverName) = 0;
    virtual QList<LspServerConfig> servers() const = 0;

    // Operations
    virtual QList<LspLocation> goToDefinition(const QString& filePath, int line, int character) = 0;
    virtual QList<LspLocation> findReferences(const QString& filePath, int line, int character) = 0;
    virtual QString hover(const QString& filePath, int line, int character) = 0;
    virtual QList<LspSymbol> documentSymbol(const QString& filePath) = 0;
    virtual QList<LspSymbol> workspaceSymbol(const QString& query) = 0;

    // Diagnostics
    virtual QList<LspDiagnostic> diagnostics(const QString& filePath) = 0;

    // File management
    virtual void openFile(const QString& filePath, const QString& content) = 0;
    virtual void closeFile(const QString& filePath) = 0;
    virtual void updateFile(const QString& filePath, const QString& content) = 0;
};

} // namespace sentinel::core
