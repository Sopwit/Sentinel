// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/mimetype/MimeSniffer.h"
#include <QFileInfo>

namespace sentinel::core {

QString MimeSniffer::sniffMimeType(const QByteArray& data) {
    if (data.size() >= 3 && static_cast<unsigned char>(data[0]) == 0xEF &&
        static_cast<unsigned char>(data[1]) == 0xBB &&
        static_cast<unsigned char>(data[2]) == 0xBF) {
        return "text/plain; charset=utf-8";
    }

    if (data.size() >= 4 && static_cast<unsigned char>(data[0]) == 0x25 &&
        static_cast<unsigned char>(data[1]) == 0x50 &&
        static_cast<unsigned char>(data[2]) == 0x44 &&
        static_cast<unsigned char>(data[3]) == 0x46) {
        return "application/pdf";
    }

    if (data.size() >= 8 && static_cast<unsigned char>(data[0]) == 0x89 &&
        static_cast<unsigned char>(data[1]) == 0x50 &&
        static_cast<unsigned char>(data[2]) == 0x4E &&
        static_cast<unsigned char>(data[3]) == 0x47) {
        return "image/png";
    }

    if (data.size() >= 3 && static_cast<unsigned char>(data[0]) == 0xFF &&
        static_cast<unsigned char>(data[1]) == 0xD8 &&
        static_cast<unsigned char>(data[2]) == 0xFF) {
        return "image/jpeg";
    }

    bool isText = true;
    for (int i = 0; i < qMin(data.size(), 512); ++i) {
        if (static_cast<unsigned char>(data[i]) == 0) {
            isText = false;
            break;
        }
    }

    return isText ? "text/plain" : "application/octet-stream";
}

QString MimeSniffer::mimeTypeFromExtension(const QString& filePath) {
    QString ext = QFileInfo(filePath).suffix().toLower();
    if (ext == "txt" || ext == "md" || ext == "json" || ext == "xml" || ext == "html" ||
        ext == "css")
        return "text/plain";
    if (ext == "png")
        return "image/png";
    if (ext == "jpg" || ext == "jpeg")
        return "image/jpeg";
    if (ext == "gif")
        return "image/gif";
    if (ext == "pdf")
        return "application/pdf";
    if (ext == "cpp" || ext == "h" || ext == "c" || ext == "py" || ext == "js" || ext == "ts")
        return "text/plain";
    return "application/octet-stream";
}

QString MimeSniffer::mimeTypeFromData(const QByteArray& data) {
    return sniffMimeType(data);
}

bool MimeSniffer::isText(const QString& mimeType) {
    return mimeType.startsWith("text/") || mimeType == "application/json" ||
           mimeType == "application/xml";
}

bool MimeSniffer::isBinary(const QString& mimeType) {
    return !isText(mimeType);
}

} // namespace sentinel::core
