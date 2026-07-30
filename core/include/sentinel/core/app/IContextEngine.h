#pragma once

#include <QString>

namespace sentinel::core {

class IContextEngine {
public:
    virtual ~IContextEngine() = default;

    virtual QString buildContextForPrompt(const QString& prompt) const = 0;
};

class BasicContextEngine final : public IContextEngine {
public:
    QString buildContextForPrompt(const QString& prompt) const override;
};

} // namespace sentinel::core
