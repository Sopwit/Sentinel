// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/session/IStashService.h"
#include <QObject>
#include <QList>

namespace sentinel::core {

class StashService : public QObject, public IStashService {
    Q_OBJECT
public:
    explicit StashService(QObject* parent = nullptr);
    ~StashService() override;

    QString stash(const QString& sessionId, const QJsonObject& data, const QString& description = {}) override;
    bool unstash(const QString& stashId, QJsonObject& data) override;
    bool removeStash(const QString& stashId) override;
    QList<StashedChanges> stashes(const QString& sessionId = {}) const override;
    std::optional<StashedChanges> findStash(const QString& stashId) const override;

private:
    QString generateStashId() const;
    QList<StashedChanges> m_stashes;
};

} // namespace sentinel::core
