#pragma once

#include <QJsonObject>
#include <QMap>
#include <QString>
#include <functional>

namespace sentinel::core::plugin {

class PluginHookRegistry final {
public:
    using Hook = std::function<QJsonObject(const QJsonObject&)>;

    bool registerHook(const QString& name, Hook hook);
    bool unregisterHook(const QString& name);
    QJsonObject invoke(const QString& name, const QJsonObject& input) const;
    bool contains(const QString& name) const;

private:
    QMap<QString, Hook> hooks_;
};

} // namespace sentinel::core::plugin
