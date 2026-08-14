// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/lsp/ILspService.h"
#include "sentinel/core/lsp/LspClient.h"
#include <QObject>
#include <map>
#include <memory>

namespace sentinel::core {

class LspService : public QObject, public ILspService {
    Q_OBJECT
public:
    explicit LspService(QObject* parent = nullptr);
    ~LspService() override;

    // ILspService interface
    bool addServer(const LspServerConfig& config) override;
    bool removeServer(const QString& serverName) override;
    QList<LspServerConfig> servers() const override;

    QList<LspLocation> goToDefinition(const QString& filePath, int line, int character) override;
    QList<LspLocation> findReferences(const QString& filePath, int line, int character) override;
    QString hover(const QString& filePath, int line, int character) override;
    QList<LspSymbol> documentSymbol(const QString& filePath) override;
    QList<LspSymbol> workspaceSymbol(const QString& query) override;

    QList<LspDiagnostic> diagnostics(const QString& filePath) override;

    void openFile(const QString& filePath, const QString& content) override;
    void closeFile(const QString& filePath) override;
    void updateFile(const QString& filePath, const QString& content) override;

signals:
    void serverStarted(const QString& serverName);
    void serverStopped(const QString& serverName);
    void serverError(const QString& serverName, const QString& error);
    void diagnosticsUpdated(const QString& filePath, const QList<LspDiagnostic>& diagnostics);

private:
    LspClient* findClientForFile(const QString& filePath) const;
    LspClient* findClient(const QString& serverName) const;
    QString fileExtension(const QString& filePath) const;

    QList<LspServerConfig> m_serverList;
    std::map<QString, std::unique_ptr<LspClient>> m_clients;
    std::map<QString, QString> m_fileToServer; // file extension -> server name
    QMap<QString, QList<LspDiagnostic>> m_diagnostics;
};

} // namespace sentinel::core
