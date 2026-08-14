// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/session/IMdnsService.h"
#include <QObject>
#include <QList>
#include <QMap>
#include <QProcess>

namespace sentinel::core {

class MdnsServiceImpl : public QObject, public IMdnsService {
    Q_OBJECT
public:
    explicit MdnsServiceImpl(QObject* parent = nullptr);
    ~MdnsServiceImpl() override;

    bool publish(const MdnsService& service) override;
    bool unpublish(const QString& name) override;
    QList<MdnsService> discoveredServices() const override;
    void setEnabled(bool enabled) override;
    bool isEnabled() const override;

private:
    bool m_enabled{false};
    QList<MdnsService> m_published;
    mutable QList<MdnsService> m_discovered;
    QMap<QString, QProcess*> m_publishProcesses;
};

} // namespace sentinel::core
