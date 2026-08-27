// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJsonObject>
#include <QString>
#include <QStringList>

namespace sentinel::core {

struct Workspace {
    QString name;
    QString path;
    QString description;
    QStringList projectPaths;
    bool isActive{false};
};

class IWorkspaceManagerService {
public:
    virtual ~IWorkspaceManagerService() = default;

    virtual Workspace create(const QString& name, const QString& path = {}) = 0;
    virtual bool remove(const QString& name) = 0;
    virtual bool switchTo(const QString& name) = 0;
    virtual QList<Workspace> list() const = 0;
    virtual std::optional<Workspace> current() const = 0;
    virtual std::optional<Workspace> find(const QString& name) const = 0;
    virtual bool addProject(const QString& workspaceName, const QString& projectPath) = 0;
};

} // namespace sentinel::core
