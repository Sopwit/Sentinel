// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SENTINEL_DESKTOP_SINGLEINSTANCEGUARD_H
#define SENTINEL_DESKTOP_SINGLEINSTANCEGUARD_H

#include <QLocalServer>
#include <QLockFile>
#include <QObject>
#include <QString>
#include <memory>

namespace sentinel::desktop {

class DesktopShellViewModel;

class SingleInstanceGuard final : public QObject {
    Q_OBJECT
public:
    explicit SingleInstanceGuard(QObject* parent = nullptr);
    ~SingleInstanceGuard() override;

    bool tryLockAndSetupIpc();
    void bindShellViewModel(DesktopShellViewModel* shellViewModel);

private:
    std::unique_ptr<QLockFile> m_lockFile;
    QLocalServer m_ipcServer;
    DesktopShellViewModel* m_shellViewModel{nullptr};
};

} // namespace sentinel::desktop

#endif // SENTINEL_DESKTOP_SINGLEINSTANCEGUARD_H
