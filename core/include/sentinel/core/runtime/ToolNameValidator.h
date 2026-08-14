#pragma once

#include <QRegularExpression>
#include <QString>

namespace sentinel::core {

class ToolNameValidator final {
public:
    static bool isValid(const QString& name) {
        static const QRegularExpression pattern(QStringLiteral("^[A-Za-z][A-Za-z0-9_-]{0,63}$"));
        return pattern.match(name).hasMatch();
    }
};

} // namespace sentinel::core
