// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/model/ModelLifecycleService.h"

namespace sentinel::core {

ModelLifecycleService::ModelLifecycleService() {
    m_models.append({"gpt-4o", "openai", "GPT-4o", ModelStatus::Active, 128000, 0.005, 0.015});
    m_models.append({"gpt-4o-mini", "openai", "GPT-4o Mini", ModelStatus::Active, 128000, 0.00015, 0.0006});
    m_models.append({"gpt-4-turbo", "openai", "GPT-4 Turbo", ModelStatus::Active, 128000, 0.01, 0.03});
    m_models.append({"claude-sonnet-4-20250514", "anthropic", "Claude Sonnet 4", ModelStatus::Active, 200000, 0.003, 0.015});
    m_models.append({"claude-3-5-haiku-20241022", "anthropic", "Claude 3.5 Haiku", ModelStatus::Active, 200000, 0.001, 0.005});
    m_models.append({"gemini-2.5-pro", "google", "Gemini 2.5 Pro", ModelStatus::Active, 1000000, 0.00125, 0.005});
}

ModelInfo ModelLifecycleService::modelInfo(const QString& provider, const QString& model) const {
    for (const auto& m : m_models) {
        if (m.provider == provider && m.id == model) return m;
    }
    return {};
}

QList<ModelInfo> ModelLifecycleService::availableModels(const QString& provider) const {
    if (provider.isEmpty()) return m_models;
    QList<ModelInfo> result;
    for (const auto& m : m_models) {
        if (m.provider == provider) result.append(m);
    }
    return result;
}

bool ModelLifecycleService::isDeprecated(const QString& provider, const QString& model) const {
    return modelInfo(provider, model).status == ModelStatus::Deprecated;
}

bool ModelLifecycleService::isExperimental(const QString& provider, const QString& model) const {
    auto status = modelInfo(provider, model).status;
    return status == ModelStatus::Alpha || status == ModelStatus::Beta;
}

void ModelLifecycleService::registerModel(const ModelInfo& info) {
    m_models.append(info);
}

QList<ModelInfo> ModelLifecycleService::deprecatedModels() const {
    QList<ModelInfo> result;
    for (const auto& m : m_models) {
        if (m.status == ModelStatus::Deprecated) result.append(m);
    }
    return result;
}

} // namespace sentinel::core
