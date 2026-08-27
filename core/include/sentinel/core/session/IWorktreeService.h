// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QList>
#include <QString>

#include <optional>

namespace sentinel::core {

struct Worktree {
    QString name;
    QString path;
    QString branch;
    bool active{false};
};

struct GitRepository {
    QString path;
    QString gitDirectory;
};

struct GitStatus {
    QString branch;
    bool clean{false};
    QString summary;
};

struct GitDiffSummary {
    int filesChanged{0};
    int insertions{0};
    int deletions{0};
    QString summary;
};

class IWorktreeService {
public:
    virtual ~IWorktreeService() = default;

    virtual Worktree create(const QString& branch = {}) = 0;
    virtual bool remove(const QString& name) = 0;
    virtual bool reset(const QString& name) = 0;
    virtual QList<Worktree> list() const = 0;
    virtual std::optional<Worktree> find(const QString& name) const = 0;
    virtual QString worktreePath() const = 0;

    virtual void setRepositoryPath(const QString& path) = 0;
    virtual QString repositoryPath() const = 0;
    virtual std::optional<GitRepository> discoverRepository(const QString& path) const = 0;
    virtual std::optional<GitStatus> status(const QString& path) const = 0;
    virtual QString currentBranch(const QString& path) const = 0;
    virtual std::optional<GitDiffSummary> diffSummary(const QString& path) const = 0;
    virtual Worktree createWorktree(const QString& repositoryPath, const QString& branch = {},
                                    const QString& name = {}) = 0;
    virtual bool removeWorktree(const QString& repositoryPath, const QString& worktreePath) = 0;
    virtual QString lastError() const = 0;
};

} // namespace sentinel::core
