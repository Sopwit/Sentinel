// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QMap>

namespace sentinel::core {

struct IdeInfo {
    QString name;
    QString executable;
    QString configDir;
    bool detected{false};
};

class IdeIntegrationService {
public:
    IdeIntegrationService();

    IdeInfo detectIde() const;
    bool isIdeRunning() const;
    QString ideName() const;
    bool installExtension(const QString& ideName) const;
    QMap<QString, IdeInfo> supportedIdes() const;
    void setDetectedIde(const QString& name);

private:
    IdeInfo detectFromEnvironment() const;
    IdeInfo detectFromLockFiles() const;
    QString m_detectedIde;
    QMap<QString, IdeInfo> m_supportedIdes;
};

} // namespace sentinel::core
