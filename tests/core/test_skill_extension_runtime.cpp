// SPDX-License-Identifier: GPL-3.0-or-later
#include "sentinel/core/skill/SkillService.h"
#include "sentinel/core/extension/ExtensionService.h"
#include "sentinel/core/plugin/PluginManager.h"
#include "sentinel/core/runtime/InMemoryToolRegistry.h"
#include "sentinel/core/agent/ContextEngine.h"
#include "sentinel/core/network/NetworkPolicyService.h"
#include "sentinel/core/security/PermissionService.h"
#include "sentinel/core/security/SQLitePermissionGrantStore.h"
#include "sentinel/core/security/PathGuard.h"
#include <QFile>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QUuid>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QtTest>
using namespace sentinel::core;
class SkillExtensionRuntimeTest final: public QObject {
    Q_OBJECT
    QString preferencePath;
private slots:
    void workspaceScopedSkillEnablementPersists() {
        {
            SkillService service;
            QCOMPARE(service.discoverSkills(QString::fromUtf8(TEST_SKILL_FIXTURE_DIR)),2);
            service.setActiveWorkspaceId("workspace-a");
            QVERIFY(service.setEnabled("workspace-certification",false));
        }
        SkillService restored;
        QCOMPARE(restored.discoverSkills(QString::fromUtf8(TEST_SKILL_FIXTURE_DIR)),2);
        restored.setActiveWorkspaceId("workspace-a");
        QCOMPARE(restored.findSkill("workspace-certification")->state,SkillState::Disabled);
        QVERIFY(restored.getSkillContent("workspace-certification").isEmpty());
        QVERIFY(restored.setEnabled("workspace-certification",true));
        QVERIFY(!restored.getSkillContent("workspace-certification").isEmpty());
        restored.setActiveWorkspaceId("workspace-b");
        QVERIFY(restored.getSkillContent("workspace-certification").isEmpty());
    }
    void agentContextSkillsAreScopedOrderedAndBounded() {
        SkillService service;
        QCOMPARE(service.discoverSkills(QString::fromUtf8(TEST_SKILL_FIXTURE_DIR)), 2);
        service.setActiveWorkspaceId("workspace-a");
        Skill earlier; earlier.name="aaa"; earlier.content="first instruction";
        QVERIFY(service.addSkill(earlier));
        AgentContextInput input; input.goal="list workspace";
        input.workspaceContext.id="workspace-a";
        input.skills=service.skills();
        input.skills.append(earlier);
        auto context=ContextEngine{}.build(input);
        QStringList names;
        QString contents;
        for(const auto& item:context.items) if(item.kind==AgentContextKind::Skill) {
            names.append(item.source); contents+=item.content;
        }
        QCOMPARE(names,QStringList({"skill:aaa","skill:certification","skill:workspace-certification"}));
        QVERIFY(contents.contains("[SKILL_OK]"));
        input.workspaceContext.id="workspace-b";
        context=ContextEngine{}.build(input);
        for(const auto& item:context.items) QVERIFY(item.source!="skill:workspace-certification");
        QVERIFY(service.setEnabled("certification",false));
        input.skills=service.skills();
        context=ContextEngine{}.build(input);
        for(const auto& item:context.items) QVERIFY(!item.content.contains("[SKILL_OK]"));
        Skill oversized; oversized.name="large"; oversized.content=QString(20000,'x');
        input.skills={oversized};
        context=ContextEngine{}.build(input);
        QVERIFY(context.compacted);
        QVERIFY(context.omittedItems>0);
        QCOMPARE(context.items.first().content,input.goal);
        for(const auto& item:context.items) QVERIFY(item.kind!=AgentContextKind::Skill);
        QVERIFY(service.setEnabled("certification",true));
    }
    void initTestCase() {
        QStandardPaths::setTestModeEnabled(true);
        QCoreApplication::setApplicationName("sentinel-skill-certification-"+QUuid::createUuid().toString(QUuid::WithoutBraces));
        preferencePath=QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)+"/skill_preferences.json";
    }
    void discoveryScopeAndPersistence() {
        const QString root=QString::fromUtf8(TEST_SKILL_FIXTURE_DIR);
        {
            SkillService skills;
            QCOMPARE(skills.discoverSkills(root),2);
            QCOMPARE(skills.skills().size(),2);
            QCOMPARE(skills.discoverSkills(root),2);
            QCOMPARE(skills.skills().size(),2);
            const auto global=skills.findSkill("certification");
            QVERIFY(global);
            QCOMPARE(global->version,QString("1.0.0"));
            QVERIFY(skills.getSkillContent(global->name).contains("[SKILL_OK]"));
            QVERIFY(skills.getSkillContent("workspace-certification").isEmpty());
            skills.setActiveWorkspaceId("workspace-a");
            QVERIFY(!skills.getSkillContent("workspace-certification").isEmpty());
            skills.setActiveWorkspaceId("workspace-b");
            QVERIFY(skills.getSkillContent("workspace-certification").isEmpty());
            QVERIFY(skills.setEnabled(global->name,false));
            QVERIFY(skills.getSkillContent(global->name).isEmpty());
        }
        SkillService restored;
        QCOMPARE(restored.discoverSkills(root),2);
        QCOMPARE(restored.findSkill("certification")->state,SkillState::Disabled);
        QVERIFY(restored.setEnabled("certification",true));
        QVERIFY(restored.getSkillContent("certification").contains("[SKILL_OK]"));
        restored.setWorkspacePreferences({{"skill:certification",false}});
        QVERIFY(restored.getSkillContent("certification").isEmpty());
        restored.setWorkspacePreferences({});
        QVERIFY(!restored.getSkillContent("certification").isEmpty());
    }
    void duplicateCannotReplaceInstructions() {
        SkillService skills;
        Skill first;first.name="unique";first.content="original";
        QVERIFY(skills.addSkill(first));
        Skill duplicate=first;duplicate.content="replacement";
        QVERIFY(!skills.addSkill(duplicate));
        QCOMPARE(skills.getSkillContent("unique"),QString("original"));
        Skill empty;empty.name="empty";
        QVERIFY(!skills.addSkill(empty));
    }
    void extensionFailureIsolationAndScopeIdentity() {
        QTemporaryDir storage;
        plugin::PluginManager plugins("1.0.0",storage.path());
        InMemoryToolRegistry registry;
        plugins.setToolRegistry(&registry);
        SkillService skills;
        QCOMPARE(skills.discoverSkills(QString::fromUtf8(TEST_SKILL_FIXTURE_DIR)),2);
        ExtensionService extensions(nullptr,&plugins,&skills,&registry);
        extensions.setWorkspacePreferences("workspace-b",{});
        bool scoped=false;
        for(const auto& item:extensions.extensions())if(item.id=="skill:workspace-certification") {
            scoped=true;QCOMPARE(item.workspaceId,QString("workspace-a"));QVERIFY(!item.available);
        }
        QVERIFY(scoped);
        QVERIFY(extensions.perform("skill:certification",ExtensionAction::Disable));
        QVERIFY(skills.getSkillContent("certification").isEmpty());
        QVERIFY(extensions.perform("skill:certification",ExtensionAction::Enable));
        QVERIFY(skills.getSkillContent("certification").contains("[SKILL_OK]"));
        QVERIFY(!plugins.initializePlugin("missing-plugin"));
        QVERIFY(!skills.getSkillContent("certification").isEmpty());
        Skill invalid;QVERIFY(!skills.addSkill(invalid));
        QVERIFY(plugins.registeredPluginIds().isEmpty());
    }
    void localInstructionsCannotChangeNetworkPolicy() {
        SkillService skills;
        QCOMPARE(skills.discoverSkills(QString::fromUtf8(TEST_SKILL_FIXTURE_DIR)),2);
        auto& policy=NetworkPolicyService::instance();
        const auto previous=policy.mode();
        for(const auto mode:{NetworkMode::Offline,NetworkMode::LocalOnly}) {
            policy.setMode(mode);
            QVERIFY(!skills.getSkillContent("certification").isEmpty());
            QVERIFY(policy.check(QUrl("https://example.com"))!=NetworkDecision::Allowed);
            QCOMPARE(policy.check(QUrl("http://127.0.0.1")),NetworkDecision::Allowed);
        }
        policy.setMode(previous);
    }
    void pluginGrantOwnerIsolationAndPersistence() {
        QTemporaryDir directory;
        auto store=std::make_shared<SQLitePermissionGrantStore>(directory.filePath("permissions.sqlite"));
        AuthorizationRequest a{SecurityDomain::FileSystem,AccessMode::Read,directory.path()};
        a.providerId="plugin:one";
        AuthorizationRequest b=a;b.providerId="plugin:two";
        AuthorizationRequest builtIn=a;builtIn.providerId="sentinel:built-in";
        PermissionService permissions(store);
        QVERIFY(permissions.grantAuthorization(a,"session",false));
        QCOMPARE(permissions.evaluateAuthorization(a,"session"),PermissionEffect::Allow);
        QCOMPARE(permissions.evaluateAuthorization(b,"session"),PermissionEffect::Ask);
        QCOMPARE(permissions.evaluateAuthorization(builtIn,"session"),PermissionEffect::Ask);
        QCOMPARE(permissions.evaluateAuthorization(a,"other-session"),PermissionEffect::Ask);
        QVERIFY(permissions.grantAuthorization(a,"session",true));
        PermissionService restored(store);
        QCOMPARE(restored.evaluateAuthorization(a,"new-session"),PermissionEffect::Allow);
        QCOMPARE(restored.evaluateAuthorization(b,"new-session"),PermissionEffect::Ask);
        QCOMPARE(restored.evaluateAuthorization(builtIn,"new-session"),PermissionEffect::Ask);
    }
    void legacyGrantMigrationDoesNotTrustPlugins() {
        QTemporaryDir directory;
        const auto path=directory.filePath("legacy.sqlite");
        const QString connection=QUuid::createUuid().toString();
        {
            auto database=QSqlDatabase::addDatabase("QSQLITE",connection);
            database.setDatabaseName(path);QVERIFY(database.open());
            QSqlQuery query(database);
            QVERIFY(query.exec("CREATE TABLE permission_grants(grant_id TEXT PRIMARY KEY,domain INTEGER NOT NULL,access INTEGER NOT NULL,resource_kind INTEGER NOT NULL,resource_scope TEXT NOT NULL,decision INTEGER NOT NULL,created_at TEXT NOT NULL)"));
            query.prepare("INSERT INTO permission_grants VALUES ('legacy',?,?,?, ?,1,'2026-10-03T00:00:00.000Z')");
            query.addBindValue(static_cast<int>(SecurityDomain::FileSystem));
            query.addBindValue(static_cast<int>(AccessMode::Read));
            query.addBindValue(static_cast<int>(AuthorizationResourceKind::FileSystemPath));
            query.addBindValue(PathGuard::canonicalPath(directory.path()));QVERIFY(query.exec());
        }
        QSqlDatabase::removeDatabase(connection);
        auto store=std::make_shared<SQLitePermissionGrantStore>(path);
        PermissionService restored(store);
        AuthorizationRequest builtin{SecurityDomain::FileSystem,AccessMode::Read,directory.path()};
        builtin.providerId="sentinel:built-in";
        QCOMPARE(restored.evaluateAuthorization(builtin,"session"),PermissionEffect::Allow);
        auto plugin=builtin;plugin.providerId="plugin:one";
        QCOMPARE(restored.evaluateAuthorization(plugin,"session"),PermissionEffect::Ask);
        QCOMPARE(restored.persistentGrants().size(),1);
    }
    void pluginManifestRejectionAndDuplicateIdentity() {
        QTemporaryDir directory;
        auto write=[&](const QString& folder,const QByteArray& bytes) {
            const auto path=directory.filePath(folder);
            if(!QDir().mkpath(path))return false;
            QFile file(path+"/plugin.json");
            return file.open(QIODevice::WriteOnly) && file.write(bytes)==bytes.size();
        };
        QVERIFY(write("malformed","{broken"));
        QVERIFY(write("missing","{\"name\":\"Missing identity\"}"));
        auto manifest=QJsonObject{{"id","certification.plugin"},{"name","Certification"},
            {"version","1.0.0"},{"api_version","5.0"},{"entry_point","not_loaded"},
            {"permissions",QJsonArray{"tool.execute"}}};
        QVERIFY(write("one",QJsonDocument(manifest).toJson()));
        plugin::PluginManager manager("1.0.0",directory.filePath("storage"));
        QCOMPARE(manager.discoverPlugins(directory.path()),1);
        QVERIFY(manager.descriptor("certification.plugin"));
        QVERIFY(manager.pluginInstance("certification.plugin")==nullptr);
        QVERIFY(write("two",QJsonDocument(manifest).toJson()));
        manager.discoverPlugins(directory.path());
        QCOMPARE(manager.registeredPluginIds().size(),1);
        QCOMPARE(manager.descriptor("certification.plugin")->failureCategory,QString("PluginDuplicateId"));
        QVERIFY(!manager.initializePlugin("certification.plugin"));
        manifest.insert("id","certification.incompatible");manifest.insert("api_version","999.0");
        QVERIFY(write("incompatible",QJsonDocument(manifest).toJson()));
        manager.discoverPlugins(directory.path());
        QCOMPARE(manager.descriptor("certification.incompatible")->failureCategory,QString("PluginIncompatible"));
        QVERIFY(!manager.initializePlugin("certification.incompatible"));
    }
    void cleanupTestCase() { if(QFile::exists(preferencePath))QVERIFY(QFile::remove(preferencePath)); }
};
QTEST_GUILESS_MAIN(SkillExtensionRuntimeTest)
#include "test_skill_extension_runtime.moc"
