#include "sentinel/core/runtime/SecureFileMutationBackend.h"
#include "sentinel/core/security/PathGuard.h"

#if defined(Q_OS_WIN)

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <QDir>
#include <QFileInfo>
#include <QUuid>

#include <cstring>
#include <algorithm>
#include <utility>

namespace sentinel::core::securefs {
namespace {

struct Handle {
    HANDLE value = INVALID_HANDLE_VALUE;
    explicit Handle(HANDLE handle = INVALID_HANDLE_VALUE) : value(handle) {}
    ~Handle() { if (valid()) CloseHandle(value); }
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
    Handle(Handle&& other) noexcept : value(std::exchange(other.value, INVALID_HANDLE_VALUE)) {}
    Handle& operator=(Handle&& other) noexcept {
        if (this != &other) {
            if (valid()) CloseHandle(value);
            value = std::exchange(other.value, INVALID_HANDLE_VALUE);
        }
        return *this;
    }
    bool valid() const { return value != INVALID_HANDLE_VALUE && value != nullptr; }
};

struct Parent {
    Handle handle;
    QString path;
    FileSystemFailure failure = FileSystemFailure::None;
};

template <typename T>
FileSystemResult<T> failed(FileSystemOperation operation, const QString& path,
                           FileSystemFailure reason) {
    FileSystemResult<T> result;
    result.operation = operation;
    result.resource = path;
    result.failure = reason;
    return result;
}

QString normalized(QString path) {
    path.replace(QLatin1Char('\\'), QLatin1Char('/'));
    if (path.startsWith(QLatin1String("//?/UNC/"), Qt::CaseInsensitive))
        path = QStringLiteral("//") + path.mid(8);
    else if (path.startsWith(QLatin1String("//?/")))
        path = path.mid(4);
    return QDir::cleanPath(path);
}

bool samePath(const QString& actual, const QString& expected) {
    return normalized(actual).compare(normalized(expected), Qt::CaseInsensitive) == 0;
}

QString finalPath(HANDLE handle) {
    const DWORD length = GetFinalPathNameByHandleW(handle, nullptr, 0,
                                                    FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
    if (length == 0 || length > 32767) return {};
    QString path(static_cast<qsizetype>(length + 1), Qt::Uninitialized);
    const DWORD written = GetFinalPathNameByHandleW(handle, path.data(), length + 1,
                                                     FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
    if (written == 0 || written > length) return {};
    path.resize(static_cast<qsizetype>(written));
    return normalized(path);
}

bool identity(HANDLE handle, quint64& device, quint64& file) {
    BY_HANDLE_FILE_INFORMATION info {};
    if (!GetFileInformationByHandle(handle, &info) ||
        (info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0)
        return false;
    device = info.dwVolumeSerialNumber;
    file = (static_cast<quint64>(info.nFileIndexHigh) << 32) | info.nFileIndexLow;
    return true;
}

bool matches(HANDLE handle, const QString& expected, quint64 device, quint64 file) {
    quint64 actualDevice = 0;
    quint64 actualFile = 0;
    return identity(handle, actualDevice, actualFile) &&
           actualDevice == device && actualFile == file &&
           samePath(finalPath(handle), expected);
}

Handle openPath(const QString& path, DWORD access, bool directory, DWORD share) {
    const std::wstring native = QDir::toNativeSeparators(path).toStdWString();
    return Handle(CreateFileW(native.c_str(), access, share, nullptr, OPEN_EXISTING,
                               FILE_FLAG_OPEN_REPARSE_POINT |
                                   (directory ? FILE_FLAG_BACKUP_SEMANTICS : 0),
                               nullptr));
}

FileSystemFailure winError(DWORD error) {
    if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND ||
        error == ERROR_SHARING_VIOLATION || error == ERROR_ALREADY_EXISTS ||
        error == ERROR_FILE_EXISTS)
        return FileSystemFailure::ResourceChanged;
    if (error == ERROR_CANT_ACCESS_FILE || error == ERROR_INVALID_REPARSE_DATA)
        return FileSystemFailure::SymlinkEscape;
    if (error == ERROR_ACCESS_DENIED) return FileSystemFailure::PermissionDenied;
    return FileSystemFailure::IOError;
}

Parent openParent(const AuthorizedPath& path, bool makeParents, bool destinationWrite = false) {
    Parent result;
    result.path = QFileInfo(path.canonicalPath).absolutePath();
    if (path.parentAnchorPath.isEmpty() ||
        !PathGuard::contains(path.parentAnchorPath, result.path)) {
        result.failure = FileSystemFailure::UnsafeParent;
        return result;
    }
    auto anchor = openPath(path.parentAnchorPath, FILE_READ_ATTRIBUTES, true,
                           FILE_SHARE_READ | FILE_SHARE_WRITE);
    if (!anchor.valid() || !matches(anchor.value, path.parentAnchorPath,
                                     path.parentDeviceId, path.parentFileId)) {
        result.failure = FileSystemFailure::UnsafeParent;
        return result;
    }
    if (!samePath(path.parentAnchorPath, result.path) && makeParents) {
        // Win32 has no CreateDirectoryW variant relative to a directory handle.
        // Reopen and verify the final parent before any file mutation.
        if (!QDir().mkpath(result.path)) {
            result.failure = FileSystemFailure::UnsafeParent;
            return result;
        }
    }
    result.handle = openPath(result.path, FILE_READ_ATTRIBUTES | FILE_TRAVERSE |
                             (destinationWrite ? FILE_ADD_FILE : 0), true,
                             FILE_SHARE_READ | FILE_SHARE_WRITE);
    quint64 unusedDevice = 0;
    quint64 unusedFile = 0;
    if (!result.handle.valid() ||
        !identity(result.handle.value, unusedDevice, unusedFile) ||
        !samePath(finalPath(result.handle.value), result.path) ||
        (samePath(result.path, path.parentAnchorPath) &&
         !matches(result.handle.value, result.path, path.parentDeviceId, path.parentFileId)))
        result.failure = FileSystemFailure::UnsafeParent;
    return result;
}

struct Target {
    Handle handle;
    bool exists = false;
    FileSystemFailure failure = FileSystemFailure::None;
};

Target openTarget(const AuthorizedPath& path, bool required, DWORD access,
                  DWORD share = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE) {
    Target target;
    target.handle = openPath(path.canonicalPath, access, false, share);
    if (!target.handle.valid()) {
        const DWORD error = GetLastError();
        if (error == ERROR_FILE_NOT_FOUND && !required && !path.existedAtAuthorization)
            return target;
        target.failure = winError(error);
        return target;
    }
    target.exists = true;
    quint64 device = 0;
    quint64 file = 0;
    if (!identity(target.handle.value, device, file)) {
        target.failure = FileSystemFailure::SymlinkEscape;
        return target;
    }
    BY_HANDLE_FILE_INFORMATION info {};
    if (!GetFileInformationByHandle(target.handle.value, &info) ||
        (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 ||
        !samePath(finalPath(target.handle.value), path.canonicalPath) ||
        !path.existedAtAuthorization || device != path.deviceId || file != path.fileId)
        target.failure = FileSystemFailure::ResourceChanged;
    return target;
}

bool renameHandle(HANDLE source, HANDLE destinationParent, const QString& leaf,
                  bool replace) {
    const std::wstring name = leaf.toStdWString();
    const DWORD bytes = static_cast<DWORD>(name.size() * sizeof(wchar_t));
    QByteArray storage(static_cast<qsizetype>(sizeof(FILE_RENAME_INFO) + bytes), 0);
    auto* info = reinterpret_cast<FILE_RENAME_INFO*>(storage.data());
    info->ReplaceIfExists = replace ? TRUE : FALSE;
    info->RootDirectory = destinationParent;
    info->FileNameLength = bytes;
    std::memcpy(info->FileName, name.data(), bytes);
    return SetFileInformationByHandle(source, FileRenameInfo, info,
                                       static_cast<DWORD>(storage.size())) != 0;
}

void discard(HANDLE handle) {
    FILE_DISPOSITION_INFO disposition {TRUE};
    SetFileInformationByHandle(handle, FileDispositionInfo, &disposition,
                                sizeof(disposition));
}

bool writeAll(HANDLE handle, const QByteArray& bytes) {
    qsizetype offset = 0;
    while (offset < bytes.size()) {
        DWORD written = 0;
        const DWORD count = static_cast<DWORD>(std::min<qsizetype>(bytes.size() - offset, 1 << 20));
        if (!WriteFile(handle, bytes.constData() + offset, count, &written, nullptr) ||
            written == 0)
            return false;
        offset += written;
    }
    return FlushFileBuffers(handle) != 0;
}

} // namespace

void captureIdentity(AuthorizedPath& path) {
    auto target = openPath(path.canonicalPath, FILE_READ_ATTRIBUTES, false,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE);
    if (!target.valid())
        target = openPath(path.canonicalPath, FILE_READ_ATTRIBUTES, true,
                          FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE);
    quint64 device = 0;
    quint64 file = 0;
    if (target.valid() && identity(target.value, device, file) &&
        samePath(finalPath(target.value), path.canonicalPath)) {
        path.existedAtAuthorization = true;
        path.deviceId = device;
        path.fileId = file;
    }
    QString parent = QFileInfo(path.canonicalPath).absolutePath();
    while (!parent.isEmpty()) {
        auto anchor = openPath(parent, FILE_READ_ATTRIBUTES, true,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE);
        if (anchor.valid() && identity(anchor.value, device, file) &&
            samePath(finalPath(anchor.value), parent)) {
            path.parentAnchorPath = parent;
            path.parentDeviceId = device;
            path.parentFileId = file;
            break;
        }
        const QString next = QFileInfo(parent).absolutePath();
        if (next == parent) break;
        parent = next;
    }
}

FileSystemResult<FileWrite> writeFile(const AuthorizedPath& path, const QByteArray& bytes,
                                      bool makeParents) {
    auto parent = openParent(path, makeParents, true);
    if (parent.failure != FileSystemFailure::None)
        return failed<FileWrite>(FileSystemOperation::WriteFile, path.canonicalPath, parent.failure);
    auto existing = openTarget(path, false, FILE_READ_ATTRIBUTES | DELETE);
    if (existing.failure != FileSystemFailure::None)
        return failed<FileWrite>(FileSystemOperation::WriteFile, path.canonicalPath, existing.failure);
    const bool created = !existing.exists;
    const QString temporary = QDir(parent.path).filePath(
        QStringLiteral(".sentinel-%1").arg(QUuid::createUuid().toString(QUuid::Id128)));
    const std::wstring native = QDir::toNativeSeparators(temporary).toStdWString();
    Handle output(CreateFileW(native.c_str(), GENERIC_WRITE | DELETE | FILE_READ_ATTRIBUTES,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT,
                              nullptr));
    if (!output.valid())
        return failed<FileWrite>(FileSystemOperation::WriteFile, path.canonicalPath,
                                 FileSystemFailure::WriteFailed);
    quint64 temporaryDevice = 0;
    quint64 temporaryFile = 0;
    if (!identity(output.value, temporaryDevice, temporaryFile) ||
        !samePath(finalPath(output.value), temporary) ||
        !samePath(finalPath(parent.handle.value), parent.path)) {
        discard(output.value);
        return failed<FileWrite>(FileSystemOperation::WriteFile, path.canonicalPath,
                                 FileSystemFailure::UnsafeParent);
    }
    if (!writeAll(output.value, bytes)) {
        discard(output.value);
        return failed<FileWrite>(FileSystemOperation::WriteFile, path.canonicalPath,
                                 FileSystemFailure::WriteFailed);
    }
    existing = openTarget(path, false, FILE_READ_ATTRIBUTES | DELETE);
    if (existing.failure != FileSystemFailure::None || existing.exists == created) {
        discard(output.value);
        return failed<FileWrite>(FileSystemOperation::WriteFile, path.canonicalPath,
                                 existing.failure == FileSystemFailure::None
                                     ? FileSystemFailure::ResourceChanged : existing.failure);
    }
    existing.handle = Handle();
    if (!samePath(finalPath(output.value), temporary) ||
        !samePath(finalPath(parent.handle.value), parent.path)) {
        discard(output.value);
        return failed<FileWrite>(FileSystemOperation::WriteFile, path.canonicalPath,
                                 FileSystemFailure::ResourceChanged);
    }
    if (!renameHandle(output.value, parent.handle.value,
                      QFileInfo(path.canonicalPath).fileName(), !created)) {
        const DWORD error = GetLastError();
        discard(output.value);
        return failed<FileWrite>(FileSystemOperation::WriteFile, path.canonicalPath,
                                 error == ERROR_ALREADY_EXISTS || error == ERROR_FILE_EXISTS
                                     ? FileSystemFailure::ResourceChanged : winError(error));
    }
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
    auto target = openTarget(path, true, FILE_READ_ATTRIBUTES | DELETE,
                             FILE_SHARE_READ | FILE_SHARE_WRITE);
    if (target.failure != FileSystemFailure::None)
        return failed<bool>(FileSystemOperation::Delete, path.canonicalPath, target.failure);
    if (!matches(target.handle.value, path.canonicalPath, path.deviceId, path.fileId) ||
        !samePath(finalPath(parent.handle.value), parent.path))
        return failed<bool>(FileSystemOperation::Delete, path.canonicalPath,
                            FileSystemFailure::ResourceChanged);
    FILE_DISPOSITION_INFO disposition {TRUE};
    if (!SetFileInformationByHandle(target.handle.value, FileDispositionInfo, &disposition,
                                    sizeof(disposition)))
        return failed<bool>(FileSystemOperation::Delete, path.canonicalPath,
                            winError(GetLastError()));
    target.handle = Handle();
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
    auto to = openParent(destination, true, true);
    if (to.failure != FileSystemFailure::None)
        return failed<FileMove>(FileSystemOperation::Move, destination.canonicalPath, to.failure);
    auto original = openTarget(source, true, FILE_READ_ATTRIBUTES | DELETE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE);
    if (original.failure != FileSystemFailure::None)
        return failed<FileMove>(FileSystemOperation::Move, source.canonicalPath, original.failure);
    auto target = openTarget(destination, false, FILE_READ_ATTRIBUTES);
    if (target.failure != FileSystemFailure::None)
        return failed<FileMove>(FileSystemOperation::Move, destination.canonicalPath, target.failure);
    if (target.exists)
        return failed<FileMove>(FileSystemOperation::Move, destination.canonicalPath,
                                FileSystemFailure::AlreadyExists);
    if (!matches(original.handle.value, source.canonicalPath, source.deviceId, source.fileId) ||
        !samePath(finalPath(from.handle.value), from.path) ||
        !samePath(finalPath(to.handle.value), to.path))
        return failed<FileMove>(FileSystemOperation::Move, source.canonicalPath,
                                FileSystemFailure::ResourceChanged);
    if (!renameHandle(original.handle.value, to.handle.value,
                      QFileInfo(destination.canonicalPath).fileName(), false))
        return failed<FileMove>(FileSystemOperation::Move, destination.canonicalPath,
                                winError(GetLastError()));
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
