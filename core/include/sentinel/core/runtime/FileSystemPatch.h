// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "sentinel/core/runtime/IFileSystemService.h"
#include "sentinel/core/runtime/ToolExecution.h"
namespace sentinel::core {
struct PatchTarget {
    QString path;
    QString action;
};

bool inspectPatchTargets(const QString& patch, QList<PatchTarget>& targets, QString& error,
                         const FileSystemOperationContext& context = {});
ToolExecutionResult applyPatchWithFileSystem(const QString& patch, const QString& cwd,
                                             const IFileSystemService& service,
                                             const FileSystemOperationContext& context = {},
                                             const ResourceAuthorizationSnapshot* authorization = nullptr);
}
