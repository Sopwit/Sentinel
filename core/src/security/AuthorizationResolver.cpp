// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/security/AuthorizationResolver.h"

#include "sentinel/core/runtime/ToolInvocationPlan.h"
#include "sentinel/core/security/ExternalDirectoryGate.h"

#include <QDir>
#include <QCryptographicHash>
#include <QUrl>

namespace sentinel::core {

bool AuthorizationResolver::validDescriptor(const ToolDescriptor& descriptor) {
    const auto properties = descriptor.inputSchema.value(QStringLiteral("properties")).toObject();
    for (const auto& requirement : descriptor.authorizationRequirements) {
        switch (requirement.domain) {
        case SecurityDomain::FileSystem: case SecurityDomain::Process: case SecurityDomain::Network:
        case SecurityDomain::Clipboard: case SecurityDomain::Application: case SecurityDomain::System:
        case SecurityDomain::Memory: case SecurityDomain::Conversation: case SecurityDomain::Audio:
        case SecurityDomain::Browser: case SecurityDomain::Agent:
        case SecurityDomain::ExternalService: break;
        default: return false;
        }
        switch (requirement.access) {
        case AccessMode::Read: case AccessMode::Write: case AccessMode::Delete:
        case AccessMode::Execute: case AccessMode::Control: case AccessMode::Invoke: break;
        default: return false;
        }
        switch (requirement.resourceKind) {
        case AuthorizationResourceKind::None: break;
        case AuthorizationResourceKind::Argument:
        case AuthorizationResourceKind::ArgumentDigest:
        case AuthorizationResourceKind::FileSystemPath:
        case AuthorizationResourceKind::Host:
            if (requirement.resourceArgument.isEmpty() && requirement.staticResource.isEmpty())
                return false;
            if (!requirement.resourceArgument.isEmpty() &&
                !properties.contains(requirement.resourceArgument))
                return false;
            break;
        case AuthorizationResourceKind::Provider:
            if (requirement.staticResource.isEmpty() && descriptor.providerId.isEmpty())
                return false;
            break;
        default: return false;
        }
        if (requirement.domain == SecurityDomain::FileSystem &&
            requirement.resourceKind != AuthorizationResourceKind::FileSystemPath &&
            !(descriptor.source == ToolSource::BuiltIn &&
              descriptor.id == QLatin1String("apply-patch") &&
              requirement.resourceKind == AuthorizationResourceKind::None))
            return false;
    }
    return true;
}

QList<AuthorizationRequest> AuthorizationResolver::resolve(
    const ToolDescriptor& descriptor, const PlannedToolInvocation& invocation,
    const ExternalDirectoryGate* pathResolver, const QString& workingDirectory) {
    if (invocation.resourceSnapshot)
        return invocation.resourceSnapshot->requests;
    QList<AuthorizationRequest> requests;
    for (const auto& requirement : descriptor.authorizationRequirements) {
        QString resource = requirement.staticResource;
        if (!requirement.resourceArgument.isEmpty()) {
            for (const auto& argument : invocation.arguments) {
                if (argument.id == requirement.resourceArgument) {
                    resource = argument.value.trimmed();
                    break;
                }
            }
        }
        if (requirement.resourceKind == AuthorizationResourceKind::ArgumentDigest &&
            !resource.isEmpty()) {
            resource = QString::fromLatin1(QCryptographicHash::hash(
                resource.toUtf8(), QCryptographicHash::Sha256).toHex());
        } else if (requirement.resourceKind == AuthorizationResourceKind::FileSystemPath &&
            !resource.isEmpty()) {
            resource = pathResolver
                           ? pathResolver->resolvePath(resource, workingDirectory.isEmpty()
                                                                  ? QDir::currentPath()
                                                                  : workingDirectory)
                           : QDir::cleanPath(QDir(workingDirectory.isEmpty()
                                                      ? QDir::currentPath()
                                                      : workingDirectory)
                                                 .absoluteFilePath(resource));
        } else if (requirement.resourceKind == AuthorizationResourceKind::Provider &&
                   resource.isEmpty()) {
            resource = descriptor.providerId;
        } else if (requirement.resourceKind == AuthorizationResourceKind::Host &&
                   !resource.isEmpty()) {
            const QUrl url(resource);
            resource = url.host().toLower();
        }
        requests.append({requirement.domain, requirement.access, resource,
                         descriptor.riskLevel, descriptor.providerId, descriptor.id,
                         requirement.resourceKind});
    }
    if (requests.isEmpty() &&
        (descriptor.source == ToolSource::MCP || descriptor.source == ToolSource::Plugin)) {
        requests.append({SecurityDomain::ExternalService, AccessMode::Invoke,
                         descriptor.providerId, descriptor.riskLevel, descriptor.providerId,
                         descriptor.id, AuthorizationResourceKind::Provider});
    }
    return requests;
}

QString securityDomainName(SecurityDomain domain) {
    switch (domain) {
    case SecurityDomain::FileSystem: return QStringLiteral("File system");
    case SecurityDomain::Process: return QStringLiteral("Process");
    case SecurityDomain::Network: return QStringLiteral("Network");
    case SecurityDomain::Clipboard: return QStringLiteral("Clipboard");
    case SecurityDomain::Application: return QStringLiteral("Application");
    case SecurityDomain::System: return QStringLiteral("System");
    case SecurityDomain::Memory: return QStringLiteral("Memory");
    case SecurityDomain::Conversation: return QStringLiteral("Conversation");
    case SecurityDomain::Audio: return QStringLiteral("Audio");
    case SecurityDomain::Browser: return QStringLiteral("Browser");
    case SecurityDomain::Agent: return QStringLiteral("Agent");
    case SecurityDomain::ExternalService: return QStringLiteral("External service");
    }
    return QStringLiteral("Unknown");
}

QString accessModeName(AccessMode access) {
    switch (access) {
    case AccessMode::Read: return QStringLiteral("Read");
    case AccessMode::Write: return QStringLiteral("Write");
    case AccessMode::Delete: return QStringLiteral("Delete");
    case AccessMode::Execute: return QStringLiteral("Execute");
    case AccessMode::Control: return QStringLiteral("Control");
    case AccessMode::Invoke: return QStringLiteral("Invoke");
    }
    return QStringLiteral("Unknown");
}

} // namespace sentinel::core
