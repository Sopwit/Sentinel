#pragma once

#include <QJsonObject>
#include <QStringList>

namespace sentinel::core {

struct ResolvedConfig {
    QJsonObject value;
    QStringList sources;
    QStringList errors;
};

class ConfigResolver final {
public:
    static ResolvedConfig resolve(const QString& projectDirectory,
                                  const QJsonObject& globalConfig = {},
                                  const QJsonObject& managedConfig = {});
    static QJsonObject deepMerge(const QJsonObject& base, const QJsonObject& overlay);
};

} // namespace sentinel::core
