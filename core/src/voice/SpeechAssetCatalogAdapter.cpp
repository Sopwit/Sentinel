// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#include "sentinel/core/voice/SpeechAssetCatalogAdapter.h"
#include <QFileInfo>

namespace sentinel::core {
void SpeechAssetCatalogAdapter::setAsset(const QString& runtimeId, const QString& path) {
    if (runtimeId != QLatin1String("whisper.cpp") && runtimeId != QLatin1String("piper") &&
        runtimeId != QLatin1String("kokoro") &&
        runtimeId != QLatin1String("kokoro-voices")) return;
    if (path.trimmed().isEmpty() || path == QLatin1String("not configured"))
        paths_.remove(runtimeId);
    else paths_.insert(runtimeId, path.trimmed());
}
QList<ModelLibraryEntry> SpeechAssetCatalogAdapter::entries() const {
    QList<ModelLibraryEntry> result;
    for (auto it = paths_.cbegin(); it != paths_.cend(); ++it) {
        const QFileInfo file(it.value());
        ModelLibraryEntry entry;
        const QString runtime = it.key() == QLatin1String("kokoro-voices")
            ? QStringLiteral("kokoro") : it.key();
        entry.provider = {QStringLiteral("speech-%1").arg(runtime), runtime, ProviderKind::Local};
        entry.runtime = {runtime, runtime};
        entry.source = {sourceId(), QStringLiteral("Configured speech assets"),
                        QStringLiteral("user-configured-local"), {}};
        entry.artifactId = QStringLiteral("speech:%1:%2").arg(it.key(), it.value());
        entry.displayName = file.fileName().isEmpty() ? it.key() : file.fileName();
        entry.family = runtime;
        if (it.key() == QLatin1String("whisper.cpp"))
            entry.capabilities.audioInput = CapabilitySupport::Supported;
        else
            entry.capabilities.audioOutput = CapabilitySupport::Supported;
        entry.local = true;
        entry.localFile = file.exists() ? file.canonicalFilePath() : it.value();
        entry.installed = file.exists() && file.isReadable()
            ? ModelLibraryInstalledState::Installed : ModelLibraryInstalledState::NotInstalled;
        entry.availability = file.exists() && file.isReadable()
            ? ModelLibraryAvailability::Available : ModelLibraryAvailability::Unavailable;
        entry.installation = ModelInstallationStrategy::ExternalApplicationManaged;
        entry.catalog = ModelLibraryCatalogState::ConfiguredOnly;
        entry.catalogDetail = QStringLiteral("Configured local speech asset; runtime readiness is separate");
        entry.sourceProvenance = entry.source.provenance;
        if (file.isFile()) entry.sizeBytes = file.size();
        result.append(entry);
    }
    return result;
}
} // namespace sentinel::core
