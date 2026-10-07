// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "sentinel/desktop/DaemonClient.h"
#include <QSet>
#include <QVariant>
namespace sentinel::desktop {
class DesktopModelHelper final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool pulling READ pulling NOTIFY changed)
    Q_PROPERTY(bool fetching READ fetching NOTIFY changed)
    Q_PROPERTY(QString activeModel READ activeModel NOTIFY changed)
    Q_PROPERTY(double progress READ progress NOTIFY changed)
    Q_PROPERTY(QString statusText READ statusText NOTIFY changed)
    Q_PROPERTY(QString errorText READ errorText NOTIFY changed)
    Q_PROPERTY(QVariantList models READ models NOTIFY changed)
    Q_PROPERTY(QString readme READ readme NOTIFY changed)
    Q_PROPERTY(QVariantList tags READ tags NOTIFY changed)
    Q_PROPERTY(QString installCmd READ installCmd NOTIFY changed)
public:
    DesktopModelHelper(DaemonClient& transport, QString component, QObject* parent = nullptr);
    bool pulling() const {
        return m_values.value("pulling").value<bool>();
    }
    bool fetching() const {
        return m_values.value("fetching").value<bool>();
    }
    QString activeModel() const {
        return m_values.value("activeModel").value<QString>();
    }
    double progress() const {
        return m_values.value("progress").value<double>();
    }
    QString statusText() const {
        return m_values.value("statusText").value<QString>();
    }
    QString errorText() const {
        return m_values.value("errorText").value<QString>();
    }
    QVariantList models() const {
        return m_values.value("models").value<QVariantList>();
    }
    QString readme() const {
        return m_values.value("readme").value<QString>();
    }
    QVariantList tags() const {
        return m_values.value("tags").value<QVariantList>();
    }
    QString installCmd() const {
        return m_values.value("installCmd").value<QString>();
    }
    Q_INVOKABLE void pull(const QString& value) {
        action("pull", value);
    }
    Q_INVOKABLE void removeModel(const QString& value) {
        action("removeModel", value);
    }
    Q_INVOKABLE void fetch(const QString& value = {}) {
        action("fetch", value);
    }
    Q_INVOKABLE void fetchDetails(const QString& value) {
        action("fetchDetails", value);
    }
    Q_INVOKABLE void cancel() {
        action("cancel", {});
    }
signals:
    void changed();
    void fetchFinished(bool success);
    void pullFinished(const QString& model, bool success);
    void removeFinished(const QString& model, bool success);

private:
    void refresh();
    void action(const QString& name, const QString& value);
    DaemonClient& m_transport;
    QString m_component;
    QVariantMap m_values;
    QSet<QString> m_reads, m_actions;
};
} // namespace sentinel::desktop
