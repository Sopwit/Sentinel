#include "sentinel/core/security/SQLitePermissionGrantStore.h"
#include "sentinel/core/security/PathGuard.h"

#include <QDir>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>
#include <QVariant>

namespace sentinel::core {
namespace {
struct Connection {
    QString name = QStringLiteral("sentinel_permission_%1").arg(QUuid::createUuid().toString(QUuid::Id128));
    QSqlDatabase db;
    explicit Connection(const QString& path) {
        db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), name);
        db.setDatabaseName(path);
        if (db.open()) {
            QSqlQuery query(db);
            query.exec(QStringLiteral("PRAGMA busy_timeout=5000"));
        }
    }
    ~Connection() {
        db = {};
        QSqlDatabase::removeDatabase(name);
    }
};
} // namespace

SQLitePermissionGrantStore::SQLitePermissionGrantStore(QString databasePath)
    : databasePath_(std::move(databasePath)) {
    ready_ = initialize();
}

bool SQLitePermissionGrantStore::initialize() {
    if (!QDir().mkpath(QFileInfo(databasePath_).absolutePath())) {
        lastError_ = QStringLiteral("Permission storage directory is unavailable.");
        return false;
    }
    Connection connection(databasePath_);
    if (!connection.db.isOpen()) {
        lastError_ = connection.db.lastError().text();
        return false;
    }
    if (!connection.db.transaction()) {
        lastError_ = connection.db.lastError().text();
        return false;
    }
    QSqlQuery query(connection.db);
    const QStringList schema{
        QStringLiteral("CREATE TABLE IF NOT EXISTS permission_grants("
                       "grant_id TEXT PRIMARY KEY,domain INTEGER NOT NULL,access INTEGER NOT NULL,"
                       "resource_kind INTEGER NOT NULL,resource_scope TEXT NOT NULL,"
                       "decision INTEGER NOT NULL CHECK(decision=1),created_at TEXT NOT NULL)"),
        QStringLiteral("CREATE INDEX IF NOT EXISTS permission_grants_lookup ON "
                       "permission_grants(domain,access,resource_kind,resource_scope)")};
    for (const auto& sql : schema) {
        if (!query.exec(sql)) {
            lastError_ = query.lastError().text();
            connection.db.rollback();
            return false;
        }
    }
    if (!query.exec(QStringLiteral("PRAGMA user_version=1")) || !connection.db.commit()) {
        lastError_ = connection.db.lastError().text();
        connection.db.rollback();
        return false;
    }
    return true;
}

bool SQLitePermissionGrantStore::load(QList<PersistentPermissionGrant>& grants) {
    grants.clear();
    if (!ready_)
        return false;
    Connection connection(databasePath_);
    QSqlQuery query(connection.db);
    if (!connection.db.isOpen() ||
        !query.exec(QStringLiteral("SELECT grant_id,domain,access,resource_kind,resource_scope,"
                                   "created_at FROM permission_grants WHERE decision=1"))) {
        lastError_ = connection.db.isOpen() ? query.lastError().text() : connection.db.lastError().text();
        return false;
    }
    while (query.next()) {
        const int domain = query.value(1).toInt();
        const int access = query.value(2).toInt();
        const int kind = query.value(3).toInt();
        if (domain < 0 || domain > static_cast<int>(SecurityDomain::ExternalService) ||
            access < 0 || access > static_cast<int>(AccessMode::Invoke) ||
            kind < 0 || kind > static_cast<int>(AuthorizationResourceKind::Provider))
            continue;
        const QString scope = query.value(4).toString();
        if (kind == static_cast<int>(AuthorizationResourceKind::None) ||
            kind == static_cast<int>(AuthorizationResourceKind::Argument) || scope.size() > 2048 ||
            (kind == static_cast<int>(AuthorizationResourceKind::FileSystemPath) &&
             (scope.isEmpty() || scope == QStringLiteral("/") ||
              PathGuard::canonicalPath(scope) != scope)) ||
            (kind == static_cast<int>(AuthorizationResourceKind::Host) &&
             (scope.isEmpty() || scope.contains(QLatin1Char('/')) ||
              scope.contains(QLatin1Char('*')))))
            continue;
        grants.append({query.value(0).toString(), static_cast<SecurityDomain>(domain),
                       static_cast<AccessMode>(access), static_cast<AuthorizationResourceKind>(kind),
                       scope,
                       QDateTime::fromString(query.value(5).toString(), Qt::ISODateWithMs)});
    }
    return true;
}

bool SQLitePermissionGrantStore::save(const QList<PersistentPermissionGrant>& grants) {
    if (!ready_)
        return false;
    Connection connection(databasePath_);
    if (!connection.db.isOpen() || !connection.db.transaction()) {
        lastError_ = connection.db.lastError().text();
        return false;
    }
    for (const auto& grant : grants) {
        QSqlQuery query(connection.db);
        query.prepare(QStringLiteral("INSERT INTO permission_grants "
                                     "(grant_id,domain,access,resource_kind,resource_scope,decision,created_at) "
                                     "VALUES (?,?,?,?,?,1,?)"));
        query.addBindValue(grant.id);
        query.addBindValue(static_cast<int>(grant.domain));
        query.addBindValue(static_cast<int>(grant.access));
        query.addBindValue(static_cast<int>(grant.resourceKind));
        query.addBindValue(grant.scope);
        query.addBindValue(grant.createdAt.toUTC().toString(Qt::ISODateWithMs));
        if (!query.exec()) {
            lastError_ = query.lastError().text();
            connection.db.rollback();
            return false;
        }
    }
    if (!connection.db.commit()) {
        lastError_ = connection.db.lastError().text();
        return false;
    }
    return true;
}

bool SQLitePermissionGrantStore::remove(const QString& id) {
    if (!ready_)
        return false;
    Connection connection(databasePath_);
    QSqlQuery query(connection.db);
    query.prepare(QStringLiteral("DELETE FROM permission_grants WHERE grant_id=?"));
    query.addBindValue(id);
    if (!connection.db.isOpen() || !query.exec()) {
        lastError_ = connection.db.isOpen() ? query.lastError().text() : connection.db.lastError().text();
        return false;
    }
    return true;
}

bool SQLitePermissionGrantStore::clear() {
    if (!ready_)
        return false;
    Connection connection(databasePath_);
    QSqlQuery query(connection.db);
    if (!connection.db.isOpen() || !query.exec(QStringLiteral("DELETE FROM permission_grants"))) {
        lastError_ = connection.db.isOpen() ? query.lastError().text() : connection.db.lastError().text();
        return false;
    }
    return true;
}

} // namespace sentinel::core
