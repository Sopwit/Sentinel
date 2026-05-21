#include "sentinel/core/JsonSettingsStore.h"

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
    if (!fileInfo.dir().exists()) {
        QDir().mkpath(fileInfo.dir().absolutePath());
    }

    QFile file(filePath_);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return;
    }

    file.write(QJsonDocument(values_).toJson(QJsonDocument::Indented));
}

} // namespace sentinel::core
