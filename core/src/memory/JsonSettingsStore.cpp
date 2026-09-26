// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/memory/JsonSettingsStore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>

namespace sentinel::core {

JsonSettingsStore::JsonSettingsStore(QString filePath) : filePath_(std::move(filePath)) {
    load();
}

QString JsonSettingsStore::value(const QString& key, const QString& defaultValue) const {
    const auto jsonValue = values_.value(key);
    return jsonValue.isString() ? jsonValue.toString() : defaultValue;
}

void JsonSettingsStore::setValue(QString key, QString value) {
    values_.insert(key, value);
    save();
}

void JsonSettingsStore::remove(const QString& key) {
    if (values_.remove(key) > 0)
        save();
}

QString JsonSettingsStore::filePath() const {
    return filePath_;
}

void JsonSettingsStore::load() {
    QFile file(filePath_);
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        return;
    }

    const auto document = QJsonDocument::fromJson(file.readAll());
    if (document.isObject()) {
        values_ = document.object();
    }
}

void JsonSettingsStore::save() const {
    const QFileInfo fileInfo(filePath_);
    const QDir parentDir = fileInfo.dir();
    const bool createdParent = !parentDir.exists();
    if (createdParent) {
        QDir().mkpath(parentDir.absolutePath());
        QFile::setPermissions(parentDir.absolutePath(),
                              QFileDevice::ReadOwner | QFileDevice::WriteOwner |
                                  QFileDevice::ExeOwner);
    }

    QFile file(filePath_);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return;
    }

    file.write(QJsonDocument(values_).toJson(QJsonDocument::Indented));
    file.close();

    QFile::setPermissions(filePath_, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
}

} // namespace sentinel::core
