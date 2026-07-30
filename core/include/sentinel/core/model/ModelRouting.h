#pragma once

#include <QList>
#include <QString>
#include <QStringList>

namespace sentinel::core {

enum class ProviderKind {
    Local,
    Cloud,
};

enum class RoutingMode {
    Auto,
    Fast,
    Balanced,
    Quality,
    LocalOnly,
    CloudAllowed,
    BatterySaver,
};

enum class TaskType {
    Unknown,
    Chat,
    Summarization,
    Coding,
    Planning,
    LongContext,
    ToolPlanning,
    SensitiveData,
};

enum class ModelRoutingStatus {
    Unavailable,
    Routed,
    NoAvailableModel,
};

inline QString providerKindName(ProviderKind kind) {
    switch (kind) {
    case ProviderKind::Local:
        return QStringLiteral("Local");
    case ProviderKind::Cloud:
        return QStringLiteral("Cloud");
    }

    return QStringLiteral("Local");
}

inline QString routingModeName(RoutingMode mode) {
    switch (mode) {
    case RoutingMode::Auto:
        return QStringLiteral("Auto");
    case RoutingMode::Fast:
        return QStringLiteral("Fast");
    case RoutingMode::Balanced:
        return QStringLiteral("Balanced");
    case RoutingMode::Quality:
        return QStringLiteral("Quality");
    case RoutingMode::LocalOnly:
        return QStringLiteral("Local Only");
    case RoutingMode::CloudAllowed:
        return QStringLiteral("Cloud Allowed");
    case RoutingMode::BatterySaver:
        return QStringLiteral("Battery Saver");
    }

    return QStringLiteral("Auto");
}

inline QStringList routingModeNames() {
    return {
        routingModeName(RoutingMode::Auto),         routingModeName(RoutingMode::Fast),
        routingModeName(RoutingMode::Balanced),     routingModeName(RoutingMode::Quality),
        routingModeName(RoutingMode::LocalOnly),    routingModeName(RoutingMode::CloudAllowed),
        routingModeName(RoutingMode::BatterySaver),
    };
}

inline RoutingMode routingModeFromName(const QString& name) {
    const auto normalized = name.trimmed();
    if (normalized == routingModeName(RoutingMode::Auto)) {
        return RoutingMode::Auto;
    }
    if (normalized == routingModeName(RoutingMode::Fast)) {
        return RoutingMode::Fast;
    }
    if (normalized == routingModeName(RoutingMode::Balanced)) {
        return RoutingMode::Balanced;
    }
    if (normalized == routingModeName(RoutingMode::Quality)) {
        return RoutingMode::Quality;
    }
    if (normalized == routingModeName(RoutingMode::CloudAllowed)) {
        return RoutingMode::CloudAllowed;
    }
    if (normalized == routingModeName(RoutingMode::BatterySaver)) {
        return RoutingMode::BatterySaver;
    }
    return RoutingMode::LocalOnly;
}

inline QString normalizedRoutingModeName(const QString& name) {
    return routingModeName(routingModeFromName(name));
}

inline QString taskTypeName(TaskType type) {
    switch (type) {
    case TaskType::Unknown:
        return QStringLiteral("Unknown");
    case TaskType::Chat:
        return QStringLiteral("Chat");
    case TaskType::Summarization:
        return QStringLiteral("Summarization");
    case TaskType::Coding:
        return QStringLiteral("Coding");
    case TaskType::Planning:
        return QStringLiteral("Planning");
    case TaskType::LongContext:
        return QStringLiteral("Long Context");
    case TaskType::ToolPlanning:
        return QStringLiteral("Tool Planning");
    case TaskType::SensitiveData:
        return QStringLiteral("Sensitive Data");
    }

    return QStringLiteral("Unknown");
}

inline QString modelRoutingStatusName(ModelRoutingStatus status) {
    switch (status) {
    case ModelRoutingStatus::Unavailable:
        return QStringLiteral("Unavailable");
    case ModelRoutingStatus::Routed:
        return QStringLiteral("Routed");
    case ModelRoutingStatus::NoAvailableModel:
        return QStringLiteral("No Available Model");
    }

    return QStringLiteral("Unavailable");
}

struct ProviderCapabilityProfile {
    bool local = true;
    bool cloud = false;
    bool supportsChat = true;
    bool supportsTools = false;
    bool sensitiveDataAllowed = true;
    int contextWindowTokens = 0;
    QString latencyClass;
    QString costClass;
    QString privacyPosture;
    QStringList supportedTaskTypes;
};

struct ProviderDescriptor {
    QString id;
    QString name;
    ProviderKind kind = ProviderKind::Local;
    ProviderCapabilityProfile capabilityProfile;
};

struct ModelDescriptor {
    QString id;
    QString name;
    QString providerId;
    bool localOnly = true;
    bool installed = true;
    int contextWindowTokens = 0;
    QString qualityClass;
    QString latencyClass;
    QStringList recommendedTaskTypes;
};

struct TaskClassification {
    TaskType type = TaskType::Unknown;
    bool sensitive = false;
    QString summary;
};

struct ModelRoute {
    ModelRoutingStatus status = ModelRoutingStatus::Unavailable;
    RoutingMode routingMode = RoutingMode::LocalOnly;
    TaskClassification task;
    ProviderDescriptor provider;
    ModelDescriptor model;
    QString summary;
    bool networkRequired = false;
    bool modelExecutionAllowed = false;
};

inline QString safeModelRouteSummary(const ModelRoute& route) {
    if (route.status != ModelRoutingStatus::Routed) {
        return route.summary.isEmpty() ? modelRoutingStatusName(route.status) : route.summary;
    }

    if (!route.summary.isEmpty()) {
        return route.summary;
    }

    return QStringLiteral("%1 -> %2 / %3")
        .arg(routingModeName(route.routingMode), route.provider.name, route.model.name);
}

} // namespace sentinel::core
