// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "sentinel/core/runtime/ToolDescriptor.h"
#include "sentinel/core/runtime/ToolInvocationPlan.h"
#include <QJsonObject>
#include <QList>

namespace sentinel::core {

struct ToolValidationError {
    QString path;
    QString keyword;
    QString message;
};

struct ToolValidationResult {
    bool valid = false;
    QJsonObject normalizedArguments;
    QList<ToolValidationError> errors;
};

// Validates the supported JSON Schema subset. Unknown descriptive keywords are ignored;
// ambiguous composition/ref keywords are rejected as invalid contracts.
class ToolArgumentValidator final {
public:
    static QList<ToolValidationError> validateSchema(const QJsonObject& schema);
    static ToolValidationResult validate(const ToolDescriptor& descriptor,
                                         const QList<ToolInvocationArgument>& arguments);
    static QList<ToolInvocationArgument> toInvocationArguments(const QJsonObject& normalized);
    static QString compactContract(const ToolDescriptor& descriptor);
};

} // namespace sentinel::core
