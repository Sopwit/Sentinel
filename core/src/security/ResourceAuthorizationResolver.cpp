// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#include "sentinel/core/security/ResourceAuthorizationResolver.h"

#include "sentinel/core/runtime/FileSystemPatch.h"
#include "sentinel/core/security/AuthorizationResolver.h"
#include "sentinel/core/security/ExternalDirectoryGate.h"
#include "sentinel/core/security/PathGuard.h"
#include "sentinel/core/security/PermissionService.h"

#include <QDir>
#include <algorithm>

namespace sentinel::core {
namespace {
QString argumentValue(const PlannedToolInvocation& invocation, const QString& name) {
    for (const auto& argument : invocation.arguments)
        if (argument.id == name) return argument.value;
    return {};
}

bool appendFile(ResourceAuthorizationResult& result, const ToolDescriptor& descriptor,
                const QString& argument, const QString& raw, AccessMode access,
                const QString& action, const ExternalDirectoryGate* gate) {
    QString path = raw.trimmed();
    if (path.isEmpty()) {
        result.failure = FileSystemFailure::InvalidPath;
        result.reason = QStringLiteral("Filesystem resource path is empty.");
        return false;
    }
    if (path == QLatin1String("~"))
        path = QDir::homePath();
    else if (path.startsWith(QLatin1String("~/")))
        path = QDir::home().absoluteFilePath(path.mid(2));
    const auto canonical = gate
        ? gate->resolvePath(path, result.snapshot.workingDirectory)
        : PathGuard::canonicalPath(QDir(result.snapshot.workingDirectory).absoluteFilePath(path));
    if (canonical.isEmpty()) {
        result.failure = FileSystemFailure::InvalidPath;
        result.resource = path;
        result.reason = QStringLiteral("Filesystem resource could not be resolved.");
        return false;
    }
    const FileSystemAccess filesystemAccess = access == AccessMode::Read
        ? FileSystemAccess::Read : FileSystemAccess::Write;
    result.snapshot.files.append({argument, action, access,
                                  {canonical, path, filesystemAccess}});
    result.snapshot.requests.append({SecurityDomain::FileSystem, access, canonical,
                                     descriptor.riskLevel, descriptor.providerId, descriptor.id});
    return true;
}
} // namespace

ResourceAuthorizationResult ResourceAuthorizationResolver::resolve(
    const ToolDescriptor& descriptor, const PlannedToolInvocation& invocation,
    const QString& workingDirectory, const ExternalDirectoryGate* gate) {
    ResourceAuthorizationResult result;
    result.snapshot.normalizedArguments = invocation.arguments;
    result.snapshot.workingDirectory = PathGuard::canonicalPath(workingDirectory);
    if (result.snapshot.workingDirectory.isEmpty()) {
        result.failure = FileSystemFailure::InvalidPath;
        result.reason = QStringLiteral("Working directory could not be resolved.");
        return result;
    }
    if (descriptor.source == ToolSource::BuiltIn &&
        descriptor.id == QLatin1String("apply-patch")) {
        QList<PatchTarget> targets;
        QString error;
        if (!inspectPatchTargets(argumentValue(invocation, QStringLiteral("patch")), targets,
                                 error, {invocation.cancellation, invocation.toolCancellation})) {
            result.failure = FileSystemFailure::InvalidPath;
            result.reason = error;
            return result;
        }
        for (const auto& target : targets) {
            if (target.action == QLatin1String("update") &&
                !appendFile(result, descriptor, target.path, target.path, AccessMode::Read,
                            target.action, gate)) return result;
            const auto access = target.action == QLatin1String("delete")
                ? AccessMode::Delete : AccessMode::Write;
            if (!appendFile(result, descriptor, target.path, target.path, access,
                            target.action, gate)) return result;
        }
        return result;
    }

    auto semanticInvocation = invocation;
    semanticInvocation.resourceSnapshot.reset();
    for (const auto& request : AuthorizationResolver::resolve(
             descriptor, semanticInvocation, gate, result.snapshot.workingDirectory))
        if (request.domain != SecurityDomain::FileSystem)
            result.snapshot.requests.append(request);

    for (const auto& requirement : descriptor.authorizationRequirements) {
        if (requirement.domain != SecurityDomain::FileSystem) continue;
        if (requirement.resourceKind != AuthorizationResourceKind::FileSystemPath) {
            result.failure = FileSystemFailure::InvalidPath;
            result.reason = QStringLiteral("Filesystem authorization contract has no path binding.");
            return result;
        }
        QString path = requirement.resourceArgument.isEmpty()
            ? requirement.staticResource : argumentValue(invocation, requirement.resourceArgument);
        if (path.trimmed().isEmpty() && requirement.resourceArgument == QLatin1String("path") &&
            (descriptor.id == QLatin1String("grep") || descriptor.id == QLatin1String("glob")))
            path = result.snapshot.workingDirectory;
        if (!appendFile(result, descriptor, requirement.resourceArgument, path,
                        requirement.access, {}, gate)) return result;
    }
    return result;
}

ResourceAuthorizationResult ResourceAuthorizationResolver::authorize(
    const ResourceAuthorizationSnapshot& snapshot, const ExternalDirectoryGate* gate,
    const PermissionService* permissions, const QString& sessionId) {
    ResourceAuthorizationResult result;
    result.snapshot = snapshot;
    QtFileSystemService service(gate);
    for (const auto& resource : snapshot.files) {
        if (gate && !PathGuard::contains(snapshot.workingDirectory,
                                         resource.path.canonicalPath)) {
            const bool granted = permissions && std::any_of(
                snapshot.requests.cbegin(), snapshot.requests.cend(),
                [&](const AuthorizationRequest& request) {
                    return request.domain == SecurityDomain::FileSystem &&
                           request.access == resource.access &&
                           request.resource == resource.path.canonicalPath &&
                           permissions->evaluateAuthorization(request, sessionId) ==
                               PermissionEffect::Allow;
                });
            if (!granted) {
                result.failure = FileSystemFailure::PermissionDenied;
                result.resource = resource.path.displayPath;
                result.reason = QStringLiteral("Filesystem access denied for %1.")
                                    .arg(resource.path.displayPath);
                return result;
            }
        }
        const auto checked = service.resolve(resource.path.canonicalPath,
                                             snapshot.workingDirectory,
                                             resource.path.access);
        if (!checked.ok() || checked.value->canonicalPath != resource.path.canonicalPath) {
            result.failure = checked.failure == FileSystemFailure::None
                ? FileSystemFailure::PermissionDenied : checked.failure;
            result.resource = resource.path.displayPath;
            result.reason = QStringLiteral("Filesystem access denied for %1.")
                                .arg(resource.path.displayPath);
            return result;
        }
    }
    result.snapshot.authorized = true;
    return result;
}

} // namespace sentinel::core
