#pragma once

#include <QString>

namespace sentinel::core {

class ProviderPromptTemplates final {
public:
    static QString systemPrompt(const QString& provider, const QString& basePrompt);
};

} // namespace sentinel::core
