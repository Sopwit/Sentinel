// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/session/ISessionExportService.h"
#include "sentinel/core/chat/IConversationStore.h"
#include <QObject>

namespace sentinel::core {

class SessionExportService : public QObject, public ISessionExportService {
    Q_OBJECT
public:
    explicit SessionExportService(QObject* parent = nullptr);
    ~SessionExportService() override;

    QString exportSession(const QString& sessionId, const ExportConfig& config = {}) const override;
    bool importSession(const QString& jsonContent, QString& sessionId) override;
    bool importFromUrl(const QString& url, QString& sessionId) override;
    QJsonObject redactSensitive(const QJsonObject& data) const override;

    void setConversationStore(IConversationStore* store);

private:
    QString generateSessionId() const;
    IConversationStore* conversationStore_{nullptr};
};

} // namespace sentinel::core
