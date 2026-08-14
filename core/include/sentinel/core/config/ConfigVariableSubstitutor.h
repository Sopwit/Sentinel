// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QMap>

namespace sentinel::core {

struct VariableSubstitutionConfig {
    bool allowEnv{true};
    bool allowFile{true};
    QStringList allowedPaths;
};

class ConfigVariableSubstitutor {
public:
    explicit ConfigVariableSubstitutor(const VariableSubstitutionConfig& config = {});

    QString substitute(const QString& input) const;
    QString substituteEnv(const QString& varName) const;
    QString substituteFile(const QString& filePath) const;
    bool validate(const QString& input, QString& error) const;

private:
    VariableSubstitutionConfig m_config;
};

} // namespace sentinel::core
