#include "SingleInstanceGuard.h"

#include "sentinel/core/platform/WinProtocolHandler.h"
#include "sentinel/desktop/DesktopShellViewModel.h"

#include <QCoreApplication>
#include <QDebug>
#include <QLocalSocket>
#include <QStandardPaths>

namespace sentinel::desktop {

SingleInstanceGuard::SingleInstanceGuard(QObject* parent) : QObject(parent) {}
SingleInstanceGuard::~SingleInstanceGuard() = default;

bool SingleInstanceGuard::tryLockAndSetupIpc() {
    const QString lockPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation)
                             + QStringLiteral("/sentinel-desktop.lock");
    m_lockFile = std::make_unique<QLockFile>(lockPath);
    m_lockFile->setStaleLockTime(0);

    if (!m_lockFile->tryLock(100)) {
        const QString incomingUrl =
            sentinel::core::extractSentinelUrl(QCoreApplication::arguments());
        if (!incomingUrl.isEmpty()) {
            QLocalSocket socket;
            socket.connectToServer(sentinel::core::sentinelIpcServerName);
            if (socket.waitForConnected(500)) {
                socket.write(incomingUrl.toUtf8());
                socket.waitForBytesWritten(500);
                socket.disconnectFromServer();
            }
        }
        qWarning().noquote() << "Sentinel is already running. Only one instance is allowed.";
        return false;
    }

    // Start IPC server for protocol handler URL forwarding
    QLocalServer::removeServer(sentinel::core::sentinelIpcServerName);
    m_ipcServer.listen(sentinel::core::sentinelIpcServerName);

    return true;
}

void SingleInstanceGuard::bindShellViewModel(DesktopShellViewModel* shellViewModel) {
    m_shellViewModel = shellViewModel;
    if (!m_shellViewModel) {
        return;
    }

    QObject::connect(&m_ipcServer, &QLocalServer::newConnection, this, [this]() {
        while (auto* socket = m_ipcServer.nextPendingConnection()) {
            socket->waitForReadyRead(1000);
            const QString url = QString::fromUtf8(socket->readAll());
            if (!url.isEmpty() && m_shellViewModel) {
                qInfo().noquote() << "Deep link received:" << url;
                m_shellViewModel->addNotification(
                    QStringLiteral("System"), QStringLiteral("Deep Link"),
                    QStringLiteral("Received: %1").arg(url));
            }
            socket->deleteLater();
        }
    });
}

} // namespace sentinel::desktop
