// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/bom/BomHandler.h"

namespace sentinel::core {

BomType BomHandler::detectBom(const QByteArray& data) {
    if (data.size() >= 3 && 
        static_cast<unsigned char>(data[0]) == 0xEF &&
        static_cast<unsigned char>(data[1]) == 0xBB &&
        static_cast<unsigned char>(data[2]) == 0xBF) {
        return BomType::UTF8;
    }
    if (data.size() >= 2) {
        if (static_cast<unsigned char>(data[0]) == 0xFF && static_cast<unsigned char>(data[1]) == 0xFE)
            return BomType::UTF16LE;
        if (static_cast<unsigned char>(data[0]) == 0xFE && static_cast<unsigned char>(data[1]) == 0xFF)
            return BomType::UTF16BE;
    }
    if (data.size() >= 4 &&
        static_cast<unsigned char>(data[0]) == 0xFF &&
        static_cast<unsigned char>(data[1]) == 0xFE &&
        static_cast<unsigned char>(data[2]) == 0x00 &&
        static_cast<unsigned char>(data[3]) == 0x00) {
        return BomType::UTF32LE;
    }
    return BomType::None;
}

QByteArray BomHandler::removeBom(const QByteArray& data) {
    BomType bom = detectBom(data);
    switch (bom) {
        case BomType::UTF8: return data.mid(3);
        case BomType::UTF16LE:
        case BomType::UTF16BE: return data.mid(2);
        case BomType::UTF32LE:
        case BomType::UTF32BE: return data.mid(4);
        default: return data;
    }
}

QByteArray BomHandler::addBom(const QByteArray& data, BomType type) {
    if (hasBom(data)) return data;
    switch (type) {
        case BomType::UTF8: return QByteArray("\xEF\xBB\xBF", 3) + data;
        case BomType::UTF16LE: return QByteArray("\xFF\xFE", 2) + data;
        case BomType::UTF16BE: return QByteArray("\xFE\xFF", 2) + data;
        case BomType::UTF32LE: return QByteArray("\xFF\xFE\x00\x00", 4) + data;
        case BomType::UTF32BE: return QByteArray("\x00\x00\xFE\xFF", 4) + data;
        default: return data;
    }
}

QString BomHandler::bomName(BomType type) {
    switch (type) {
        case BomType::UTF8: return "UTF-8 BOM";
        case BomType::UTF16LE: return "UTF-16 LE BOM";
        case BomType::UTF16BE: return "UTF-16 BE BOM";
        case BomType::UTF32LE: return "UTF-32 LE BOM";
        case BomType::UTF32BE: return "UTF-32 BE BOM";
        default: return "No BOM";
    }
}

bool BomHandler::hasBom(const QByteArray& data) {
    return detectBom(data) != BomType::None;
}

} // namespace sentinel::core
