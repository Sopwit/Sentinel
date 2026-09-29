// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QByteArray>
#include <QStringList>

namespace sentinel::core {

class AppSettings;
class IConversationStore;
class IMemoryStore;

enum class BackupError {
    None, ExportFailure, ImportValidationFailure, ImportConflict,
    UnsupportedBackupVersion, StoreFailure, ImportCommitFailure, ImportRollbackFailure
};
enum class ImportMode { Merge, ReplaceSelectedDomains };

struct BackupResult {
    BackupError error = BackupError::None;
    QByteArray data;
    QString detail;
    bool succeeded() const { return error == BackupError::None; }
};

// A bounded, versioned JSON document. Stores remain the persistence authorities.
class BackupService final {
public:
    BackupService(AppSettings& settings, IConversationStore* conversations = nullptr,
                  IMemoryStore* memory = nullptr);
    BackupResult exportJson(const QStringList& domains) const;
    BackupResult importJson(const QByteArray& data, const QStringList& selectedDomains,
                            ImportMode mode);

private:
    AppSettings& settings_;
    IConversationStore* conversations_;
    IMemoryStore* memory_;
};

} // namespace sentinel::core
