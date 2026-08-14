// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>

namespace sentinel::core {

class MimeSniffer {
public:
    static QString sniffMimeType(const QByteArray& data);
    static QString mimeTypeFromExtension(const QString& filePath);
    static QString mimeTypeFromData(const QByteArray& data);
    static bool isText(const QString& mimeType);
    static bool isBinary(const QString& mimeType);
};

} // namespace sentinel::core
