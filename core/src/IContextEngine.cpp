#include "sentinel/core/IContextEngine.h"

namespace sentinel::core {

QString BasicContextEngine::buildContextForPrompt(const QString& prompt) const {
    return prompt.trimmed();
}

} // namespace sentinel::core
