// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/session/WorktreeService.h"
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QUuid>

namespace {
constexpr int kGitTimeoutMs = 5000;
constexpr qsizetype kMaxGitOutput = 64 * 1024;

struct GitResult {
    bool success{false};
    QString output;
    QString error;
};

GitResult runGit(const QString& directory, const QStringList& arguments) {
    QProcess process;
    process.setProgram(QStringLiteral("git"));
    process.setArguments(arguments);
    if (!directory.isEmpty())
        process.setWorkingDirectory(directory);
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start();
    if (!process.waitForStarted(kGitTimeoutMs)) {
        return {false, {}, QStringLiteral("Unable to start git: %1").arg(process.errorString())};
    }
    if (!process.waitForFinished(kGitTimeoutMs)) {
        process.kill();
        process.waitForFinished(1000);
        return {false, {}, QStringLiteral("git command timed out")};
    }
    QByteArray output = process.readAll();
    output.truncate(kMaxGitOutput);
    const bool success = process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
    return {success, QString::fromLocal8Bit(output),
            success ? QString() : QString::fromLocal8Bit(output).trimmed()};
}
} // namespace

namespace sentinel::core {

WorktreeService::WorktreeService(QObject* parent) : QObject(parent) {
    m_worktreeBasePath =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/worktrees";
    QDir().mkpath(m_worktreeBasePath);
}

WorktreeService::~WorktreeService() = default;

Worktree WorktreeService::create(const QString& branch) {
    if (m_repositoryPath.isEmpty()) {
        setError(QStringLiteral("No Git repository has been configured"));
        return {};
    }
    return createWorktree(m_repositoryPath, branch, generateName());
}

bool WorktreeService::remove(const QString& name) {
    if (!m_repositoryPath.isEmpty()) {
        const auto worktree = find(name);
        return worktree.has_value() && removeWorktree(m_repositoryPath, worktree->path);
    }
    for (int i = 0; i < m_worktrees.size(); ++i) {
        if (m_worktrees[i].name == name) {
            m_worktrees.removeAt(i);
            return true;
        }
    }
    return false;
}

bool WorktreeService::reset(const QString& name) {
    for (auto& wt : m_worktrees) {
        if (wt.name == name) {
            const GitResult result =
                runGit(wt.path,
                       {QStringLiteral("reset"), QStringLiteral("--hard"), QStringLiteral("HEAD")});
            if (!result.success)
                setError(result.error.isEmpty() ? result.output : result.error);
            return result.success;
        }
    }
    return false;
}

QList<Worktree> WorktreeService::list() const {
    if (m_repositoryPath.isEmpty())
        return m_worktrees;
    const GitResult result =
        runGit(m_repositoryPath,
               {QStringLiteral("worktree"), QStringLiteral("list"), QStringLiteral("--porcelain")});
    if (!result.success) {
        setError(result.error);
        return {};
    }
    QList<Worktree> worktrees;
    const QStringList records = result.output.split(QStringLiteral("\n\n"), Qt::SkipEmptyParts);
    for (const QString& record : records) {
        const QStringList lines = record.split('\n', Qt::SkipEmptyParts);
        QString path;
        QString branch;
        for (const QString& line : lines) {
            if (line.startsWith(QStringLiteral("worktree ")))
                path = line.mid(9).trimmed();
            if (line.startsWith(QStringLiteral("branch refs/heads/")))
                branch = line.mid(18).trimmed();
        }
        if (!path.isEmpty())
            worktrees.append(Worktree{QFileInfo(path).fileName(), path, branch, true});
    }
    return worktrees;
}

std::optional<Worktree> WorktreeService::find(const QString& name) const {
    const QList<Worktree> worktrees = m_repositoryPath.isEmpty() ? m_worktrees : list();
    for (const auto& wt : worktrees) {
        if (wt.name == name)
            return wt;
    }
    return std::nullopt;
}

QString WorktreeService::worktreePath() const {
    return m_worktreeBasePath;
}

void WorktreeService::setRepositoryPath(const QString& path) {
    if (path.isEmpty()) {
        m_repositoryPath.clear();
        return;
    }
    const auto repository = discoverRepository(path);
    if (repository)
        m_repositoryPath = repository->path;
    else
        m_repositoryPath.clear();
}

QString WorktreeService::repositoryPath() const {
    return m_repositoryPath;
}

std::optional<GitRepository> WorktreeService::discoverRepository(const QString& path) const {
    if (!validRepositoryPath(path)) {
        setError(QStringLiteral("Repository path must be an existing directory"));
        return std::nullopt;
    }
    const QString absolutePath = QFileInfo(path).absoluteFilePath();
    const GitResult result =
        runGit(absolutePath, {QStringLiteral("rev-parse"), QStringLiteral("--show-toplevel")});
    if (!result.success || result.output.trimmed().isEmpty()) {
        setError(result.error.isEmpty() ? QStringLiteral("Path is not a Git repository")
                                        : result.error);
        return std::nullopt;
    }
    const QString root = QDir::cleanPath(result.output.trimmed());
    const GitResult gitDir =
        runGit(root, {QStringLiteral("rev-parse"), QStringLiteral("--git-dir")});
    return GitRepository{root,
                         gitDir.success ? QDir::cleanPath(gitDir.output.trimmed()) : QString()};
}

std::optional<GitStatus> WorktreeService::status(const QString& path) const {
    if (!validRepositoryPath(path)) {
        setError(QStringLiteral("Status path must be an existing directory"));
        return std::nullopt;
    }
    const GitResult result =
        runGit(QFileInfo(path).absoluteFilePath(),
               {QStringLiteral("status"), QStringLiteral("--short"), QStringLiteral("--branch")});
    if (!result.success) {
        setError(result.error);
        return std::nullopt;
    }
    const QStringList lines = result.output.split('\n', Qt::SkipEmptyParts);
    if (lines.isEmpty() || !lines.first().startsWith(QStringLiteral("## "))) {
        setError(QStringLiteral("Unable to read Git status"));
        return std::nullopt;
    }
    return GitStatus{lines.first().mid(3).trimmed(), lines.size() == 1, result.output.trimmed()};
}

QString WorktreeService::currentBranch(const QString& path) const {
    if (!validRepositoryPath(path)) {
        setError(QStringLiteral("Branch path must be an existing directory"));
        return {};
    }
    const GitResult result =
        runGit(path, {QStringLiteral("branch"), QStringLiteral("--show-current")});
    if (!result.success) {
        setError(result.error);
        return {};
    }
    return result.output.trimmed();
}

std::optional<GitDiffSummary> WorktreeService::diffSummary(const QString& path) const {
    if (!validRepositoryPath(path)) {
        setError(QStringLiteral("Diff path must be an existing directory"));
        return std::nullopt;
    }
    const GitResult result = runGit(
        path, {QStringLiteral("diff"), QStringLiteral("--shortstat"), QStringLiteral("HEAD")});
    if (!result.success) {
        setError(result.error);
        return std::nullopt;
    }
    const QString summary = result.output.trimmed();
    const QRegularExpression expression(QStringLiteral(
        "(\\d+) files? changed(?:, (\\d+) insertions?\\(\\+\\))?(?:, (\\d+) deletions?\\(-\\))?"));
    const auto match = expression.match(summary);
    return GitDiffSummary{match.hasMatch() ? match.captured(1).toInt() : 0,
                          match.hasMatch() ? match.captured(2).toInt() : 0,
                          match.hasMatch() ? match.captured(3).toInt() : 0, summary};
}

Worktree WorktreeService::createWorktree(const QString& repositoryPath, const QString& branch,
                                         const QString& name) {
    Worktree result;
    const auto repository = discoverRepository(repositoryPath);
    const QString worktreeName = name.isEmpty() ? generateName() : name;
    if (!repository || !validWorktreeName(worktreeName)) {
        if (repository)
            setError(QStringLiteral("Invalid worktree name"));
        return result;
    }
    const QString target = QDir(m_worktreeBasePath).filePath(worktreeName);
    if (QFileInfo::exists(target)) {
        setError(QStringLiteral("Worktree path already exists"));
        return result;
    }
    QStringList args{QStringLiteral("worktree"), QStringLiteral("add")};
    if (branch.isEmpty())
        args << QStringLiteral("--detach");
    args << QStringLiteral("--") << target;
    if (branch.isEmpty())
        args << QStringLiteral("HEAD");
    else
        args << branch;
    const GitResult command = runGit(repository->path, args);
    if (!command.success) {
        setError(command.error.isEmpty() ? command.output : command.error);
        return result;
    }
    result =
        Worktree{worktreeName, target, branch.isEmpty() ? currentBranch(target) : branch, true};
    m_worktrees.append(result);
    return result;
}

bool WorktreeService::removeWorktree(const QString& repositoryPath, const QString& worktreePath) {
    const auto repository = discoverRepository(repositoryPath);
    const QString target = QDir::cleanPath(QFileInfo(worktreePath).absoluteFilePath());
    const QString base = QDir(m_worktreeBasePath).absolutePath();
    if (!repository || !target.startsWith(base + QDir::separator())) {
        setError(QStringLiteral("Worktree path is outside Sentinel's worktree directory"));
        return false;
    }
    const GitResult result =
        runGit(repository->path, {QStringLiteral("worktree"), QStringLiteral("remove"),
                                  QStringLiteral("--force"), QStringLiteral("--"), target});
    if (!result.success)
        setError(result.error.isEmpty() ? result.output : result.error);
    if (result.success) {
        for (int i = m_worktrees.size() - 1; i >= 0; --i)
            if (m_worktrees[i].path == target)
                m_worktrees.removeAt(i);
    }
    return result.success;
}

QString WorktreeService::lastError() const {
    return m_lastError;
}

bool WorktreeService::validRepositoryPath(const QString& path) const {
    return !path.isEmpty() && QFileInfo(path).isDir();
}

bool WorktreeService::validWorktreeName(const QString& name) const {
    return !name.isEmpty() && name != QStringLiteral(".") && name != QStringLiteral("..") &&
           !name.contains('/') && !name.contains('\\') && !name.contains(QChar::Null);
}

void WorktreeService::setError(const QString& error) const {
    m_lastError = error;
}

QString WorktreeService::generateName() const {
    return "wt-" + QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
}

} // namespace sentinel::core
