// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/session/PromptTemplateService.h"

namespace sentinel::core {

PromptTemplateService::PromptTemplateService(QObject* parent) : QObject(parent) {
    PromptTemplate defaultTmpl;
    defaultTmpl.provider = "default";
    defaultTmpl.systemPrompt = "You are a helpful AI assistant.";
    m_templates.append(defaultTmpl);
}

PromptTemplateService::~PromptTemplateService() = default;

void PromptTemplateService::registerTemplate(const PromptTemplate& tmpl) {
    for (int i = 0; i < m_templates.size(); ++i) {
        if (m_templates[i].provider == tmpl.provider && m_templates[i].modelPattern == tmpl.modelPattern) {
            m_templates[i] = tmpl;
            return;
        }
    }
    m_templates.append(tmpl);
}

QString PromptTemplateService::systemPrompt(const QString& provider, const QString& model) const {
    for (const auto& tmpl : m_templates) {
        if (tmpl.provider == provider) {
            if (model.isEmpty() || tmpl.modelPattern.isEmpty() || model.contains(tmpl.modelPattern)) {
                return tmpl.systemPrompt;
            }
        }
    }
    return m_templates.isEmpty() ? "You are a helpful AI assistant." : m_templates.first().systemPrompt;
}

QString PromptTemplateService::planPrompt(const QString& provider) const {
    for (const auto& tmpl : m_templates) {
        if (tmpl.provider == provider && !tmpl.planPrompt.isEmpty()) {
            return tmpl.planPrompt;
        }
    }
    return "Create a structured implementation plan before execution.";
}

QString PromptTemplateService::buildSwitchPrompt(const QString& provider) const {
    for (const auto& tmpl : m_templates) {
        if (tmpl.provider == provider && !tmpl.buildSwitchPrompt.isEmpty()) {
            return tmpl.buildSwitchPrompt;
        }
    }
    return "Switching to build mode. Execute the plan step by step.";
}

QList<PromptTemplate> PromptTemplateService::templates() const { return m_templates; }

} // namespace sentinel::core
