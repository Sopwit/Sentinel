#pragma once

#include "sentinel/core/IPathProvider.h"

namespace sentinel::core {

class StandardPathProvider final : public IPathProvider {
public:
    QString settingsFilePath() const override;
    QString memoryDatabasePath() const override;
    QString chatHistoryDatabasePath() const override;
    QString conversationDatabasePath() const override;
    QString conversationExportDirectoryPath() const override;
    QString localRagDatabasePath() const;
};

} // namespace sentinel::core
