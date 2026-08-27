// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QString>
#include <functional>

#include "sentinel/core/skill/Skill.h"

namespace sentinel::core {

class ISkillService {
public:
    virtual ~ISkillService() = default;

    // Skill discovery
    virtual int discoverSkills(const QString& searchDir) = 0;
    virtual int loadSkillsFromUrl(const QString& indexUrl) = 0;

    // Skill management
    virtual bool addSkill(const Skill& skill) = 0;
    virtual bool removeSkill(const QString& name) = 0;
    virtual QList<Skill> skills() const = 0;
    virtual std::optional<Skill> findSkill(const QString& name) const = 0;

    // Skill content
    virtual QString getSkillContent(const QString& name) const = 0;
    virtual QString getSkillContentWithFiles(const QString& name, const QString& baseDir) const = 0;

    // Embedded skills
    virtual void registerEmbeddedSkill(const Skill& skill) = 0;
};

} // namespace sentinel::core
