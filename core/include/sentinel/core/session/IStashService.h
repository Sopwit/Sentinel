// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QJsonObject>

namespace sentinel::core {

struct StashedChanges {
    QString stashId;
    QString sessionId;
    QString description;
    QJsonObject data;
    QDateTime createdAt;
};

class IStashService {
public:
    virtual ~IStashService() = default;

    virtual QString stash(const QString& sessionId, const QJsonObject& data, const QString& description = {}) = 0;
    virtual bool unstash(const QString& stashId, QJsonObject& data) = 0;
    virtual bool removeStash(const QString& stashId) = 0;
    virtual QList<StashedChanges> stashes(const QString& sessionId = {}) const = 0;
    virtual std::optional<StashedChanges> findStash(const QString& stashId) const = 0;
};

} // namespace sentinel::core
