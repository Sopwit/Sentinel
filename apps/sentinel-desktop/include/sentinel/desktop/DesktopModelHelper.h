// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "sentinel/desktop/DaemonClient.h"
#include <QSet>
#include <QVariant>
namespace sentinel::desktop {
class DesktopModelHelper final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool pulling READ pulling NOTIFY changed)
    Q_PROPERTY(bool hasMore READ hasMore NOTIFY changed)
    Q_PROPERTY(bool hasPrevious READ hasPrevious NOTIFY changed)
    Q_PROPERTY(bool fetching READ fetching NOTIFY changed)
    Q_PROPERTY(QString activeModel READ activeModel NOTIFY changed)
    Q_PROPERTY(double progress READ progress NOTIFY changed)
    Q_PROPERTY(QString catalogStatus READ catalogStatus NOTIFY changed)
    Q_PROPERTY(QString statusText READ statusText NOTIFY changed)
    Q_PROPERTY(QString errorText READ errorText NOTIFY changed)
    Q_PROPERTY(QVariantList models READ models NOTIFY changed)
    Q_PROPERTY(QVariantList catalog READ catalog NOTIFY changed)
    Q_PROPERTY(QString query READ query NOTIFY changed)
    Q_PROPERTY(int installedCount READ installedCount NOTIFY changed)
    Q_PROPERTY(QString setupStatus READ setupStatus NOTIFY changed)
    Q_PROPERTY(bool runtimeInstalled READ runtimeInstalled NOTIFY changed)
    Q_PROPERTY(bool setupBusy READ setupBusy NOTIFY changed)
    Q_PROPERTY(QString readme READ readme NOTIFY changed)
    Q_PROPERTY(QVariantList tags READ tags NOTIFY changed)
    Q_PROPERTY(QString installCmd READ installCmd NOTIFY changed)
public:
    DesktopModelHelper(DaemonClient& transport, QString component, QObject* parent = nullptr);
    QString query() const { return m_values.value("query").toString(); }
    int installedCount() const { return m_values.value("installedCount").toInt(); }
    QString setupStatus() const { return m_values.value("setupStatus").toString(); }
    bool runtimeInstalled() const { return m_values.value("runtimeInstalled").toBool(); }
    bool setupBusy() const { return m_values.value("setupBusy").toBool(); }
    Q_INVOKABLE void cancelSetup() { action("cancelSetup", {}); }
    Q_INVOKABLE void setupRuntime() { action("setupRuntime", {}); }
    bool hasMore() const {
        return m_values.value("hasMore").toBool();
    }
    bool hasPrevious() const {
        return m_values.value("hasPrevious").toBool();
    }
    Q_INVOKABLE void nextPage() {
        action("nextPage", {});
    }
    Q_INVOKABLE void previousPage() {
        action("previousPage", {});
    }
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
    QString catalogStatus() const {
        return m_values.value("catalogStatus").toString();
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
    QVariantList catalog() const {
        return m_values.value("catalog").toList();
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
    Q_INVOKABLE void refreshCatalog(const QString& query = {}) {
        action("refresh", query);
    }
    Q_INVOKABLE void fetchDetails(const QString& value) {
        action("fetchDetails", value);
    }
    Q_INVOKABLE void downloadFile(const QString& id) { action("downloadFile", id); }
    Q_INVOKABLE void download(const QString& id) {
        action("download", id);
    }
    Q_INVOKABLE void select(const QString& id) {
        action("select", id);
    }
    Q_INVOKABLE void importFile(const QString& path) {
        action("import", path);
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
    QString m_actionError;
    QSet<QString> m_reads, m_actions;
    QVariantList m_partialModels;
    QString m_partialQuery;
    int m_offset = 0;
};
} // namespace sentinel::desktop
