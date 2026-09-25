// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/runtime/ToolDescriptor.h"

namespace sentinel::core {

struct PlannedToolInvocation;
class ExternalDirectoryGate;

class AuthorizationResolver final {
public:
    static bool validDescriptor(const ToolDescriptor& descriptor);
    static QList<AuthorizationRequest> resolve(const ToolDescriptor& descriptor,
                                               const PlannedToolInvocation& invocation,
                                               const ExternalDirectoryGate* pathResolver = nullptr,
                                               const QString& workingDirectory = {});
};

QString securityDomainName(SecurityDomain domain);
QString accessModeName(AccessMode access);

} // namespace sentinel::core
