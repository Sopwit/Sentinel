// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "sentinel/core/agent/ObservationEvidence.h"
#include <QByteArray>
#include <QList>
#include <QString>
#include <functional>
#include <atomic>
#include <memory>
#include <optional>

namespace sentinel::core {

class ExternalDirectoryGate;

enum class FileSystemAccess { Read, Write };
enum class FileMutationKind { Created, Modified, Deleted, MovedFrom, MovedTo };
struct FileMutation {
    QString path;
    FileMutationKind kind = FileMutationKind::Modified;
};
struct FileSystemOperationContext {
    std::shared_ptr<std::atomic_bool> cancellation;
    std::shared_ptr<std::atomic_bool> toolCancellation;
    bool isCancelled() const {
        return (cancellation && cancellation->load()) ||
               (toolCancellation && toolCancellation->load());
    }
};
struct TraversalIssue { QString resource; FileSystemFailure failure = FileSystemFailure::IOError; };
struct TraversalStatus {
    bool complete = false;
    bool truncated = false;
    bool cancelled = false;
    QList<TraversalIssue> issues;
};
struct AuthorizedPath {
    QString canonicalPath;
    QString displayPath;
    FileSystemAccess access = FileSystemAccess::Read;
};
template <typename T> struct FileSystemResult {
    std::optional<T> value;
    FileSystemFailure failure = FileSystemFailure::None;
    FileSystemOperation operation = FileSystemOperation::Stat;
    QString resource;
    QString diagnostic;
    QList<FileMutation> mutations;
    bool cancelled = false;
    bool ok() const { return failure == FileSystemFailure::None && value.has_value(); }
};
struct FileSystemEntry {
    QString name;
    QString path;
    bool directory = false;
    qint64 size = 0;
    bool regularFile = false;
};
struct DirectoryListing { QString path; QList<FileSystemEntry> entries; TraversalStatus status; };
struct FileRead { QString path; QByteArray content; bool complete = false; bool truncated = false; bool binary = false; };
struct FileWrite { QString path; qint64 bytesWritten = 0; bool created = false; };
struct FileTraversal { QList<FileSystemEntry> files; TraversalStatus status; };
struct FileMove { QString source; QString destination; };

// Local operations receive only paths resolved through the existing authorization gate.
// Keep Qt filesystem access and failure classification in the concrete service.
class IFileSystemService {
public:
    virtual ~IFileSystemService() = default;
    virtual FileSystemResult<AuthorizedPath> resolve(const QString& raw, const QString& cwd,
                                                       FileSystemAccess access) const = 0;
    virtual FileSystemResult<AuthorizedPath> revalidateAuthorized(
        const AuthorizedPath& path, const QString& cwd) const {
        return resolve(path.canonicalPath, cwd, path.access);
    }
    virtual FileSystemResult<FileSystemEntry> stat(const AuthorizedPath& path) const = 0;
    virtual FileSystemResult<DirectoryListing> listDirectory(const AuthorizedPath& path,
                                                               bool includeHidden, int limit, const FileSystemOperationContext& context = {}) const = 0;
    virtual FileSystemResult<FileRead> readFile(const AuthorizedPath& path, qint64 maxBytes) const = 0;
    virtual FileSystemResult<FileWrite> writeFile(const AuthorizedPath& path,
                                                   const QByteArray& bytes, bool makeParents = false) const = 0;
    virtual FileSystemResult<bool> deleteFile(const AuthorizedPath& path) const = 0;
    virtual FileSystemResult<FileMove> moveFile(const AuthorizedPath& source,
                                                 const AuthorizedPath& destination) const = 0;
    virtual FileSystemResult<FileTraversal> traverseFiles(
        const AuthorizedPath& root, bool includeHidden, int limit,
        const std::function<bool(const QString&)>& allowPath,
        const FileSystemOperationContext& context = {},
        const std::function<bool(const FileSystemEntry&)>& onFile = {}) const = 0;
};

class QtFileSystemService final : public IFileSystemService {
public:
    explicit QtFileSystemService(const ExternalDirectoryGate* gate = nullptr) : gate_(gate) {}
    void setExternalDirectoryGate(const ExternalDirectoryGate* gate) { gate_ = gate; }
    FileSystemResult<AuthorizedPath> resolve(const QString& raw, const QString& cwd,
                                               FileSystemAccess access) const override;
    FileSystemResult<AuthorizedPath> revalidateAuthorized(
        const AuthorizedPath& path, const QString& cwd) const override;
    FileSystemResult<FileSystemEntry> stat(const AuthorizedPath& path) const override;
    FileSystemResult<DirectoryListing> listDirectory(const AuthorizedPath& path,
                                                       bool includeHidden, int limit, const FileSystemOperationContext& context = {}) const override;
    FileSystemResult<FileRead> readFile(const AuthorizedPath& path, qint64 maxBytes) const override;
    FileSystemResult<FileWrite> writeFile(const AuthorizedPath& path,
                                           const QByteArray& bytes, bool makeParents = false) const override;
    FileSystemResult<bool> deleteFile(const AuthorizedPath& path) const override;
    FileSystemResult<FileMove> moveFile(const AuthorizedPath& source,
                                         const AuthorizedPath& destination) const override;
    FileSystemResult<FileTraversal> traverseFiles(
        const AuthorizedPath& root, bool includeHidden, int limit,
        const std::function<bool(const QString&)>& allowPath,
        const FileSystemOperationContext& context = {},
        const std::function<bool(const FileSystemEntry&)>& onFile = {}) const override;
private:
    const ExternalDirectoryGate* gate_ = nullptr;
};

} // namespace sentinel::core
