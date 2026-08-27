// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/session/IWorktreeService.h"
#include <QList>
#include <QObject>
#include <QStandardPaths>

namespace sentinel::core {

class WorktreeService : public QObject, public IWorktreeService {
    Q_OBJECT
public:
    explicit WorktreeService(QObject* parent = nullptr);
    ~WorktreeService() override;

    Worktree create(const QString& branch = {}) override;
    bool remove(const QString& name) override;
    bool reset(const QString& name) override;
    QList<Worktree> list() const override;
    std::optional<Worktree> find(const QString& name) const override;
    QString worktreePath() const override;
    void setRepositoryPath(const QString& path) override;
    QString repositoryPath() const override;
    std::optional<GitRepository> discoverRepository(const QString& path) const override;
    std::optional<GitStatus> status(const QString& path) const override;
    QString currentBranch(const QString& path) const override;
    std::optional<GitDiffSummary> diffSummary(const QString& path) const override;
    Worktree createWorktree(const QString& repositoryPath, const QString& branch = {},
                            const QString& name = {}) override;
    bool removeWorktree(const QString& repositoryPath, const QString& worktreePath) override;
    QString lastError() const override;

private:
    QString generateName() const;
    bool validRepositoryPath(const QString& path) const;
    bool validWorktreeName(const QString& name) const;
    void setError(const QString& error) const;
    QString m_worktreeBasePath;
    QString m_repositoryPath;
    mutable QString m_lastError;
    QList<Worktree> m_worktrees;
};

} // namespace sentinel::core
