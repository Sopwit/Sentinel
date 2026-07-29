#pragma once

#include <QByteArray>
#include <QSGRendererInterface>
#include <QString>

#include <optional>

namespace sentinel::desktop {

QString graphicsApiName(QSGRendererInterface::GraphicsApi api);

std::optional<QSGRendererInterface::GraphicsApi>
linuxDefaultGraphicsApi(const QByteArray& qsgRhiBackend, const QByteArray& qtQuickBackend);

QSGRendererInterface::GraphicsApi defaultGraphicsApiForPlatform();

} // namespace sentinel::desktop
