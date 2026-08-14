// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QJsonObject>

namespace sentinel::core {

enum class ModelStatus : std::uint8_t {
    Active,
    Alpha,
    Beta,
    Deprecated
};

struct ModelInfo {
    QString id;
    QString provider;
    QString displayName;
    ModelStatus status{ModelStatus::Active};
    int contextWindow{0};
    double inputCostPer1k{0.0};
    double outputCostPer1k{0.0};
};

class ModelLifecycleService {
public:
    ModelLifecycleService();

    ModelInfo modelInfo(const QString& provider, const QString& model) const;
    QList<ModelInfo> availableModels(const QString& provider = {}) const;
    bool isDeprecated(const QString& provider, const QString& model) const;
    bool isExperimental(const QString& provider, const QString& model) const;
    void registerModel(const ModelInfo& info);
    QList<ModelInfo> deprecatedModels() const;

private:
    QList<ModelInfo> m_models;
};

} // namespace sentinel::core
