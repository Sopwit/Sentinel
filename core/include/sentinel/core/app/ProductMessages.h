// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QJsonObject>
#include <QString>

namespace sentinel::core {
struct ProductMessage {
    QString id;
    QJsonObject parameters;
    QString safeDiagnostic;
};

// Stable IDs stay in backend events and storage. Qt translates presentation text.
class ProductMessages final {
public:
    static QString present(const ProductMessage& message);
};
} // namespace sentinel::core
