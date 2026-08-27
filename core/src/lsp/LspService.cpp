// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/lsp/LspService.h"
#include <QDebug>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QUrl>

namespace sentinel::core {

LspService::LspService(QObject* parent) : QObject(parent) {}

LspService::~LspService() {
    for (auto& [name, client] : m_clients) {
        if (client) {
            client->stop();
        }
    }
}

bool LspService::addServer(const LspServerConfig& config) {
    for (const auto& existing : m_serverList) {
        if (existing.name == config.name) {
            return false;
        }
    }

    m_serverList.append(config);

    // Map file extensions to server
    for (const QString& ext : config.fileExtensions) {
        m_fileToServer[ext.toLower()] = config.name;
    }

    return true;
}

bool LspService::removeServer(const QString& serverName) {
    for (int i = 0; i < m_serverList.size(); ++i) {
        if (m_serverList[i].name == serverName) {
            m_serverList.removeAt(i);
            break;
        }
    }

    // Stop and remove client
    auto clientIt = m_clients.find(serverName);
    if (clientIt != m_clients.end()) {
        clientIt->second->stop();
        m_clients.erase(clientIt);
    }

    // Remove extension mappings
    for (auto extIt = m_fileToServer.begin(); extIt != m_fileToServer.end();) {
        if (extIt->second == serverName) {
            extIt = m_fileToServer.erase(extIt);
        } else {
            ++extIt;
        }
    }

    return true;
}

QList<LspServerConfig> LspService::servers() const {
    return m_serverList;
}

QList<LspLocation> LspService::goToDefinition(const QString& filePath, int line, int character) {
    LspClient* client = findClientForFile(filePath);
    if (!client) {
        return {};
    }

    QJsonObject params;
    params["textDocument"] = QJsonObject{{"uri", QStringLiteral("file://%1").arg(filePath)}};
    params["position"] = QJsonObject{{"line", line}, {"character", character}};

    QJsonObject response = client->sendRequest("textDocument/definition", params);
    QList<LspLocation> locations;

    QJsonArray result = response["result"].toArray();
    for (const auto& item : result) {
        QJsonObject loc = item.toObject();
        LspLocation location;
        location.uri = loc["uri"].toString();
        QJsonObject range = loc["range"].toObject();
        location.line = range["start"].toObject()["line"].toInt();
        location.character = range["start"].toObject()["character"].toInt();
        locations.append(location);
    }

    return locations;
}

QList<LspLocation> LspService::findReferences(const QString& filePath, int line, int character) {
    LspClient* client = findClientForFile(filePath);
    if (!client) {
        return {};
    }

    QJsonObject params;
    params["textDocument"] = QJsonObject{{"uri", QStringLiteral("file://%1").arg(filePath)}};
    params["position"] = QJsonObject{{"line", line}, {"character", character}};
    params["context"] = QJsonObject{{"includeDeclaration", true}};

    QJsonObject response = client->sendRequest("textDocument/references", params);
    QList<LspLocation> locations;

    QJsonArray result = response["result"].toArray();
    for (const auto& item : result) {
        QJsonObject loc = item.toObject();
        LspLocation location;
        location.uri = loc["uri"].toString();
        QJsonObject range = loc["range"].toObject();
        location.line = range["start"].toObject()["line"].toInt();
        location.character = range["start"].toObject()["character"].toInt();
        locations.append(location);
    }

    return locations;
}

QString LspService::hover(const QString& filePath, int line, int character) {
    LspClient* client = findClientForFile(filePath);
    if (!client) {
        return {};
    }

    QJsonObject params;
    params["textDocument"] = QJsonObject{{"uri", QStringLiteral("file://%1").arg(filePath)}};
    params["position"] = QJsonObject{{"line", line}, {"character", character}};

    QJsonObject response = client->sendRequest("textDocument/hover", params);
    QJsonObject result = response["result"].toObject();

    if (result.contains("contents")) {
        QJsonObject contents = result["contents"].toObject();
        if (contents.contains("value")) {
            return contents["value"].toString();
        }
        // If contents is a string, return it directly
        return result["contents"].toString();
    }

    return {};
}

QList<LspSymbol> LspService::documentSymbol(const QString& filePath) {
    LspClient* client = findClientForFile(filePath);
    if (!client) {
        return {};
    }

    QJsonObject params;
    params["textDocument"] = QJsonObject{{"uri", QStringLiteral("file://%1").arg(filePath)}};

    QJsonObject response = client->sendRequest("textDocument/documentSymbol", params);
    QList<LspSymbol> symbols;

    QJsonArray result = response["result"].toArray();
    for (const auto& item : result) {
        QJsonObject sym = item.toObject();
        LspSymbol symbol;
        symbol.name = sym["name"].toString();
        symbol.kind = QString::number(sym["kind"].toInt());
        symbol.containerName = sym["containerName"].toString();
        QJsonObject range = sym["range"].toObject();
        symbol.line = range["start"].toObject()["line"].toInt();
        symbol.character = range["start"].toObject()["character"].toInt();
        symbols.append(symbol);
    }

    return symbols;
}

QList<LspSymbol> LspService::workspaceSymbol(const QString& query) {
    // Find any available client
    for (auto& [name, client] : m_clients) {
        if (client && client->isRunning()) {
            QJsonObject params;
            params["query"] = query;

            QJsonObject response = client->sendRequest("workspace/symbol", params);
            QList<LspSymbol> symbols;

            QJsonArray result = response["result"].toArray();
            for (const auto& item : result) {
                QJsonObject sym = item.toObject();
                LspSymbol symbol;
                symbol.name = sym["name"].toString();
                symbol.kind = QString::number(sym["kind"].toInt());
                symbol.containerName = sym["containerName"].toString();
                QJsonObject location = sym["location"].toObject();
                QJsonObject range = location["range"].toObject();
                symbol.line = range["start"].toObject()["line"].toInt();
                symbol.character = range["start"].toObject()["character"].toInt();
                symbols.append(symbol);
            }

            return symbols;
        }
    }

    return {};
}

QList<LspDiagnostic> LspService::diagnostics(const QString& filePath) {
    return m_diagnostics.value(QFileInfo(filePath).absoluteFilePath());
}

void LspService::openFile(const QString& filePath, const QString& content) {
    LspClient* client = findClientForFile(filePath);
    if (!client) {
        // Try to start a server for this file type
        QString ext = fileExtension(filePath);
        auto serverIt = m_fileToServer.find(ext.toLower());

        if (serverIt == m_fileToServer.end()) {
            return;
        }

        QString serverName = serverIt->second;

        // Find server config
        const LspServerConfig* config = nullptr;
        for (const auto& s : m_serverList) {
            if (s.name == serverName) {
                config = &s;
                break;
            }
        }

        if (!config) {
            return;
        }

        auto newClient = std::make_unique<LspClient>(serverName, this);

        if (newClient->start(config->command, config->arguments)) {
            connect(newClient.get(), &LspClient::notificationReceived, this,
                    [this](const QString& method, const QJsonObject& params) {
                        if (method != QStringLiteral("textDocument/publishDiagnostics"))
                            return;
                        const QString path =
                            QUrl(params.value(QStringLiteral("uri")).toString()).toLocalFile();
                        QList<LspDiagnostic> diagnostics;
                        for (const QJsonValue& value :
                             params.value(QStringLiteral("diagnostics")).toArray()) {
                            const QJsonObject diagnostic = value.toObject();
                            const QJsonObject start = diagnostic.value(QStringLiteral("range"))
                                                          .toObject()
                                                          .value(QStringLiteral("start"))
                                                          .toObject();
                            diagnostics.append(
                                {start.value(QStringLiteral("line")).toInt(),
                                 start.value(QStringLiteral("character")).toInt(),
                                 diagnostic.value(QStringLiteral("severity")).toInt(),
                                 diagnostic.value(QStringLiteral("message")).toString(),
                                 diagnostic.value(QStringLiteral("source")).toString()});
                        }
                        m_diagnostics.insert(path, diagnostics);
                        emit diagnosticsUpdated(path, diagnostics);
                    });
            newClient->initialize(QFileInfo(filePath).absolutePath());
            newClient->initialized();

            client = newClient.get();
            m_clients[serverName] = std::move(newClient);
        } else {
            return;
        }
    }

    QJsonObject params;
    params["textDocument"] = QJsonObject{{"uri", QStringLiteral("file://%1").arg(filePath)},
                                         {"languageId", fileExtension(filePath).mid(1)},
                                         {"version", 1},
                                         {"text", content}};

    client->sendNotification("textDocument/didOpen", params);
}

void LspService::closeFile(const QString& filePath) {
    LspClient* client = findClientForFile(filePath);
    if (!client) {
        return;
    }

    QJsonObject params;
    params["textDocument"] = QJsonObject{{"uri", QStringLiteral("file://%1").arg(filePath)}};

    client->sendNotification("textDocument/didClose", params);
}

void LspService::updateFile(const QString& filePath, const QString& content) {
    LspClient* client = findClientForFile(filePath);
    if (!client) {
        return;
    }

    QJsonObject params;
    params["textDocument"] =
        QJsonObject{{"uri", QStringLiteral("file://%1").arg(filePath)}, {"version", 2}};
    params["contentChanges"] = QJsonArray{QJsonObject{{"text", content}}};

    client->sendNotification("textDocument/didChange", params);
}

LspClient* LspService::findClientForFile(const QString& filePath) const {
    QString ext = fileExtension(filePath);
    auto serverIt = m_fileToServer.find(ext.toLower());

    if (serverIt == m_fileToServer.end()) {
        return nullptr;
    }

    return findClient(serverIt->second);
}

LspClient* LspService::findClient(const QString& serverName) const {
    auto it = m_clients.find(serverName);
    if (it == m_clients.end()) {
        return nullptr;
    }
    return it->second.get();
}

QString LspService::fileExtension(const QString& filePath) const {
    QFileInfo info(filePath);
    return info.suffix().toLower();
}

} // namespace sentinel::core
