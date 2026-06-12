#include "sentinel/core/WorkspaceService.h"

#include <QtTest>

using sentinel::core::WorkspaceService;

class WorkspaceServiceTest final : public QObject {
    Q_OBJECT

private slots:
    void exposesMetadataOnlyPlaceholder();
    void reportsDisabledWorkspaceReadiness();
    void normalizesUnknownSelection();
};

void WorkspaceServiceTest::exposesMetadataOnlyPlaceholder() {
    const WorkspaceService service;
    const auto workspaces = service.availableWorkspaces();

    QCOMPARE(workspaces.size(), 1);
    QCOMPARE(workspaces.first().id, QStringLiteral("local-placeholder"));
    QCOMPARE(workspaces.first().accessState, QStringLiteral("Access not enabled"));
    QVERIFY(workspaces.first().rootSummary.contains(QStringLiteral("No folder selected")));
}

void WorkspaceServiceTest::reportsDisabledWorkspaceReadiness() {
    const WorkspaceService service;
    const auto readiness = service.readiness(QStringLiteral("local-placeholder"));

    QCOMPARE(readiness.status, QStringLiteral("Not enabled"));
    QVERIFY(readiness.summary.contains(QStringLiteral("Workspace access is not enabled yet")));
    QVERIFY(readiness.checks.contains(QStringLiteral("Filesystem scanning: disabled")));
    QVERIFY(readiness.checks.contains(QStringLiteral("File reading: disabled")));
    QVERIFY(readiness.boundaryDiagnostics.contains(
        QStringLiteral("Runtime boundary: no subprocesses, no tools, no recursive scanning")));
}

void WorkspaceServiceTest::normalizesUnknownSelection() {
    const WorkspaceService service;

    QCOMPARE(service.normalizedWorkspaceId(QStringLiteral("unknown")),
             QStringLiteral("local-placeholder"));
    QCOMPARE(service.selectedWorkspace(QStringLiteral("unknown")).name,
             QStringLiteral("Local Workspace"));
}

QTEST_MAIN(WorkspaceServiceTest)

#include "test_workspace_service.moc"
