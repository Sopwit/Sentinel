// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QJsonObject>
#include <functional>

#include "sentinel/core/reference/Reference.h"

namespace sentinel::core {

class IReferenceService {
public:
    virtual ~IReferenceService() = default;

    // Reference management
    virtual bool addReference(const Reference& ref) = 0;
    virtual bool removeReference(const QString& name) = 0;
    virtual bool updateReference(const Reference& ref) = 0;
    virtual QList<Reference> references() const = 0;
    virtual std::optional<Reference> findReference(const QString& name) const = 0;

    // Reference content
    virtual QString getReferenceContent(const QString& name) const = 0;
    virtual QString getReferenceContent(const QString& name, const QString& query) const = 0;

    // Discovery
    virtual int loadReferencesFromConfig(const QJsonArray& refsArray) = 0;

    // Validation
    virtual bool validateReference(const Reference& ref) const = 0;
    virtual void refreshAvailability() = 0;
};

} // namespace sentinel::core
