// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJsonObject>
#include <QString>
#include <QStringList>

namespace sentinel::core {

enum class SkillSourceType : std::uint8_t { Embedded, Directory, Url };
enum class SkillState : std::uint8_t { Enabled, Disabled, Failed, Incompatible, MissingRequirements };
enum class SkillScope : std::uint8_t { Global, Workspace, GlobalWithWorkspaceOverride };

struct Skill {
    QString name;
    QString description;
    QString content;
    SkillSourceType sourceType{SkillSourceType::Embedded};
    QString sourceLocation;
    QString indexSource;
    QString version;
    QString author;
    SkillState state{SkillState::Enabled};
    bool enabledPreference{true};
    SkillScope scope{SkillScope::Global};
    QString workspaceId;
    QStringList requiredTools;
    QStringList requiredProviders;
    QStringList requiredCapabilities;
    QStringList missingRequirements;
    QString lastError;
    bool isValid() const {
        return !name.isEmpty() && !content.isEmpty();
    }
};

} // namespace sentinel::core
