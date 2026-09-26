// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#include "sentinel/core/runtime/IFileSystemService.h"
#include "sentinel/core/runtime/SecureFileMutationBackend.h"
#include "sentinel/core/security/ExternalDirectoryGate.h"
#include "sentinel/core/security/PathGuard.h"
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QSaveFile>
#include <QFileInfo>
#include <QSet>
#include <algorithm>
#include <utility>
#if defined(Q_OS_UNIX)
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace sentinel::core {
namespace {
template <typename T> FileSystemResult<T> failure(FileSystemOperation operation,
    const QString& resource, FileSystemFailure reason, const QString& diagnostic = {}) {
    FileSystemResult<T> result;
    result.operation = operation;
    result.resource = resource;
    result.failure = reason;
    result.diagnostic = diagnostic;
    return result;
}
FileSystemFailure requiredType(const QFileInfo& info, bool directory) {
    if (!info.exists()) {
        if (info.isSymLink()) return FileSystemFailure::IOError;
        QFileInfo ancestor(info.absolutePath());
        while (!ancestor.exists() && ancestor.absolutePath() != ancestor.absoluteFilePath())
            ancestor = QFileInfo(ancestor.absolutePath());
        return ancestor.exists() && !ancestor.isReadable()
            ? FileSystemFailure::IOError : FileSystemFailure::NotFound;
    }
    if (directory && !info.isDir()) return info.isFile() ? FileSystemFailure::NotDirectory : FileSystemFailure::IOError;
    if (!directory && !info.isFile()) return info.isDir() ? FileSystemFailure::NotFile : FileSystemFailure::IOError;
    return FileSystemFailure::None;
}
bool sensitive(const QString& path) {
    static const QSet<QString> parts{QStringLiteral(".ssh"), QStringLiteral(".gnupg"),
        QStringLiteral(".aws"), QStringLiteral(".kube"), QStringLiteral(".password-store")};
    static const QSet<QString> files{QStringLiteral("id_rsa"), QStringLiteral("id_ed25519"),
        QStringLiteral("id_ecdsa"), QStringLiteral("id_dsa"), QStringLiteral(".git-credentials"),
        QStringLiteral(".npmrc"), QStringLiteral(".pypirc"), QStringLiteral(".netrc")};
    const QFileInfo info(path);
    if (files.contains(info.fileName())) return true;
    for (const auto& component : QDir::cleanPath(path).split(QLatin1Char('/')))
        if (parts.contains(component)) return true;
    return false;
}
FileSystemEntry entryOf(const QFileInfo& info) {
    return {info.fileName(), info.absoluteFilePath(), info.isDir(), info.size(), info.isFile()};
}
void captureIdentity(AuthorizedPath& path) {
#if defined(Q_OS_UNIX)
    struct stat info {};
    const QByteArray encoded = QFile::encodeName(path.canonicalPath);
    if (::lstat(encoded.constData(), &info) == 0) {
        path.existedAtAuthorization = true;
        path.deviceId = static_cast<quint64>(info.st_dev);
        path.fileId = static_cast<quint64>(info.st_ino);
    }
    QString parent = QFileInfo(path.canonicalPath).absolutePath();
    while (!parent.isEmpty()) {
        const QByteArray parentName = QFile::encodeName(parent);
        if (::lstat(parentName.constData(), &info) == 0) {
            if (S_ISDIR(info.st_mode)) {
                path.parentAnchorPath = parent;
                path.parentDeviceId = static_cast<quint64>(info.st_dev);
                path.parentFileId = static_cast<quint64>(info.st_ino);
            }
            break;
        }
        const QString next = QFileInfo(parent).absolutePath();
        if (next == parent) break;
        parent = next;
    }
#elif defined(Q_OS_WIN)
    securefs::captureIdentity(path);
#else
    path.existedAtAuthorization = QFileInfo::exists(path.canonicalPath);
#endif
}
}
FileSystemResult<AuthorizedPath> QtFileSystemService::resolve(const QString& raw,
        const QString& cwd, FileSystemAccess access) const {
    if (raw.trimmed().isEmpty())
        return failure<AuthorizedPath>(FileSystemOperation::Stat, raw, FileSystemFailure::InvalidPath);
    QString expanded = raw.trimmed();
    if (expanded == QLatin1String("~")) expanded = QDir::homePath();
    else if (expanded.startsWith(QLatin1String("~/"))) expanded = QDir::home().filePath(expanded.mid(2));
    if (QFileInfo(expanded).isRelative()) expanded = QDir(cwd).absoluteFilePath(expanded);
    if (QFileInfo(expanded).isSymLink() && !QFileInfo(expanded).exists())
        return failure<AuthorizedPath>(FileSystemOperation::Stat, QDir::cleanPath(expanded),
                                       FileSystemFailure::PermissionDenied);
    QString path;
    if (gate_) {
        path = gate_->resolvePath(expanded, cwd);
        if (path.isEmpty() || sensitive(path) ||
            !gate_->isAccessAllowed(path, cwd, access == FileSystemAccess::Write))
            return failure<AuthorizedPath>(FileSystemOperation::Stat, QDir::cleanPath(expanded),
                                           FileSystemFailure::PermissionDenied);
    } else {
        path = PathGuard::safePath(cwd, expanded);
        if (path.isEmpty() || sensitive(path))
            return failure<AuthorizedPath>(FileSystemOperation::Stat, QDir::cleanPath(expanded),
                                           FileSystemFailure::PermissionDenied);
    }
    FileSystemResult<AuthorizedPath> result;
    result.resource = path;
    result.value = AuthorizedPath{path, raw, access};
    captureIdentity(*result.value);
    return result;
}
FileSystemFailure QtFileSystemService::validateFinal(const AuthorizedPath& path, bool creating) const {
    if (path.canonicalPath.isEmpty() || sensitive(path.canonicalPath))
        return FileSystemFailure::SecurityBoundaryViolation;
    const QString current = PathGuard::canonicalPath(path.canonicalPath);
    if (current != path.canonicalPath)
        return FileSystemFailure::SymlinkEscape;
    const QFileInfo parent(QFileInfo(path.canonicalPath).absolutePath());
    if ((!parent.exists() && !creating) ||
        (parent.exists() && !parent.isDir()) ||
        PathGuard::canonicalPath(parent.absoluteFilePath()) != parent.absoluteFilePath() ||
        sensitive(parent.absoluteFilePath()))
        return FileSystemFailure::UnsafeParent;
    const QFileInfo target(path.canonicalPath);
    if (target.isSymLink())
        return FileSystemFailure::SymlinkEscape;
    if (path.existedAtAuthorization && !target.exists())
        return FileSystemFailure::ResourceChanged;
    if (!path.existedAtAuthorization && target.exists() && creating)
        return FileSystemFailure::ResourceChanged;
#if defined(Q_OS_UNIX)
    if (path.existedAtAuthorization) {
        struct stat info {};
        const QByteArray encoded = QFile::encodeName(path.canonicalPath);
        if (::lstat(encoded.constData(), &info) != 0 ||
            static_cast<quint64>(info.st_dev) != path.deviceId ||
            static_cast<quint64>(info.st_ino) != path.fileId)
            return FileSystemFailure::ResourceChanged;
    }
#endif
    return FileSystemFailure::None;
}
FileSystemResult<AuthorizedPath> QtFileSystemService::revalidateAuthorized(
    const AuthorizedPath& path, const QString& cwd) const {
    if (const auto security = validateFinal(path, true); security != FileSystemFailure::None)
        return failure<AuthorizedPath>(FileSystemOperation::Stat, path.displayPath, security);
    if (gate_ ? !gate_->isPathSafe(path.canonicalPath, cwd)
              : !PathGuard::contains(cwd, path.canonicalPath))
        return failure<AuthorizedPath>(FileSystemOperation::Stat, path.displayPath,
                                       FileSystemFailure::SecurityBoundaryViolation);
    FileSystemResult<AuthorizedPath> result;
    result.resource = path.canonicalPath;
    result.value = path;
    return result;
}
FileSystemResult<FileSystemEntry> QtFileSystemService::stat(const AuthorizedPath& path) const {
    if (const auto security = validateFinal(path); security != FileSystemFailure::None)
        return failure<FileSystemEntry>(FileSystemOperation::Stat, path.canonicalPath, security);
    const QFileInfo info(path.canonicalPath);
    if (!info.exists()) return failure<FileSystemEntry>(FileSystemOperation::Stat, path.canonicalPath,
                                                         requiredType(info, false));
    FileSystemResult<FileSystemEntry> result;
    result.resource = path.canonicalPath;
    result.value = entryOf(info);
    return result;
}
namespace {
void addIssue(TraversalStatus& status, const QString& resource, FileSystemFailure failure) {
    status.complete = false;
    if (status.issues.size() < 16) status.issues.append({resource, failure});
}
QList<QFileInfo> enumerateDirectory(const QString& directoryPath, bool includeHidden,
                                    const FileSystemOperationContext& context, int maxEntries,
                                    TraversalStatus& status) {
    QList<QFileInfo> entries;
    if (context.isCancelled()) {
        status.cancelled = true;
        status.complete = false;
        return entries;
    }
    const QFileInfo before(directoryPath);
    if (!before.exists() || !before.isDir() || !before.isReadable()) {
        addIssue(status, directoryPath, FileSystemFailure::IOError);
        return entries;
    }
    QDir::Filters filters = QDir::AllEntries | QDir::System | QDir::NoDotAndDotDot;
    if (includeHidden) filters |= QDir::Hidden;
    QDirIterator iterator(directoryPath, filters, QDirIterator::NoIteratorFlags);
    while (iterator.hasNext()) {
        if (context.isCancelled()) {
            status.cancelled = true;
            status.complete = false;
            break;
        }
        iterator.next();
        const QFileInfo entry = iterator.fileInfo();
        if (!entry.exists() && !entry.isSymLink()) {
            addIssue(status, entry.absoluteFilePath(), FileSystemFailure::IOError);
            continue;
        }
        entries.append(entry);
        if (entries.size() >= maxEntries && iterator.hasNext()) {
            status.complete = false;
            status.truncated = true;
            break;
        }
    }
    const QFileInfo after(directoryPath);
    if (!after.exists() || !after.isDir() || !after.isReadable() ||
        before.lastModified() != after.lastModified())
        addIssue(status, directoryPath, FileSystemFailure::IOError);
    std::sort(entries.begin(), entries.end(), [](const QFileInfo& a, const QFileInfo& b) {
        if (a.isDir() != b.isDir()) return a.isDir();
        return a.fileName() < b.fileName();
    });
    return entries;
}
}
FileSystemResult<DirectoryListing> QtFileSystemService::listDirectory(const AuthorizedPath& path,
        bool includeHidden, int limit, const FileSystemOperationContext& context) const {
    if (const auto security = validateFinal(path); security != FileSystemFailure::None)
        return failure<DirectoryListing>(FileSystemOperation::ListDirectory, path.canonicalPath, security);
    const QFileInfo info(path.canonicalPath);
    const auto reason = requiredType(info, true);
    if (reason != FileSystemFailure::None)
        return failure<DirectoryListing>(FileSystemOperation::ListDirectory, path.canonicalPath, reason);
    if (!info.isReadable())
        return failure<DirectoryListing>(FileSystemOperation::ListDirectory, path.canonicalPath, FileSystemFailure::IOError);
    DirectoryListing listing;
    listing.path = path.canonicalPath;
    listing.status.complete = true;
    const auto entries = enumerateDirectory(path.canonicalPath, includeHidden, context, 20000, listing.status);
    for (const auto& entry : entries) {
        if (context.isCancelled()) { listing.status.cancelled = true; listing.status.complete = false; }
        if (listing.entries.size() >= limit) {
            listing.status.truncated = true;
            break;
        }
        const auto allowed = resolve(entry.absoluteFilePath(), path.canonicalPath, FileSystemAccess::Read);
        if (!allowed.ok() || !PathGuard::contains(path.canonicalPath, allowed.value->canonicalPath)) {
            addIssue(listing.status, entry.absoluteFilePath(), allowed.ok()
                         ? FileSystemFailure::SymlinkEscape : allowed.failure);
            continue;
        }
        if (const auto security = validateFinal(*allowed.value); security != FileSystemFailure::None) {
            addIssue(listing.status, entry.absoluteFilePath(), security);
            continue;
        }
        listing.entries.append(entryOf(QFileInfo(allowed.value->canonicalPath)));
    }
    FileSystemResult<DirectoryListing> result;
    result.operation = FileSystemOperation::ListDirectory;
    result.resource = path.canonicalPath;
    result.cancelled = listing.status.cancelled;
    result.value = std::move(listing);
    return result;
}
FileSystemResult<FileRead> QtFileSystemService::readFile(const AuthorizedPath& path, qint64 maxBytes) const {
    if (const auto security = validateFinal(path); security != FileSystemFailure::None)
        return failure<FileRead>(FileSystemOperation::ReadFile, path.canonicalPath, security);
    const QFileInfo info(path.canonicalPath);
    const auto reason = requiredType(info, false);
    if (reason != FileSystemFailure::None)
        return failure<FileRead>(FileSystemOperation::ReadFile, path.canonicalPath, reason);
    QFile file(path.canonicalPath);
#if defined(Q_OS_UNIX)
    const QByteArray encoded = QFile::encodeName(path.canonicalPath);
    int fd = ::open(encoded.constData(), O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
    if (fd < 0)
        return failure<FileRead>(FileSystemOperation::ReadFile, path.canonicalPath, FileSystemFailure::ResourceChanged);
    struct stat opened {};
    if (::fstat(fd, &opened) != 0 ||
        (path.existedAtAuthorization &&
         (static_cast<quint64>(opened.st_dev) != path.deviceId ||
          static_cast<quint64>(opened.st_ino) != path.fileId))) {
        ::close(fd);
        return failure<FileRead>(FileSystemOperation::ReadFile, path.canonicalPath, FileSystemFailure::ResourceChanged);
    }
    if (!file.open(fd, QIODevice::ReadOnly, QFileDevice::AutoCloseHandle)) {
        ::close(fd);
        return failure<FileRead>(FileSystemOperation::ReadFile, path.canonicalPath, FileSystemFailure::ReadFailed);
    }
#else
    if (!file.open(QIODevice::ReadOnly))
        return failure<FileRead>(FileSystemOperation::ReadFile, path.canonicalPath, FileSystemFailure::ReadFailed, file.errorString());
#endif
    FileRead read;
    read.path = path.canonicalPath;
    read.content = file.read(maxBytes + 1);
    if (file.error() != QFileDevice::NoError)
        return failure<FileRead>(FileSystemOperation::ReadFile, path.canonicalPath, FileSystemFailure::ReadFailed, file.errorString());
    read.complete = read.content.size() <= maxBytes && file.atEnd();
    read.truncated = !read.complete;
    read.content.truncate(maxBytes);
    static const QSet<QString> binaryExtensions{
        QStringLiteral("zip"), QStringLiteral("exe"), QStringLiteral("so"), QStringLiteral("dll"),
        QStringLiteral("dylib"), QStringLiteral("wasm"), QStringLiteral("pyc"), QStringLiteral("jpg"),
        QStringLiteral("jpeg"), QStringLiteral("png"), QStringLiteral("gif"), QStringLiteral("webp"),
        QStringLiteral("pdf"), QStringLiteral("mp3"), QStringLiteral("mp4"), QStringLiteral("wav")};
    read.binary = binaryExtensions.contains(info.suffix().toLower()) || read.content.contains('\0');
    if (!read.binary && !read.content.isEmpty()) {
        int controls = 0;
        for (const char value : read.content) {
            const auto byte = static_cast<unsigned char>(value);
            if (byte < 9 || (byte > 13 && byte < 32)) ++controls;
        }
        read.binary = controls * 100 / read.content.size() > 30;
    }
    FileSystemResult<FileRead> result;
    result.operation = FileSystemOperation::ReadFile;
    result.resource = path.canonicalPath;
    result.value = std::move(read);
    return result;
}
FileSystemResult<FileWrite> QtFileSystemService::writeFile(const AuthorizedPath& path,
        const QByteArray& bytes, bool makeParents) const {
    if (path.access != FileSystemAccess::Write)
        return failure<FileWrite>(FileSystemOperation::WriteFile, path.canonicalPath, FileSystemFailure::PermissionDenied);
    if (const auto security = validateFinal(path, true); security != FileSystemFailure::None)
        return failure<FileWrite>(FileSystemOperation::WriteFile, path.canonicalPath, security);
#if defined(Q_OS_UNIX)
    return securefs::writeFile(path, bytes, makeParents);
#elif defined(Q_OS_WIN)
    return securefs::writeFile(path, bytes, makeParents);
#else
    const QFileInfo info(path.canonicalPath);
    if (PathGuard::canonicalPath(info.absolutePath()) != info.absolutePath() ||
        sensitive(info.absolutePath()))
        return failure<FileWrite>(FileSystemOperation::WriteFile, info.absolutePath(), FileSystemFailure::UnsafeParent);
    if (makeParents && !QDir().mkpath(info.absolutePath()))
        return failure<FileWrite>(FileSystemOperation::WriteFile, info.absolutePath(), FileSystemFailure::WriteFailed);
    if (const auto security = validateFinal(path, true); security != FileSystemFailure::None)
        return failure<FileWrite>(FileSystemOperation::WriteFile, path.canonicalPath, security);
    const auto parentReason = requiredType(QFileInfo(info.absolutePath()), true);
    if (parentReason != FileSystemFailure::None)
        return failure<FileWrite>(FileSystemOperation::WriteFile, info.absolutePath(), parentReason);
    if (info.exists() && !info.isFile())
        return failure<FileWrite>(FileSystemOperation::WriteFile, path.canonicalPath, FileSystemFailure::NotFile);
    const bool created = !info.exists();
    QSaveFile file(path.canonicalPath);
    if (!file.open(QIODevice::WriteOnly))
        return failure<FileWrite>(FileSystemOperation::WriteFile, path.canonicalPath, FileSystemFailure::WriteFailed, file.errorString());
    if (file.write(bytes) != bytes.size())
        return failure<FileWrite>(FileSystemOperation::WriteFile, path.canonicalPath, FileSystemFailure::WriteFailed, file.errorString());
    if (const auto security = validateFinal(path, true); security != FileSystemFailure::None)
        return failure<FileWrite>(FileSystemOperation::WriteFile, path.canonicalPath, security);
    if (!file.commit())
        return failure<FileWrite>(FileSystemOperation::WriteFile, path.canonicalPath, FileSystemFailure::WriteFailed, file.errorString());
    FileSystemResult<FileWrite> result;
    result.operation = FileSystemOperation::WriteFile;
    result.resource = path.canonicalPath;
    result.value = FileWrite{path.canonicalPath, bytes.size(), created};
    result.mutations.append({path.canonicalPath, created ? FileMutationKind::Created : FileMutationKind::Modified});
    return result;
#endif
}
FileSystemResult<bool> QtFileSystemService::deleteFile(const AuthorizedPath& path) const {
    if (path.access != FileSystemAccess::Write)
        return failure<bool>(FileSystemOperation::Delete, path.canonicalPath, FileSystemFailure::PermissionDenied);
    if (const auto security = validateFinal(path); security != FileSystemFailure::None)
        return failure<bool>(FileSystemOperation::Delete, path.canonicalPath, security);
#if defined(Q_OS_UNIX)
    return securefs::deleteFile(path);
#elif defined(Q_OS_WIN)
    return securefs::deleteFile(path);
#else
    const QFileInfo info(path.canonicalPath);
    const auto reason = requiredType(info, false);
    if (reason != FileSystemFailure::None)
        return failure<bool>(FileSystemOperation::Delete, path.canonicalPath, reason);
    if (const auto security = validateFinal(path); security != FileSystemFailure::None)
        return failure<bool>(FileSystemOperation::Delete, path.canonicalPath, security);
    if (!QFile::remove(path.canonicalPath))
        return failure<bool>(FileSystemOperation::Delete, path.canonicalPath, FileSystemFailure::IOError);
    FileSystemResult<bool> result;
    result.operation = FileSystemOperation::Delete;
    result.resource = path.canonicalPath;
    result.value = true;
    result.mutations.append({path.canonicalPath, FileMutationKind::Deleted});
    return result;
#endif
}
FileSystemResult<FileMove> QtFileSystemService::moveFile(const AuthorizedPath& source,
        const AuthorizedPath& destination) const {
    if (source.access != FileSystemAccess::Write || destination.access != FileSystemAccess::Write)
        return failure<FileMove>(FileSystemOperation::Move, source.canonicalPath, FileSystemFailure::PermissionDenied);
    if (const auto security = validateFinal(source); security != FileSystemFailure::None)
        return failure<FileMove>(FileSystemOperation::Move, source.canonicalPath, security);
#if defined(Q_OS_UNIX)
    if (const auto security = validateFinal(destination, true); security != FileSystemFailure::None)
        return failure<FileMove>(FileSystemOperation::Move, destination.canonicalPath, security);
    return securefs::moveFile(source, destination);
#elif defined(Q_OS_WIN)
    if (const auto security = validateFinal(destination, true); security != FileSystemFailure::None)
        return failure<FileMove>(FileSystemOperation::Move, destination.canonicalPath, security);
    return securefs::moveFile(source, destination);
#else
    const QFileInfo info(source.canonicalPath);
    const auto reason = requiredType(info, false);
    if (reason != FileSystemFailure::None)
        return failure<FileMove>(FileSystemOperation::Move, source.canonicalPath, reason);
    if (QFileInfo(destination.canonicalPath).exists())
        return failure<FileMove>(FileSystemOperation::Move, destination.canonicalPath, FileSystemFailure::AlreadyExists);
    if (PathGuard::canonicalPath(QFileInfo(destination.canonicalPath).absolutePath()) !=
        QFileInfo(destination.canonicalPath).absolutePath())
        return failure<FileMove>(FileSystemOperation::Move, destination.canonicalPath, FileSystemFailure::UnsafeParent);
    if (!QDir().mkpath(QFileInfo(destination.canonicalPath).absolutePath()))
        return failure<FileMove>(FileSystemOperation::Move, QFileInfo(destination.canonicalPath).absolutePath(), FileSystemFailure::WriteFailed);
    if (const auto security = validateFinal(destination, true); security != FileSystemFailure::None)
        return failure<FileMove>(FileSystemOperation::Move, destination.canonicalPath, security);
    if (const auto security = validateFinal(source); security != FileSystemFailure::None)
        return failure<FileMove>(FileSystemOperation::Move, source.canonicalPath, security);
    if (const auto security = validateFinal(destination, true); security != FileSystemFailure::None)
        return failure<FileMove>(FileSystemOperation::Move, destination.canonicalPath, security);
    if (!QFile::rename(source.canonicalPath, destination.canonicalPath))
        return failure<FileMove>(FileSystemOperation::Move, source.canonicalPath, FileSystemFailure::IOError);
    FileSystemResult<FileMove> result;
    result.operation = FileSystemOperation::Move;
    result.resource = destination.canonicalPath;
    result.value = FileMove{source.canonicalPath, destination.canonicalPath};
    result.mutations = {{source.canonicalPath, FileMutationKind::MovedFrom},
                        {destination.canonicalPath, FileMutationKind::MovedTo}};
    return result;
#endif
}
FileSystemResult<FileTraversal> QtFileSystemService::traverseFiles(const AuthorizedPath& root,
        bool includeHidden, int limit, const std::function<bool(const QString&)>& allowPath,
        const FileSystemOperationContext& context,
        const std::function<bool(const FileSystemEntry&)>& onFile) const {
    if (const auto security = validateFinal(root); security != FileSystemFailure::None)
        return failure<FileTraversal>(FileSystemOperation::Glob, root.canonicalPath, security);
    const QFileInfo rootInfo(root.canonicalPath);
    const auto reason = requiredType(rootInfo, true);
    if (reason != FileSystemFailure::None)
        return failure<FileTraversal>(FileSystemOperation::Glob, root.canonicalPath, reason);
    if (!rootInfo.isReadable())
        return failure<FileTraversal>(FileSystemOperation::Glob, root.canonicalPath, FileSystemFailure::IOError);
    FileTraversal traversal;
    traversal.status.complete = true;
    QSet<QString> visited;
    bool stopTraversal = false;
    std::function<void(const QString&, int)> walk = [&](const QString& directoryPath, int depth) {
        if (traversal.status.cancelled || stopTraversal) return;
        if (context.isCancelled()) {
            traversal.status.cancelled = true;
            traversal.status.complete = false;
            return;
        }
        if (depth > 128) {
            addIssue(traversal.status, directoryPath, FileSystemFailure::Unavailable);
            return;
        }
        const QFileInfo directoryInfo(directoryPath);
        const QString canonical = directoryInfo.canonicalFilePath();
        if (canonical.isEmpty() || canonical != directoryPath ||
            !PathGuard::contains(root.canonicalPath, canonical) || sensitive(canonical)) {
            addIssue(traversal.status, directoryPath, FileSystemFailure::SymlinkEscape);
            return;
        }
        if (visited.contains(canonical)) {
            addIssue(traversal.status, directoryPath, FileSystemFailure::IOError);
            return;
        }
        visited.insert(canonical);
        const auto entries = enumerateDirectory(directoryPath, includeHidden, context, 20000, traversal.status);
        const bool enumerationCapped = traversal.status.truncated;
        for (const auto& info : entries) {
            if (stopTraversal) break;
            if (context.isCancelled()) { traversal.status.cancelled = true; traversal.status.complete = false; break; }
            const QString child = info.absoluteFilePath();
            // Symlinks are outside the recursive search scope by contract.
            if (info.isSymLink()) continue;
            if (!allowPath(child)) {
                addIssue(traversal.status, child, FileSystemFailure::PermissionDenied);
                continue;
            }
            const auto allowed = resolve(child, root.canonicalPath, FileSystemAccess::Read);
            if (!allowed.ok() || !PathGuard::contains(root.canonicalPath, allowed.value->canonicalPath)) {
                addIssue(traversal.status, child, allowed.ok()
                             ? FileSystemFailure::SymlinkEscape : allowed.failure);
                continue;
            }
            if (const auto security = validateFinal(*allowed.value); security != FileSystemFailure::None) {
                addIssue(traversal.status, child, security);
                continue;
            }
            if (info.isDir()) { walk(child, depth + 1); continue; }
            if (!info.isFile()) {
                addIssue(traversal.status, child, FileSystemFailure::IOError);
                continue;
            }
            if (traversal.files.size() >= limit) {
                traversal.status.complete = false;
                traversal.status.truncated = true;
                stopTraversal = true;
                break;
            }
            const auto fileEntry = entryOf(info);
            traversal.files.append(fileEntry);
            if (onFile && !onFile(fileEntry)) {
                traversal.status.complete = false;
                if (context.isCancelled()) traversal.status.cancelled = true;
                else traversal.status.truncated = true;
                stopTraversal = true;
                break;
            }
        }
        if (enumerationCapped) stopTraversal = true;
    };
    walk(root.canonicalPath, 0);
    std::sort(traversal.files.begin(), traversal.files.end(), [](const auto& a, const auto& b) {
        return a.path < b.path;
    });
    FileSystemResult<FileTraversal> result;
    result.operation = FileSystemOperation::Glob;
    result.resource = root.canonicalPath;
    result.cancelled = traversal.status.cancelled;
    result.value = std::move(traversal);
    return result;
}
} // namespace sentinel::core
