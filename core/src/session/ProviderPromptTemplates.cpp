#include "sentinel/core/session/ProviderPromptTemplates.h"

namespace sentinel::core {

QString ProviderPromptTemplates::systemPrompt(const QString& provider, const QString& basePrompt) {
    const QString normalized = provider.toLower();
    if (normalized.contains(QStringLiteral("anthropic"))) {
        return QStringLiteral("Be precise, transparent about uncertainty, and preserve tool safety.\n\n") + basePrompt;
    }
    if (normalized.contains(QStringLiteral("gemini"))) {
        return QStringLiteral("Organize the response clearly and distinguish facts from assumptions.\n\n") + basePrompt;
    }
    if (normalized.contains(QStringLiteral("gpt")) || normalized.contains(QStringLiteral("openai"))) {
        return QStringLiteral("Follow the requested output format exactly and use tools only when needed.\n\n") + basePrompt;
    }
    return basePrompt;
}

} // namespace sentinel::core
