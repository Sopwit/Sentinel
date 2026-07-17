#include "sentinel/core/WorkspaceService.h"

#include <QtTest>

using sentinel::core::WorkspaceService;

class WorkspaceServiceTest final : public QObject {
    Q_OBJECT

private slots:
    void exposesBuiltInTemplates();
    void reportsWorkspaceReadiness();
    void exposesPermissionPostureAndActions();
    void normalizesUnknownSelection();
    void supportsWorkspaceLifecycle();
};

void WorkspaceServiceTest::exposesBuiltInTemplates() {
    const WorkspaceService service;
    const auto workspaces = service.availableWorkspaces();

    QCOMPARE(workspaces.size(), 5);
    QCOMPARE(workspaces.first().id, QStringLiteral("personal"));
    QVERIFY(service.builtInTemplateNames().contains(QStringLiteral("Coding")));
    QVERIFY(service.builtInTemplateNames().contains(QStringLiteral("Research")));
    QVERIFY(workspaces.first().rootSummary.contains(QStringLiteral("explicitly by the user")));
}

void WorkspaceServiceTest::reportsWorkspaceReadiness() {
    const WorkspaceService service;
    const auto readiness = service.readiness(QStringLiteral("personal"));

    QCOMPARE(readiness.status, QStringLiteral("Ready"));
    QVERIFY(readiness.summary.contains(QStringLiteral("isolated by workspace")));
    QVERIFY(readiness.checks.contains(QStringLiteral("Chat context: workspace-separated")));
    QVERIFY(readiness.checks.contains(QStringLiteral("Filesystem scanning: disabled")));
    QVERIFY(readiness.checks.contains(QStringLiteral("Background indexing: disabled")));
    QVERIFY(readiness.boundaryDiagnostics.contains(
        QStringLiteral("Local RAG: disabled by default; manual indexing only")));
}

void WorkspaceServiceTest::exposesPermissionPostureAndActions() {
    const WorkspaceService service;

    QVERIFY(service.permissionPostures().contains(QStringLiteral("Workspace Only")));
    QVERIFY(service.actionPlaceholders().contains(QStringLiteral("Create Workspace: available")));
    QVERIFY(service.actionPlaceholders().contains(QStringLiteral("Recursive Scan: disabled")));
}

void WorkspaceServiceTest::normalizesUnknownSelection() {
    const WorkspaceService service;

    QCOMPARE(service.normalizedWorkspaceId(QStringLiteral("unknown")), QStringLiteral("personal"));
    QCOMPARE(service.selectedWorkspace(QStringLiteral("unknown")).name, QStringLiteral("Personal"));
}

void WorkspaceServiceTest::supportsWorkspaceLifecycle() {
    const WorkspaceService service;

    const auto created = service.createWorkspace(
        service.defaultCatalogJson(), QStringLiteral("Client Work"), QStringLiteral("Research"));
    QVERIFY(created.success);
    QCOMPARE(service.selectedWorkspace(created.selectedWorkspaceId, created.catalogJson).name,
             QStringLiteral("Client Work"));

    const auto renamed = service.renameWorkspace(created.catalogJson, created.selectedWorkspaceId,
                                                 QStringLiteral("Client Research"));
    QVERIFY(renamed.success);
    QCOMPARE(service.selectedWorkspace(created.selectedWorkspaceId, renamed.catalogJson).name,
             QStringLiteral("Client Research"));

    const auto archived =
        service.archiveWorkspace(renamed.catalogJson, created.selectedWorkspaceId);
    QVERIFY(archived.success);
    QVERIFY(service.selectedWorkspace(created.selectedWorkspaceId, archived.catalogJson).archived);

    const auto duplicated =
        service.duplicateWorkspace(archived.catalogJson, QStringLiteral("personal"));
    QVERIFY(duplicated.success);
    QVERIFY(!duplicated.selectedWorkspaceId.isEmpty());

    const auto deleted = service.deleteWorkspace(
        duplicated.catalogJson, duplicated.selectedWorkspaceId, duplicated.selectedWorkspaceId);
    QVERIFY(deleted.success);
    QCOMPARE(deleted.selectedWorkspaceId, QStringLiteral("personal"));
}

QTEST_MAIN(WorkspaceServiceTest)

#include "test_workspace_service.moc"
