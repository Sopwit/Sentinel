#include "sentinel/core/plugin/PluginHookRegistry.h"

namespace sentinel::core::plugin {

bool PluginHookRegistry::registerHook(const QString& name, Hook hook) {
    if (name.isEmpty() || !hook) {
        return false;
    }
    hooks_.insert(name, std::move(hook));
    return true;
}

bool PluginHookRegistry::unregisterHook(const QString& name) {
    return hooks_.remove(name) > 0;
}

QJsonObject PluginHookRegistry::invoke(const QString& name, const QJsonObject& input) const {
    const auto it = hooks_.constFind(name);
    return it == hooks_.constEnd() ? input : it.value()(input);
}

bool PluginHookRegistry::contains(const QString& name) const {
    return hooks_.contains(name);
}

} // namespace sentinel::core::plugin
