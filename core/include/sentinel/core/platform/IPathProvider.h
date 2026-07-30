// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>

namespace sentinel::core {

class IPathProvider {
public:
    virtual ~IPathProvider() = default;

    virtual QString settingsFilePath() const = 0;
    virtual QString memoryDatabasePath() const = 0;
    virtual QString chatHistoryDatabasePath() const = 0;
    virtual QString conversationDatabasePath() const = 0;
    virtual QString conversationExportDirectoryPath() const = 0;
};

} // namespace sentinel::core
