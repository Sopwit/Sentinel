#include "sentinel/core/security/PolicyEvaluator.h"

#include <QRegularExpression>

namespace sentinel::core {

PermissionEffect PolicyEvaluator::evaluate(const QList<PermissionRule>& rules,
                                           const QString& action, const QString& resource) {
    PermissionEffect effect = PermissionEffect::Ask;
    for (const PermissionRule& rule : rules) {
        if (rule.action != action && rule.action != QStringLiteral("*")) continue;
        QString pattern = QRegularExpression::escape(rule.resource);
        pattern.replace(QStringLiteral("\\*"), QStringLiteral(".*"));
        pattern.replace(QStringLiteral("\\?"), QStringLiteral("."));
        if (QRegularExpression(QStringLiteral("^%1$").arg(pattern), QRegularExpression::CaseInsensitiveOption)
                .match(resource).hasMatch()) {
            effect = rule.effect;
        }
    }
    return effect;
}

} // namespace sentinel::core
