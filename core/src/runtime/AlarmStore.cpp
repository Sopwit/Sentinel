// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/runtime/AlarmStore.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>

namespace sentinel::core {

namespace {

QDateTime parseUtc(const QString& text) {
    return QDateTime::fromString(text, Qt::ISODateWithMs);
}

} // namespace

AlarmStore::AlarmStore(QString persistencePath)
    : persistencePath_(std::move(persistencePath)) {
    load();
}

AlarmEntry AlarmStore::schedule(const QDateTime& triggerAt, const QString& label) {
    AlarmEntry entry;
    entry.triggerAt = triggerAt;
    entry.label = label;
    entry.createdAt = QDateTime::currentDateTime();

    const std::lock_guard<std::mutex> lock(mutex_);
    entry.id = QStringLiteral("alarm-%1-%2")
                   .arg(QString::number(++idSequence_),
                        QUuid::createUuid().toString(QUuid::WithoutBraces).left(8));
    alarms_.append(entry);
    saveLocked();
    return entry;
}

bool AlarmStore::remove(const QString& alarmId) {
    const std::lock_guard<std::mutex> lock(mutex_);
    for (int i = 0; i < alarms_.size(); ++i) {
        if (alarms_.at(i).id == alarmId) {
            alarms_.removeAt(i);
            saveLocked();
            return true;
        }
    }
    return false;
}

QList<AlarmEntry> AlarmStore::active() const {
    const std::lock_guard<std::mutex> lock(mutex_);
    return alarms_;
}

QList<AlarmEntry> AlarmStore::takeDue(const QDateTime& now) {
    const std::lock_guard<std::mutex> lock(mutex_);
    QList<AlarmEntry> due;
    for (int i = alarms_.size() - 1; i >= 0; --i) {
        if (alarms_.at(i).triggerAt <= now) {
            due.prepend(alarms_.takeAt(i));
        }
    }
    if (!due.isEmpty()) {
        saveLocked();
    }
    return due;
}

bool AlarmStore::load() {
    const std::lock_guard<std::mutex> lock(mutex_);
    if (persistencePath_.isEmpty()) {
        return false;
    }

    QFile file(persistencePath_);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    const auto document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject()) {
        return false;
    }

    alarms_.clear();
    idSequence_ = 0;
    const auto array = document.object().value(QStringLiteral("alarms")).toArray();
    for (const auto& value : array) {
        const auto object = value.toObject();
        AlarmEntry entry;
        entry.id = object.value(QStringLiteral("id")).toString();
        entry.label = object.value(QStringLiteral("label")).toString();
        entry.triggerAt = parseUtc(object.value(QStringLiteral("triggerAt")).toString());
        entry.createdAt = parseUtc(object.value(QStringLiteral("createdAt")).toString());
        if (entry.id.isEmpty() || !entry.triggerAt.isValid()) {
            continue;
        }
        alarms_.append(entry);
        idSequence_ += 1;
    }
    return true;
}

bool AlarmStore::save() const {
    const std::lock_guard<std::mutex> lock(mutex_);
    return saveLocked();
}

bool AlarmStore::saveLocked() const {
    if (persistencePath_.isEmpty()) {
        return false;
    }

    QJsonArray array;
    for (const auto& alarm : alarms_) {
        QJsonObject object;
        object.insert(QStringLiteral("id"), alarm.id);
        object.insert(QStringLiteral("label"), alarm.label);
        object.insert(QStringLiteral("triggerAt"), alarm.triggerAt.toString(Qt::ISODateWithMs));
        object.insert(QStringLiteral("createdAt"), alarm.createdAt.toString(Qt::ISODateWithMs));
        array.append(object);
    }

    QJsonObject root;
    root.insert(QStringLiteral("alarms"), array);

    QFile file(persistencePath_);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
    return true;
}

} // namespace sentinel::core
