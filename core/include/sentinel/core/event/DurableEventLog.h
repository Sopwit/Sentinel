#pragma once

#include <QJsonObject>
#include <QList>
#include <QSqlDatabase>
#include <optional>

namespace sentinel::core {

struct DurableEvent {
    QString aggregateId;
    qint64 sequence = 0;
    QString type;
    QJsonObject payload;
};

class DurableEventLog final {
public:
    explicit DurableEventLog(QSqlDatabase database);
    bool initialize();
    std::optional<DurableEvent> append(const QString& aggregateId, const QString& type,
                                       const QJsonObject& payload);
    QList<DurableEvent> replay(const QString& aggregateId) const;

private:
    QSqlDatabase database_;
};

} // namespace sentinel::core
