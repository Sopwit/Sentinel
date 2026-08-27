// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/desktop/GraphicsBackend.h"

namespace sentinel::desktop {

QString graphicsApiName(QSGRendererInterface::GraphicsApi api) {
    if (api == QSGRendererInterface::Unknown) {
        return QStringLiteral("automatic");
    }
    if (api == QSGRendererInterface::Software) {
        return QStringLiteral("software");
    }
    if (api == QSGRendererInterface::OpenVG) {
        return QStringLiteral("OpenVG");
    }
    if (api == QSGRendererInterface::OpenGL) {
        return QStringLiteral("OpenGL");
    }
    if (api == QSGRendererInterface::Direct3D11) {
        return QStringLiteral("Direct3D 11");
    }
    if (api == QSGRendererInterface::Vulkan) {
        return QStringLiteral("Vulkan");
    }
    if (api == QSGRendererInterface::Metal) {
        return QStringLiteral("Metal");
    }
    if (api == QSGRendererInterface::Null) {
        return QStringLiteral("null");
    }

    return QStringLiteral("unknown");
}

std::optional<QSGRendererInterface::GraphicsApi>
linuxDefaultGraphicsApi(const QByteArray& qsgRhiBackend, const QByteArray& qtQuickBackend) {
    if (!qsgRhiBackend.isEmpty() || !qtQuickBackend.isEmpty()) {
        return std::nullopt;
    }

    return QSGRendererInterface::OpenGL;
}

QSGRendererInterface::GraphicsApi defaultGraphicsApiForPlatform() {
#if defined(Q_OS_MACOS)
    return QSGRendererInterface::Metal;
#elif defined(Q_OS_WIN)
    return QSGRendererInterface::Direct3D11;
#elif defined(Q_OS_LINUX)
    const auto api =
        linuxDefaultGraphicsApi(qgetenv("QSG_RHI_BACKEND"), qgetenv("QT_QUICK_BACKEND"));
    return api.value_or(QSGRendererInterface::OpenGL);
#else
    return QSGRendererInterface::OpenGL;
#endif
}

} // namespace sentinel::desktop
