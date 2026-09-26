#include "sentinel/core/runtime/SecureFileMutationBackend.h"

#if defined(Q_OS_UNIX)

#include "sentinel/core/security/PathGuard.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QUuid>

#include <cerrno>
#include <utility>
#include <cstdio>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#if defined(Q_OS_LINUX)
#include <sys/syscall.h>
#endif
#if defined(Q_OS_DARWIN)
#include <sys/stdio.h>
#endif

namespace sentinel::core::securefs {
namespace {

struct Fd {
    int value = -1;
    explicit Fd(int descriptor = -1) : value(descriptor) {}
    ~Fd() { if (value >= 0) ::close(value); }
    Fd(const Fd&) = delete;
    Fd& operator=(const Fd&) = delete;
    Fd(Fd&& other) noexcept : value(std::exchange(other.value, -1)) {}
    Fd& operator=(Fd&& other) noexcept {
        if (this != &other) {
            if (value >= 0) ::close(value);
            value = std::exchange(other.value, -1);
        }
        return *this;
    }
};

struct Parent {
    Fd fd;
    QByteArray leaf;
    FileSystemFailure failure = FileSystemFailure::None;
};

template <typename T>
FileSystemResult<T> failed(FileSystemOperation operation, const QString& resource,
                           FileSystemFailure reason) {
    FileSystemResult<T> result;
    result.operation = operation;
    result.resource = resource;
    result.failure = reason;
    return result;
}

FileSystemFailure pathError(int error) {
    if (error == ELOOP) return FileSystemFailure::SymlinkEscape;
    if (error == ENOTDIR) return FileSystemFailure::UnsafeParent;
    if (error == ENOENT) return FileSystemFailure::ResourceChanged;
    return FileSystemFailure::IOError;
}

bool sameIdentity(const struct stat& actual, quint64 device, quint64 file) {
    return static_cast<quint64>(actual.st_dev) == device &&
           static_cast<quint64>(actual.st_ino) == file;
}

Parent openParent(const AuthorizedPath& path, bool makeParents) {
    Parent result;
    const QString parentPath = QFileInfo(path.canonicalPath).absolutePath();
    result.leaf = QFile::encodeName(QFileInfo(path.canonicalPath).fileName());
    if (result.leaf.isEmpty() || result.leaf == "." || result.leaf == ".." ||
        path.parentAnchorPath.isEmpty() ||
        !PathGuard::contains(path.parentAnchorPath, parentPath)) {
        result.failure = FileSystemFailure::UnsafeParent;
        return result;
    }
    result.fd = Fd(::open("/", O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC));
    if (result.fd.value < 0) {
        result.failure = FileSystemFailure::UnsafeParent;
        return result;
    }
    bool anchored = false;
    auto checkAnchor = [&](const QString& currentPath) {
        if (currentPath != path.parentAnchorPath) return true;
        struct stat info {};
        if (::fstat(result.fd.value, &info) != 0 ||
            !sameIdentity(info, path.parentDeviceId, path.parentFileId)) {
            result.failure = FileSystemFailure::UnsafeParent;
            return false;
        }
        anchored = true;
        return true;
    };
    if (!checkAnchor(QStringLiteral("/"))) return result;
    QString current = QStringLiteral("/");
    for (const QString& component : parentPath.split(QLatin1Char('/'), Qt::SkipEmptyParts)) {
        const QByteArray name = QFile::encodeName(component);
        int next = ::openat(result.fd.value, name.constData(),
                            O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
        if (next < 0 && errno == ENOENT && makeParents && anchored) {
            if (::mkdirat(result.fd.value, name.constData(), 0700) != 0 && errno != EEXIST) {
                result.failure = pathError(errno);
                return result;
            }
            next = ::openat(result.fd.value, name.constData(),
                            O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
        }
        if (next < 0) {
            result.failure = pathError(errno);
            return result;
        }
        result.fd = Fd(next);
        current = QDir(current).filePath(component);
        if (!checkAnchor(current)) return result;
    }
    if (!anchored) result.failure = FileSystemFailure::UnsafeParent;
    return result;
}

FileSystemFailure checkTarget(const Parent& parent, const AuthorizedPath& path,
                              bool mustExist) {
    struct stat actual {};
    if (::fstatat(parent.fd.value, parent.leaf.constData(), &actual, AT_SYMLINK_NOFOLLOW) != 0) {
        if (errno == ENOENT)
            return mustExist || path.existedAtAuthorization
                ? FileSystemFailure::ResourceChanged : FileSystemFailure::None;
        return pathError(errno);
    }
    if (S_ISLNK(actual.st_mode)) return FileSystemFailure::SymlinkEscape;
    if (!S_ISREG(actual.st_mode)) return FileSystemFailure::ResourceChanged;
    if (!path.existedAtAuthorization ||
        !sameIdentity(actual, path.deviceId, path.fileId))
        return FileSystemFailure::ResourceChanged;
    return FileSystemFailure::None;
}

bool writeAll(int fd, const QByteArray& bytes) {
    qsizetype offset = 0;
    while (offset < bytes.size()) {
        const ssize_t count = ::write(fd, bytes.constData() + offset,
                                      static_cast<size_t>(bytes.size() - offset));
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) return false;
        offset += count;
    }
    return true;
}

} // namespace

FileSystemResult<FileWrite> writeFile(const AuthorizedPath& path, const QByteArray& bytes,
                                      bool makeParents) {
    auto parent = openParent(path, makeParents);
    if (parent.failure != FileSystemFailure::None)
        return failed<FileWrite>(FileSystemOperation::WriteFile, path.canonicalPath, parent.failure);
    if (const auto reason = checkTarget(parent, path, false); reason != FileSystemFailure::None)
        return failed<FileWrite>(FileSystemOperation::WriteFile, path.canonicalPath, reason);
    const bool created = !path.existedAtAuthorization;
    const QByteArray temporary = QByteArray(".sentinel-") +
        QUuid::createUuid().toString(QUuid::Id128).toLatin1();
    Fd output(::openat(parent.fd.value, temporary.constData(),
                       O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600));
    if (output.value < 0)
        return failed<FileWrite>(FileSystemOperation::WriteFile, path.canonicalPath,
                                 FileSystemFailure::WriteFailed);
    auto cleanup = [&] { ::unlinkat(parent.fd.value, temporary.constData(), 0); };
    if (!created) {
        struct stat existing {};
        if (::fstatat(parent.fd.value, parent.leaf.constData(), &existing,
                      AT_SYMLINK_NOFOLLOW) != 0 ||
            !sameIdentity(existing, path.deviceId, path.fileId) ||
            ::fchmod(output.value, existing.st_mode & 0777) != 0) {
            cleanup();
            return failed<FileWrite>(FileSystemOperation::WriteFile, path.canonicalPath,
                                     FileSystemFailure::ResourceChanged);
        }
    }
    if (!writeAll(output.value, bytes) || ::fsync(output.value) != 0) {
        cleanup();
        return failed<FileWrite>(FileSystemOperation::WriteFile, path.canonicalPath,
                                 FileSystemFailure::WriteFailed);
    }
    if (const auto reason = checkTarget(parent, path, false); reason != FileSystemFailure::None) {
        cleanup();
        return failed<FileWrite>(FileSystemOperation::WriteFile, path.canonicalPath, reason);
    }
    // linkat publishes a complete new file without replacing a target created meanwhile.
    const int renamed = created
        ? ::linkat(parent.fd.value, temporary.constData(), parent.fd.value,
                   parent.leaf.constData(), 0)
        : ::renameat(parent.fd.value, temporary.constData(), parent.fd.value,
                     parent.leaf.constData());
    if (renamed != 0) {
        const int error = errno;
        cleanup();
        return failed<FileWrite>(FileSystemOperation::WriteFile, path.canonicalPath,
                                 error == EEXIST ? FileSystemFailure::ResourceChanged
                                 : FileSystemFailure::WriteFailed);
    }
    if (created) cleanup();
    FileSystemResult<FileWrite> result;
    result.operation = FileSystemOperation::WriteFile;
    result.resource = path.canonicalPath;
    result.value = FileWrite{path.canonicalPath, bytes.size(), created};
    result.mutations.append({path.canonicalPath,
                             created ? FileMutationKind::Created : FileMutationKind::Modified});
    return result;
}

FileSystemResult<bool> deleteFile(const AuthorizedPath& path) {
    auto parent = openParent(path, false);
    if (parent.failure != FileSystemFailure::None)
        return failed<bool>(FileSystemOperation::Delete, path.canonicalPath, parent.failure);
    if (const auto reason = checkTarget(parent, path, true); reason != FileSystemFailure::None)
        return failed<bool>(FileSystemOperation::Delete, path.canonicalPath, reason);
    if (::unlinkat(parent.fd.value, parent.leaf.constData(), 0) != 0)
        return failed<bool>(FileSystemOperation::Delete, path.canonicalPath, pathError(errno));
    FileSystemResult<bool> result;
    result.operation = FileSystemOperation::Delete;
    result.resource = path.canonicalPath;
    result.value = true;
    result.mutations.append({path.canonicalPath, FileMutationKind::Deleted});
    return result;
}

FileSystemResult<FileMove> moveFile(const AuthorizedPath& source,
                                    const AuthorizedPath& destination) {
    auto from = openParent(source, false);
    if (from.failure != FileSystemFailure::None)
        return failed<FileMove>(FileSystemOperation::Move, source.canonicalPath, from.failure);
    auto to = openParent(destination, true);
    if (to.failure != FileSystemFailure::None)
        return failed<FileMove>(FileSystemOperation::Move, destination.canonicalPath, to.failure);
    if (const auto reason = checkTarget(from, source, true); reason != FileSystemFailure::None)
        return failed<FileMove>(FileSystemOperation::Move, source.canonicalPath, reason);
    if (const auto reason = checkTarget(to, destination, false); reason != FileSystemFailure::None)
        return failed<FileMove>(FileSystemOperation::Move, destination.canonicalPath, reason);
    if (destination.existedAtAuthorization)
        return failed<FileMove>(FileSystemOperation::Move, destination.canonicalPath,
                                FileSystemFailure::AlreadyExists);
    int renamed = -1;
#if defined(Q_OS_LINUX) && defined(SYS_renameat2)
    renamed = static_cast<int>(::syscall(SYS_renameat2, from.fd.value, from.leaf.constData(),
                                         to.fd.value, to.leaf.constData(), 1));
#elif defined(Q_OS_DARWIN) && defined(RENAME_EXCL)
    renamed = ::renameatx_np(from.fd.value, from.leaf.constData(),
                              to.fd.value, to.leaf.constData(), RENAME_EXCL);
#else
    renamed = ::renameat(from.fd.value, from.leaf.constData(),
                         to.fd.value, to.leaf.constData());
#endif
    if (renamed != 0)
        return failed<FileMove>(FileSystemOperation::Move, destination.canonicalPath,
                                errno == EEXIST ? FileSystemFailure::ResourceChanged
                                : pathError(errno));
    FileSystemResult<FileMove> result;
    result.operation = FileSystemOperation::Move;
    result.resource = destination.canonicalPath;
    result.value = FileMove{source.canonicalPath, destination.canonicalPath};
    result.mutations = {{source.canonicalPath, FileMutationKind::MovedFrom},
                        {destination.canonicalPath, FileMutationKind::MovedTo}};
    return result;
}

} // namespace sentinel::core::securefs

#endif
