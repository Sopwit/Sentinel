// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/reference/IReferenceService.h"
#include <QMap>
#include <QNetworkAccessManager>
#include <QObject>

namespace sentinel::core {

class ReferenceService : public QObject, public IReferenceService {
    Q_OBJECT
public:
    explicit ReferenceService(QObject* parent = nullptr);
    ~ReferenceService() override;

    // IReferenceService interface
    bool addReference(const Reference& ref) override;
    bool removeReference(const QString& name) override;
    bool updateReference(const Reference& ref) override;
    QList<Reference> references() const override;
    std::optional<Reference> findReference(const QString& name) const override;

    QString getReferenceContent(const QString& name) const override;
    QString getReferenceContent(const QString& name, const QString& query) const override;

    int loadReferencesFromConfig(const QJsonArray& refsArray) override;

    bool validateReference(const Reference& ref) const override;
    void refreshAvailability() override;

signals:
    void referenceAdded(const QString& name);
    void referenceRemoved(const QString& name);
    void referenceUpdated(const QString& name);
    void referenceAvailabilityChanged(const QString& name, bool available);

private:
    QString readLocalContent(const QString& path) const;
    QString readUrlContent(const QString& url) const;
    QString readRepositoryContent(const QString& repoPath) const;

    QMap<QString, Reference> m_references;
    mutable QNetworkAccessManager m_networkManager;
};

} // namespace sentinel::core
