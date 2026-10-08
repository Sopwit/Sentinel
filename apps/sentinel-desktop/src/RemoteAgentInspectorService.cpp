// SPDX-License-Identifier: GPL-3.0-or-later
#include "sentinel/desktop/RemoteAgentInspectorService.h"
#include "sentinel/core/agent/AgentHistoryCodec.h"
#include <algorithm>
namespace sentinel::desktop {
RemoteAgentInspectorService::RemoteAgentInspectorService(DaemonClient& client, QObject* parent)
    : QObject(parent), core::AgentInspectorService(nullptr), client_(client) {
    timer_.setInterval(2500);
    connect(&timer_, &QTimer::timeout, this, [this] { request(selected_); });
    connect(&client_, &DaemonClient::connectionStateChanged, this, [this] {
        pending_.clear();
        runs_.clear();
        detail_ = {};
        queuedBefore_ = {};
        queuedBeforeId_.clear();
        if (client_.daemonReachable())
            request(selected_);
        else {
            error_ = client_.statusSummary();
            emit updated();
        }
    });
    connect(&client_, &DaemonClient::requestFailed, this,
            [this](const QString& id, DaemonClient::Error, const QString& code) {
                if (id != pending_)
                    return;
                pending_.clear();
                error_ = code;
                emit updated();
            });
    connect(&client_, &DaemonClient::responseReceived, this,
            [this](const QString& id, const QString&, const QJsonObject& payload) {
                if (id != pending_)
                    return;
                pending_.clear();
                const auto h = payload.value("history").toObject();
                using namespace core::agent_wire;
                hasMore_ = h.value("runs").toArray().size() > 25;
                for (const auto& value : h.value("runs").toArray()) {
                    const auto run = decodeStoredAgentRun(value.toObject());
                    auto it = std::find_if(runs_.begin(), runs_.end(),
                                           [&](const auto& r) { return r.runId == run.runId; });
                    if (it == runs_.end())
                        runs_.append(run);
                    else
                        *it = run;
                }
                std::sort(runs_.begin(), runs_.end(), [](const auto& a, const auto& b) {
                    return a.startedAt != b.startedAt ? a.startedAt > b.startedAt
                                                      : a.runId > b.runId;
                });
                while (runs_.size() > 500)
                    runs_.removeLast();
                detail_ = {};
                detail_.run = decodeStoredAgentRun(h.value("run").toObject());
#define READ_LIST(field, key, Type)                                                                \
    for (const auto& v : h.value(key).toArray())                                                   \
        detail_.field.append(decode##Type(v.toObject()));
                READ_LIST(steps, "steps", StoredAgentStep)
                READ_LIST(tools, "tools", StoredAgentToolCall)
                READ_LIST(authorizations, "authorizations", StoredAgentAuthorization)
                READ_LIST(evidence, "evidence", StoredAgentEvidence)
                READ_LIST(claims, "claims", StoredAgentClaim)
                READ_LIST(children, "children", StoredAgentRun)
#undef READ_LIST
                error_ = h.value("error").toString();
                emit updated();
                if (queuedBefore_.isValid()) {
                    const auto before = queuedBefore_;
                    const auto beforeId = queuedBeforeId_;
                    queuedBefore_ = {};
                    queuedBeforeId_.clear();
                    request(selected_, before, beforeId);
                }
            });
    timer_.start();
}
void RemoteAgentInspectorService::request(const QString& run, const QDateTime& before,
                                          const QString& beforeId) {
    if (!client_.daemonReachable())
        return;
    if (!pending_.isEmpty()) {
        if (before.isValid()) {
            queuedBefore_ = before;
            queuedBeforeId_ = beforeId;
        }
        return;
    }
    pending_ = client_.request(DaemonClient::Command::agent_history,
                               {{"run_id", run},
                                {"before_time", before.toString(Qt::ISODateWithMs)},
                                {"before_id", beforeId}});
}
QList<core::StoredAgentRun> RemoteAgentInspectorService::recentRuns(int limit) const {
    return runs_.mid(0, limit);
}
QList<core::StoredAgentRun> RemoteAgentInspectorService::runsBefore(const QDateTime& before,
                                                                    const QString& id,
                                                                    int limit) const {
    QList<core::StoredAgentRun> result;
    for (const auto& r : runs_)
        if (r.startedAt < before || (r.startedAt == before && r.runId < id))
            result.append(r);
    if (result.size() < limit)
        const_cast<RemoteAgentInspectorService*>(this)->request(selected_, before, id);
    return result.mid(0, limit);
}
core::AgentInspectorDetail RemoteAgentInspectorService::detail(const QString& id) const {
    if (selected_ != id) {
        auto* self = const_cast<RemoteAgentInspectorService*>(this);
        self->selected_ = id;
        self->request(id);
    }
    if (detail_.run.runId == id)
        return detail_;
    core::AgentInspectorDetail result;
    for (const auto& r : runs_)
        if (r.runId == id)
            result.run = r;
    return result;
}
} // namespace sentinel::desktop
