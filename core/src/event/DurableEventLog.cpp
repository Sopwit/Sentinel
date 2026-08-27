#include "sentinel/core/event/DurableEventLog.h"

#include <QJsonDocument>
#include <QSqlQuery>

namespace sentinel::core {

DurableEventLog::DurableEventLog(QSqlDatabase database) : database_(database) {}

bool DurableEventLog::initialize() {
    QSqlQuery query(database_);
    return query.exec(
        QStringLiteral("CREATE TABLE IF NOT EXISTS sentinel_events ("
                       "aggregate_id TEXT NOT NULL, sequence INTEGER NOT NULL, type TEXT NOT NULL, "
                       "payload TEXT NOT NULL, PRIMARY KEY (aggregate_id, sequence))"));
}

std::optional<DurableEvent> DurableEventLog::append(const QString& aggregateId, const QString& type,
                                                    const QJsonObject& payload) {
    QSqlQuery query(database_);
    query.prepare(QStringLiteral(
        "SELECT COALESCE(MAX(sequence), 0) + 1 FROM sentinel_events WHERE aggregate_id = ?"));
    query.addBindValue(aggregateId);
    if (!query.exec() || !query.next())
        return std::nullopt;
    const qint64 sequence = query.value(0).toLongLong();

    QSqlQuery insert(database_);
    insert.prepare(QStringLiteral(
        "INSERT INTO sentinel_events(aggregate_id, sequence, type, payload) VALUES(?, ?, ?, ?)"));
    insert.addBindValue(aggregateId);
    insert.addBindValue(sequence);
    insert.addBindValue(type);
    insert.addBindValue(QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact)));
    if (!insert.exec())
        return std::nullopt;
    return DurableEvent{aggregateId, sequence, type, payload};
}

QList<DurableEvent> DurableEventLog::replay(const QString& aggregateId) const {
    QList<DurableEvent> events;
    QSqlQuery query(database_);
    query.prepare(QStringLiteral("SELECT sequence, type, payload FROM sentinel_events WHERE "
                                 "aggregate_id = ? ORDER BY sequence"));
    query.addBindValue(aggregateId);
    if (!query.exec())
        return events;
    while (query.next()) {
        const QJsonDocument document = QJsonDocument::fromJson(query.value(2).toByteArray());
        events.append({aggregateId, query.value(0).toLongLong(), query.value(1).toString(),
                       document.object()});
    }
    return events;
}

} // namespace sentinel::core
