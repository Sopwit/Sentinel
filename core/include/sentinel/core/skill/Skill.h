// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJsonObject>
#include <QString>

namespace sentinel::core {

enum class SkillSourceType : std::uint8_t { Embedded, Directory, Url };

struct Skill {
    QString name;
    QString description;
    QString content;
    SkillSourceType sourceType{SkillSourceType::Embedded};
    QString sourceLocation;
    QString version;
    bool isValid() const {
        return !name.isEmpty() && !content.isEmpty();
    }
};

} // namespace sentinel::core
