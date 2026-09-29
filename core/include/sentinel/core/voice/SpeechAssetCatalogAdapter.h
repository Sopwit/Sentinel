// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "sentinel/core/model/ModelLibrary.h"
#include <QMap>

namespace sentinel::core {

// Read-only bridge to the existing Model Library. Speech assets remain externally
// configured; ModelOperationService and ModelStorageManager retain download authority.
class SpeechAssetCatalogAdapter final : public IModelLibrarySourceAdapter {
public:
    QString sourceId() const override { return QStringLiteral("configured-speech-assets"); }
    QString providerId() const override { return {}; }
    QList<ModelLibraryEntry> entries() const override;
    void setAsset(const QString& runtimeId, const QString& path);
private:
    QMap<QString, QString> paths_;
};

} // namespace sentinel::core
