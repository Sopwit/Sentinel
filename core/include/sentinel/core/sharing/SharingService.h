// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/sharing/ISharingService.h"
#include <QObject>
#include <QMap>
#include <QTimer>
#include <QNetworkAccessManager>

namespace sentinel::core {

class SharingService : public QObject, public ISharingService {
    Q_OBJECT
public:
    explicit SharingService(QObject* parent = nullptr);
    ~SharingService() override;

    // ISharingService interface
    ShareRecord createShare(const QString& sessionId) override;
    bool deleteShare(const QString& shareId) override;
    std::optional<ShareRecord> findShare(const QString& shareId) const override;
    QList<ShareRecord> shares() const override;
    std::optional<ShareRecord> shareBySession(const QString& sessionId) const override;

    bool syncShare(const QString& shareId) override;
    void enableAutoSync(bool enabled) override;
    bool isAutoSyncEnabled() const override;

    void configure(const ShareConfig& config) override;
    ShareConfig config() const override;

signals:
    void shareCreated(const QString& shareId, const QString& url);
    void shareDeleted(const QString& shareId);
    void shareSynced(const QString& shareId);
    void shareError(const QString& shareId, const QString& error);

private slots:
    void performAutoSync();

private:
    QString generateId() const;
    QString generateSecret() const;
    ShareRecord createShareRecord(const QString& sessionId);

    ShareConfig m_config;
    QMap<QString, ShareRecord> m_shares;
    QNetworkAccessManager m_networkManager;
    QTimer m_syncTimer;
};

} // namespace sentinel::core
