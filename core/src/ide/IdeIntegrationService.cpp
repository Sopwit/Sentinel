// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/ide/IdeIntegrationService.h"
#include <QProcess>
#include <QDir>
#include <QStandardPaths>

namespace sentinel::core {

IdeIntegrationService::IdeIntegrationService() {
    m_supportedIdes["vscode"] = {"Visual Studio Code", "code", ".vscode", false};
    m_supportedIdes["cursor"] = {"Cursor", "cursor", ".cursor", false};
    m_supportedIdes["windsurf"] = {"Windsurf", "windsurf", ".windsurf", false};
    m_supportedIdes["vscodium"] = {"VSCodium", "codium", ".vscode", false};
    m_supportedIdes["zed"] = {"Zed", "zed", ".zed", false};
}

IdeInfo IdeIntegrationService::detectIde() const {
    IdeInfo info = detectFromEnvironment();
    if (info.detected) return info;
    return detectFromLockFiles();
}

bool IdeIntegrationService::isIdeRunning() const {
    return !m_detectedIde.isEmpty();
}

QString IdeIntegrationService::ideName() const {
    return m_detectedIde;
}

bool IdeIntegrationService::installExtension(const QString& ideName) const {
    if (!m_supportedIdes.contains(ideName)) return false;

    const IdeInfo& info = m_supportedIdes[ideName];
    QProcess process;
    process.start(info.executable, {"--install-extension", "sentinel.sentinel"});
    process.waitForFinished(10000);
    return process.exitCode() == 0;
}

QMap<QString, IdeInfo> IdeIntegrationService::supportedIdes() const { return m_supportedIdes; }

void IdeIntegrationService::setDetectedIde(const QString& name) { m_detectedIde = name; }

IdeInfo IdeIntegrationService::detectFromEnvironment() const {
    QString termProgram = qgetenv("TERM_PROGRAM");
    if (!termProgram.isEmpty()) {
        for (auto it = m_supportedIdes.begin(); it != m_supportedIdes.end(); ++it) {
            if (termProgram.contains(it.key(), Qt::CaseInsensitive)) {
                IdeInfo info = it.value();
                info.detected = true;
                return info;
            }
        }
    }

    QString caller = qgetenv("OPENCODE_CALLER");
    if (!caller.isEmpty()) {
        for (auto it = m_supportedIdes.begin(); it != m_supportedIdes.end(); ++it) {
            if (caller.contains(it.key(), Qt::CaseInsensitive)) {
                IdeInfo info = it.value();
                info.detected = true;
                return info;
            }
        }
    }

    return {};
}

IdeInfo IdeIntegrationService::detectFromLockFiles() const {
    QString home = QDir::homePath();
    for (auto it = m_supportedIdes.begin(); it != m_supportedIdes.end(); ++it) {
        QString lockPath = home + "/" + it.value().configDir + "/lock";
        if (QFile::exists(lockPath)) {
            IdeInfo info = it.value();
            info.detected = true;
            return info;
        }
    }
    return {};
}

} // namespace sentinel::core
