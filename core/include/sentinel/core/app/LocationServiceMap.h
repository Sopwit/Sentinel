#pragma once

#include <QDir>
#include <QHash>
#include <QString>
#include <functional>
#include <memory>

namespace sentinel::core {

template <typename Service> class LocationServiceMap final {
public:
    using Factory = std::function<std::shared_ptr<Service>(const QString&)>;

    explicit LocationServiceMap(Factory factory) : factory_(std::move(factory)) {}

    std::shared_ptr<Service> get(const QString& location) {
        const QString key = QDir(location).absolutePath();
        const auto it = services_.constFind(key);
        if (it != services_.constEnd()) {
            return it.value();
        }
        const auto service = factory_(key);
        services_.insert(key, service);
        return service;
    }

    void invalidate(const QString& location) {
        services_.remove(QDir(location).absolutePath());
    }
    void clear() {
        services_.clear();
    }
    int size() const {
        return services_.size();
    }

private:
    Factory factory_;
    QHash<QString, std::shared_ptr<Service>> services_;
};

} // namespace sentinel::core
