// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/session/WorktreeService.h"

#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QTemporaryDir>
#include <QtTest>

using sentinel::core::WorktreeService;

class WorktreeServiceTest final : public QObject {
    Q_OBJECT
private slots:
    void discoversAndReportsRepository();
    void createsAndRemovesRealWorktree();
    void rejectsUnsafeWorktreePath();
};

static bool runGit(const QString& directory, const QStringList& args) {
    QProcess process;
    process.setProgram(QStringLiteral("git"));
    process.setArguments(args);
    process.setWorkingDirectory(directory);
    process.start();
    return process.waitForFinished(5000) && process.exitCode() == 0;
}

void WorktreeServiceTest::discoversAndReportsRepository() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QVERIFY(runGit(dir.path(), {QStringLiteral("init"), QStringLiteral("-b"), QStringLiteral("main")}));
    QFile file(QDir(dir.path()).filePath(QStringLiteral("tracked.txt")));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("initial\n");
    file.close();
    QVERIFY(runGit(dir.path(), {QStringLiteral("add"), QStringLiteral("tracked.txt")}));
    QVERIFY(runGit(dir.path(), {QStringLiteral("-c"), QStringLiteral("user.name=Sentinel"), QStringLiteral("-c"), QStringLiteral("user.email=sentinel@example.invalid"), QStringLiteral("commit"), QStringLiteral("-m"), QStringLiteral("initial")}));

    WorktreeService service;
    const auto repository = service.discoverRepository(dir.path());
    QVERIFY(repository.has_value());
    QCOMPARE(service.currentBranch(dir.path()), QStringLiteral("main"));
    const auto status = service.status(dir.path());
    QVERIFY(status.has_value());
    QVERIFY(status->clean);

    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Append));
    file.write("changed\n");
    file.close();
    const auto diff = service.diffSummary(dir.path());
    QVERIFY(diff.has_value());
    QCOMPARE(diff->filesChanged, 1);
    QVERIFY(diff->insertions >= 1);
}

void WorktreeServiceTest::createsAndRemovesRealWorktree() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QVERIFY(runGit(dir.path(), {QStringLiteral("init")}));
    QFile file(QDir(dir.path()).filePath(QStringLiteral("tracked.txt")));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("initial\n");
    file.close();
    QVERIFY(runGit(dir.path(), {QStringLiteral("add"), QStringLiteral("tracked.txt")}));
    QVERIFY(runGit(dir.path(), {QStringLiteral("-c"), QStringLiteral("user.name=Sentinel"), QStringLiteral("-c"), QStringLiteral("user.email=sentinel@example.invalid"), QStringLiteral("commit"), QStringLiteral("-m"), QStringLiteral("initial")}));

    WorktreeService service;
    service.setRepositoryPath(dir.path());
    const auto worktree = service.create(QStringLiteral("HEAD"));
    QVERIFY(!worktree.path.isEmpty());
    QVERIFY(QFileInfo::exists(worktree.path));
    QVERIFY(service.remove(worktree.name));
    QVERIFY(!QFileInfo::exists(worktree.path));
}

void WorktreeServiceTest::rejectsUnsafeWorktreePath() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    WorktreeService service;
    QVERIFY(!service.createWorktree(dir.path(), QStringLiteral("HEAD"), QStringLiteral("../outside")).active);
    QVERIFY(!service.lastError().isEmpty());
}

QTEST_MAIN(WorktreeServiceTest)
#include "test_worktree_service.moc"
