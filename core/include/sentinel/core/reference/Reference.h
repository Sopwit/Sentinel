// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJsonObject>
#include <QString>

namespace sentinel::core {

enum class ReferenceType : std::uint8_t { Repository, LocalPath, Url };

struct Reference {
    QString name;
    QString description;
    ReferenceType type{ReferenceType::LocalPath};
    QString path; // local path, repo URL, or web URL
    bool isAvailable{false};
    QString lastError;
    bool isValid() const {
        return !name.isEmpty() && !path.isEmpty();
    }
};

} // namespace sentinel::core
