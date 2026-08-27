// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QString>

namespace sentinel::core {

struct ExportConfig {
    bool redactSensitive{true};
    bool includeMetadata{true};
    QString format{"json"};
};

class ISessionExportService {
public:
    virtual ~ISessionExportService() = default;

    virtual QString exportSession(const QString& sessionId,
                                  const ExportConfig& config = {}) const = 0;
    virtual bool importSession(const QString& jsonContent, QString& sessionId) = 0;
    virtual bool importFromUrl(const QString& url, QString& sessionId) = 0;
    virtual QJsonObject redactSensitive(const QJsonObject& data) const = 0;
};

} // namespace sentinel::core
