// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QByteArray>
#include <QString>

namespace sentinel::core {

enum class BomType : std::uint8_t { None, UTF8, UTF16LE, UTF16BE, UTF32LE, UTF32BE };

class BomHandler {
public:
    static BomType detectBom(const QByteArray& data);
    static QByteArray removeBom(const QByteArray& data);
    static QByteArray addBom(const QByteArray& data, BomType type);
    static QString bomName(BomType type);
    static bool hasBom(const QByteArray& data);
};

} // namespace sentinel::core
