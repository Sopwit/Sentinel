// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/notification/IClipboardService.h"
#include <QObject>
#include <QClipboard>
#include <QGuiApplication>

namespace sentinel::core {

class ClipboardService : public QObject, public IClipboardService {
    Q_OBJECT
public:
    explicit ClipboardService(QObject* parent = nullptr);
    ~ClipboardService() override;

    QString text() const override;
    void setText(const QString& text) override;
    QImage image() const override;
    void setImage(const QImage& image) override;
    bool hasImage() const override;
    bool hasText() const override;
    void clear() override;

private:
    QClipboard* clipboard() const;
};

} // namespace sentinel::core
