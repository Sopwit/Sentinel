// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/image/ImageProcessor.h"
#include <QBuffer>

namespace sentinel::core {

ImageProcessor::ImageProcessor(const ImageConfig& config) : m_config(config) {}

QImage ImageProcessor::normalize(const QImage& image) const {
    QImage result = image;
    if (m_config.autoResize) {
        result = resize(result);
    }
    if (m_config.autoCompress && result.format() != QImage::Format_RGB32) {
        result = result.convertToFormat(QImage::Format_RGB32);
    }
    return result;
}

QByteArray ImageProcessor::toBase64(const QImage& image, const QString& format) const {
    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, format.toUtf8().constData(), m_config.jpegQuality);
    return ba.toBase64();
}

QImage ImageProcessor::fromBase64(const QByteArray& data) const {
    QByteArray decoded = QByteArray::fromBase64(data);
    QImage image;
    image.loadFromData(decoded);
    return image;
}

QImage ImageProcessor::loadAndNormalize(const QString& filePath) const {
    QImage image(filePath);
    if (image.isNull())
        return image;
    return normalize(image);
}

bool ImageProcessor::fitsLimits(const QImage& image) const {
    qint64 estimatedBase64 = static_cast<qint64>(image.width() * image.height() * 3) * 4 / 3;
    return image.width() <= m_config.maxWidth && image.height() <= m_config.maxHeight &&
           estimatedBase64 <= m_config.maxBase64Size;
}

QImage ImageProcessor::resize(const QImage& image) const {
    if (image.width() <= m_config.maxWidth && image.height() <= m_config.maxHeight) {
        return image;
    }
    return image.scaled(m_config.maxWidth, m_config.maxHeight, Qt::KeepAspectRatio,
                        Qt::SmoothTransformation);
}

QImage ImageProcessor::compress(const QImage& image) const {
    return image;
}

} // namespace sentinel::core
