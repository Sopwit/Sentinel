// SPDX-License-Identifier: GPL-3.0-or-later
#include "sentinel/desktop/DesktopModelHelper.h"
#include <QTimer>
namespace sentinel::desktop {
DesktopModelHelper::DesktopModelHelper(DaemonClient& transport, QString component, QObject* parent)
    : QObject(parent), m_transport(transport), m_component(std::move(component)) {
    auto* timer = new QTimer(this);
    timer->setInterval(2000);
    connect(timer, &QTimer::timeout, this, &DesktopModelHelper::refresh);
    timer->start();
    connect(&transport, &DaemonClient::connectionStateChanged, this, [this] {
        if (m_transport.daemonReachable())
            refresh();
        else {
            m_reads.clear();
            m_offset = 0; m_partialModels.clear();
            m_actions.clear();
            m_values.clear();
            m_values["errorText"] = m_transport.statusSummary();
            emit changed();
        }
    });
    connect(&transport, &DaemonClient::requestFailed, this,
            [this](const QString& id, DaemonClient::Error, const QString& code) {
                if (m_reads.remove(id) || m_actions.remove(id)) {
                    m_offset = 0; m_partialModels.clear();
                    m_actionError = code;
                    m_values["errorText"] = code;
                    emit changed();
                }
            });
    connect(&transport, &DaemonClient::responseReceived, this,
            [this](const QString& id, const QString&, const QJsonObject& payload) {
                if (m_actions.remove(id)) {
                    if (!payload.value("accepted").toBool()) {
                        m_actionError = "Daemon rejected model action";
                        m_values["errorText"] = m_actionError;
                    }
                    refresh();
                    emit changed();
                    return;
                }
                if (!m_reads.remove(id) || payload.value("component").toString() != m_component)
                    return;
                const bool wasFetching = fetching();
                const bool wasPulling = pulling();
                auto values = payload.value("properties").toObject().toVariantMap();
                if (!m_actionError.isEmpty())
                    values["errorText"] = m_actionError;
                const auto query = values.value("query").toString();
                const int offset = values.value("modelOffset").toInt();
                if (offset == 0) { m_partialModels.clear(); m_partialQuery = query; }
                if (query != m_partialQuery || offset != m_partialModels.size()) {
                    m_offset = 0; m_partialModels.clear(); refresh(); return;
                }
                m_partialModels.append(values.value("models").toList());
                const int next = values.value("nextOffset", -1).toInt();
                if (next >= 0) { m_offset = next; refresh(); return; }
                values["models"] = m_partialModels;
                m_offset = 0;
                m_partialModels.clear();
                if (values == m_values)
                    return;
                m_values = std::move(values);
                emit changed();
                if (wasFetching && !fetching())
                    emit fetchFinished(errorText().isEmpty());
                if (wasPulling && !pulling())
                    emit pullFinished(activeModel(), errorText().isEmpty());
            });
}
void DesktopModelHelper::refresh() {
    if (!m_transport.daemonReachable() || !m_reads.isEmpty())
        return;
    m_reads.insert(m_transport.request(DaemonClient::Command::model_helper_state,
                                       {{"component", m_component}, {"offset", m_offset}}));
}
void DesktopModelHelper::action(const QString& name, const QString& value) {
    m_actionError.clear();
    if (name == "fetch" || name == "refresh") {
        m_reads.clear(); m_offset = 0; m_partialModels.clear();
        m_values["models"] = QVariantList{}; m_values["query"] = value; m_values["fetching"] = true;
        emit changed();
    }
    m_values["errorText"] = QString{};
    m_actions.insert(
        m_transport.request(DaemonClient::Command::model_helper_action,
                            {{"component", m_component}, {"action", name}, {"value", value}}));
}
} // namespace sentinel::desktop
