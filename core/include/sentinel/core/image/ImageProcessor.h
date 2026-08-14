// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QImage>
#include <QByteArray>

namespace sentinel::core {

struct ImageConfig {
    int maxWidth{1024};
    int maxHeight{1024};
    qint64 maxBase64Size{5242880};
    int jpegQuality{85};
    bool autoResize{true};
    bool autoCompress{true};
};

class ImageProcessor {
public:
    explicit ImageProcessor(const ImageConfig& config = {});

    QImage normalize(const QImage& image) const;
    QByteArray toBase64(const QImage& image, const QString& format = "JPEG") const;
    QImage fromBase64(const QByteArray& data) const;
    QImage loadAndNormalize(const QString& filePath) const;
    bool fitsLimits(const QImage& image) const;

private:
    QImage resize(const QImage& image) const;
    QImage compress(const QImage& image) const;

    ImageConfig m_config;
};

} // namespace sentinel::core
