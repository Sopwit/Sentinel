// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QProcessEnvironment>
#include <QStringList>

namespace sentinel::core {

enum class SandboxEnforcement { Enforced, PartiallyEnforced, Unsupported, Failed };

struct SandboxExecutionPlan {
    QString workingDirectory;
    QStringList readablePaths;
    QStringList writablePaths;
    QString temporaryDirectory;
    bool networkAllowed = false;
    bool restrictedEnvironment = true;
    bool requireEnforcement = true;
    bool forbidDetachedChildren = false;
};

struct SandboxExecutionResult {
    SandboxEnforcement enforcement = SandboxEnforcement::Unsupported;
    QString backend;
    QString failureCategory;
    bool networkDenied = false;
    bool filesystemRestricted = false;
    bool processTreeControlled = false;
};

struct SandboxLaunch {
    QString program;
    QStringList arguments;
    QProcessEnvironment environment;
    SandboxExecutionResult result;
    bool permitted = false;
};

class IProcessSandbox {
public:
    virtual ~IProcessSandbox() = default;
    virtual SandboxLaunch prepare(const SandboxExecutionPlan& plan, const QString& program,
                                  const QStringList& arguments,
                                  const QProcessEnvironment& environment) const = 0;
};

class PlatformProcessSandbox final : public IProcessSandbox {
public:
    PlatformProcessSandbox();
    SandboxLaunch prepare(const SandboxExecutionPlan& plan, const QString& program,
                          const QStringList& arguments,
                          const QProcessEnvironment& environment) const override;
private:
    QString launcher_;
};

QString sandboxEnforcementName(SandboxEnforcement enforcement);

} // namespace sentinel::core
