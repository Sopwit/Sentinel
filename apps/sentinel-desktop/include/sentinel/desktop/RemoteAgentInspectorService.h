// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "sentinel/core/app/AgentInspectorService.h"
#include "sentinel/desktop/DaemonClient.h"
#include <QTimer>
namespace sentinel::desktop {
class RemoteAgentInspectorService final : public QObject, public core::AgentInspectorService {
    Q_OBJECT
public:
    explicit RemoteAgentInspectorService(DaemonClient& client, QObject* parent = nullptr);
    QList<core::StoredAgentRun> recentRuns(int limit) const override;
    QList<core::StoredAgentRun> runsBefore(const QDateTime&, const QString&, int) const override;
    core::AgentInspectorDetail detail(const QString&) const override;
    bool hasMore() const {
        return hasMore_;
    }
    QString error() const override {
        return error_;
    }
signals:
    void updated();

private:
    void request(const QString& run = {}, const QDateTime& before = {},
                 const QString& beforeId = {});
    DaemonClient& client_;
    QTimer timer_;
    QString pending_, selected_, error_;
    bool hasMore_ = false;
    QDateTime queuedBefore_;
    QString queuedBeforeId_;
    QList<core::StoredAgentRun> runs_;
    core::AgentInspectorDetail detail_;
};
} // namespace sentinel::desktop
