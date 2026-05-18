#pragma once

#include "sentinel/core/IPathProvider.h"

namespace sentinel::core {

class StandardPathProvider final : public IPathProvider {
public:
    QString settingsFilePath() const override;
    QString memoryDatabasePath() const override;
    QString chatHistoryDatabasePath() const override;
    QString conversationExportDirectoryPath() const override;
};

} // namespace sentinel::core
