// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/platform/IPathProvider.h"

namespace sentinel::core {

class StandardPathProvider final : public IPathProvider {
public:
    StandardPathProvider() = default;
    explicit StandardPathProvider(bool portableOverride);

    bool isPortable() const;
    void setPortable(bool portable);

    QString settingsFilePath() const override;
    QString memoryDatabasePath() const override;
    QString chatHistoryDatabasePath() const override;
    QString conversationDatabasePath() const override;
    QString conversationExportDirectoryPath() const override;
    QString localRagDatabasePath() const;
    QString logDirectoryPath() const;
    QString crashDumpDirectoryPath() const;

private:
    bool detectPortableMode() const;

    bool m_portableOverride{false};
    bool m_hasPortableOverride{false};
};

} // namespace sentinel::core

