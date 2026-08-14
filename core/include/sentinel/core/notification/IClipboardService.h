// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QImage>
#include <QByteArray>

namespace sentinel::core {

class IClipboardService {
public:
    virtual ~IClipboardService() = default;

    virtual QString text() const = 0;
    virtual void setText(const QString& text) = 0;
    virtual QImage image() const = 0;
    virtual void setImage(const QImage& image) = 0;
    virtual bool hasImage() const = 0;
    virtual bool hasText() const = 0;
    virtual void clear() = 0;
};

} // namespace sentinel::core
