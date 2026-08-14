// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/notification/ClipboardService.h"
#include <QMimeData>

namespace sentinel::core {

ClipboardService::ClipboardService(QObject* parent) : QObject(parent) {}
ClipboardService::~ClipboardService() = default;

QClipboard* ClipboardService::clipboard() const {
    return QGuiApplication::clipboard();
}

QString ClipboardService::text() const {
    return clipboard()->text();
}

void ClipboardService::setText(const QString& text) {
    clipboard()->setText(text);
}

QImage ClipboardService::image() const {
    return clipboard()->image();
}

void ClipboardService::setImage(const QImage& image) {
    clipboard()->setImage(image);
}

bool ClipboardService::hasImage() const {
    return clipboard()->mimeData()->hasImage();
}

bool ClipboardService::hasText() const {
    return clipboard()->mimeData()->hasText();
}

void ClipboardService::clear() {
    clipboard()->clear();
}

} // namespace sentinel::core
