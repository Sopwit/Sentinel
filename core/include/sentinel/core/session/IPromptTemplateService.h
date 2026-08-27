// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QMap>
#include <QString>

namespace sentinel::core {

struct PromptTemplate {
    QString provider;
    QString modelPattern;
    QString systemPrompt;
    QString planPrompt;
    QString buildSwitchPrompt;
};

class IPromptTemplateService {
public:
    virtual ~IPromptTemplateService() = default;

    virtual void registerTemplate(const PromptTemplate& tmpl) = 0;
    virtual QString systemPrompt(const QString& provider, const QString& model = {}) const = 0;
    virtual QString planPrompt(const QString& provider) const = 0;
    virtual QString buildSwitchPrompt(const QString& provider) const = 0;
    virtual QList<PromptTemplate> templates() const = 0;
};

} // namespace sentinel::core
