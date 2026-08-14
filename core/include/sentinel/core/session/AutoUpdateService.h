// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/session/IAutoUpdateService.h"
#include <QObject>
#include <QUrl>

namespace sentinel::core {

class AutoUpdateService : public QObject, public IAutoUpdateService {
    Q_OBJECT
public:
    explicit AutoUpdateService(QUrl manifestUrl = {}, QObject* parent = nullptr);
    ~AutoUpdateService() override;

    UpdateInfo checkForUpdates() const override;
    bool downloadUpdate(const QString& version) override;
    bool applyUpdate() override;
    void setAutoCheck(bool enabled) override;
    bool autoCheckEnabled() const override;
    QString currentVersion() const override;

    void setManifestUrl(const QUrl& url);

private:
    bool m_autoCheck{false};
    QUrl m_manifestUrl;
    UpdateInfo m_lastUpdate;
    QString m_downloadedArtifact;
    QByteArray m_expectedSha256;
};

} // namespace sentinel::core
