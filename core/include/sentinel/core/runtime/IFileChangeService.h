// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QJsonObject>
#include <QSet>

namespace sentinel::core {

enum class FileChangeType : std::uint8_t {
    Created,
    Modified,
    Deleted
};

struct FileChangeEvent {
    QString filePath;
    FileChangeType type;
    QDateTime timestamp;
};

class IFileChangeService {
public:
    virtual ~IFileChangeService() = default;

    virtual void publishEvent(const FileChangeEvent& event) = 0;
    virtual void subscribe(const QString& pattern, std::function<void(const FileChangeEvent&)> callback) = 0;
    virtual void unsubscribe(const QString& pattern) = 0;
    virtual QList<FileChangeEvent> recentEvents(int count = 100) const = 0;
};

} // namespace sentinel::core
