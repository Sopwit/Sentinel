// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "sentinel/core/runtime/ToolInvocationPlan.h"

namespace sentinel::core {

class ExternalDirectoryGate;
class PermissionService;

struct ResourceAuthorizationResult {
    ResourceAuthorizationSnapshot snapshot;
    FileSystemFailure failure = FileSystemFailure::None;
    QString resource;
    QString reason;
    bool ok() const { return failure == FileSystemFailure::None; }
};

class ResourceAuthorizationResolver final {
public:
    static ResourceAuthorizationResult resolve(const ToolDescriptor& descriptor,
                                               const PlannedToolInvocation& invocation,
                                               const QString& workingDirectory,
                                               const ExternalDirectoryGate* gate);
    static ResourceAuthorizationResult authorize(const ResourceAuthorizationSnapshot& snapshot,
                                                 const ExternalDirectoryGate* gate,
                                                 const PermissionService* permissions,
                                                 const QString& sessionId);
};

} // namespace sentinel::core
