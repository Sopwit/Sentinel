// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "GraphicsInitializer.h"

#include "sentinel/desktop/GraphicsBackend.h"

#include <QDebug>
#include <QQuickWindow>
#include <QSGRendererInterface>

namespace sentinel::desktop {

void configureGraphicsBackend() {
    qInfo().noquote() << "Sentinel Qt version:" << qVersion();
    const auto api = sentinel::desktop::defaultGraphicsApiForPlatform();
    QQuickWindow::setGraphicsApi(api);
    qInfo().noquote() << "Sentinel graphics backend requested:"
                      << sentinel::desktop::graphicsApiName(api);
}

void installGraphicsDiagnostics(QQuickWindow& window) {
    const auto logSelectedBackend = [&window]() {
        qInfo().noquote() << "Sentinel graphics backend selected:"
                          << sentinel::desktop::graphicsApiName(
                                 window.rendererInterface()->graphicsApi());
    };

    QObject::connect(&window, &QQuickWindow::sceneGraphInitialized, &window, logSelectedBackend,
                     Qt::DirectConnection);
    QObject::connect(&window, &QQuickWindow::sceneGraphError, &window,
                     [](QQuickWindow::SceneGraphError error, const QString& message) {
                         qCritical().noquote()
                             << "Sentinel scene graph initialization failed; error:" << error
                             << "message:" << message;
                     });
}

} // namespace sentinel::desktop
