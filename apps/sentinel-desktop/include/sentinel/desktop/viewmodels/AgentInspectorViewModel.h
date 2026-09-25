// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "sentinel/core/app/AgentInspectorService.h"

#include <QAbstractListModel>
#include <QTimer>
#include <QVariantMap>

namespace sentinel::desktop {

class InspectorRowsModel final : public QAbstractListModel {
    Q_OBJECT
public:
    enum Role { EntryRole = Qt::UserRole + 1 };
    explicit InspectorRowsModel(QObject* parent = nullptr) : QAbstractListModel(parent) {}
    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    void setRows(QList<QVariantMap> rows);
private:
    QList<QVariantMap> rows_;
};

class AgentInspectorViewModel final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel* runs READ runs CONSTANT)
    Q_PROPERTY(QAbstractItemModel* timeline READ timeline CONSTANT)
    Q_PROPERTY(QAbstractItemModel* evidence READ evidence CONSTANT)
    Q_PROPERTY(QAbstractItemModel* claims READ claims CONSTANT)
    Q_PROPERTY(QAbstractItemModel* approvals READ approvals CONSTANT)
    Q_PROPERTY(QAbstractItemModel* children READ children CONSTANT)
    Q_PROPERTY(QVariantMap selectedRun READ selectedRun NOTIFY selectedRunChanged)
    Q_PROPERTY(QString selectedRunId READ selectedRunId NOTIFY selectedRunChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(bool hasMore READ hasMore NOTIFY runsChanged)
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(QString stateFilter READ stateFilter WRITE setStateFilter NOTIFY filtersChanged)
    Q_PROPERTY(QString typeFilter READ typeFilter WRITE setTypeFilter NOTIFY filtersChanged)
    Q_PROPERTY(QString providerFilter READ providerFilter WRITE setProviderFilter NOTIFY filtersChanged)
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY filtersChanged)
public:
    explicit AgentInspectorViewModel(sentinel::core::AgentInspectorService& service,
                                     QObject* parent = nullptr);
    QAbstractItemModel* runs() { return &runs_; }
    QAbstractItemModel* timeline() { return &timeline_; }
    QAbstractItemModel* evidence() { return &evidence_; }
    QAbstractItemModel* claims() { return &claims_; }
    QAbstractItemModel* approvals() { return &approvals_; }
    QAbstractItemModel* children() { return &children_; }
    QVariantMap selectedRun() const { return selectedRun_; }
    QString selectedRunId() const { return selectedRunId_; }
    QString errorMessage() const { return errorMessage_; }
    bool hasMore() const { return hasMore_; }
    bool active() const { return active_; }
    void setActive(bool value);
    QString stateFilter() const { return stateFilter_; }
    QString typeFilter() const { return typeFilter_; }
    QString providerFilter() const { return providerFilter_; }
    QString searchText() const { return searchText_; }
    void setStateFilter(QString value);
    void setTypeFilter(QString value);
    void setProviderFilter(QString value);
    void setSearchText(QString value);
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void loadMore();
    Q_INVOKABLE void selectRun(const QString& runId);
    Q_INVOKABLE void copySafe(const QString& field, const QString& value);
signals:
    void selectedRunChanged();
    void errorMessageChanged();
    void runsChanged();
    void activeChanged();
    void filtersChanged();
private:
    void applyFilters();
    void poll();
    void setError(QString error);
    sentinel::core::AgentInspectorService& service_;
    InspectorRowsModel runs_{this};
    InspectorRowsModel timeline_{this};
    InspectorRowsModel evidence_{this};
    InspectorRowsModel claims_{this};
    InspectorRowsModel approvals_{this};
    InspectorRowsModel children_{this};
    QList<sentinel::core::StoredAgentRun> loadedRuns_;
    QVariantMap selectedRun_;
    QString selectedRunId_;
    QString errorMessage_;
    bool hasMore_ = false;
    bool active_ = false;
    QString stateFilter_;
    QString typeFilter_;
    QString providerFilter_;
    QString searchText_;
    QTimer refreshTimer_;
};

} // namespace sentinel::desktop
