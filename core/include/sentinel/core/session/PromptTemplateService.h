// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/session/IPromptTemplateService.h"
#include <QObject>
#include <QList>

namespace sentinel::core {

class PromptTemplateService : public QObject, public IPromptTemplateService {
    Q_OBJECT
public:
    explicit PromptTemplateService(QObject* parent = nullptr);
    ~PromptTemplateService() override;

    void registerTemplate(const PromptTemplate& tmpl) override;
    QString systemPrompt(const QString& provider, const QString& model = {}) const override;
    QString planPrompt(const QString& provider) const override;
    QString buildSwitchPrompt(const QString& provider) const override;
    QList<PromptTemplate> templates() const override;

private:
    QList<PromptTemplate> m_templates;
};

} // namespace sentinel::core
