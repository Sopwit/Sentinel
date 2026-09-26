#pragma once

#include "sentinel/core/runtime/IFileSystemService.h"

namespace sentinel::core::securefs {

#if defined(Q_OS_UNIX) || defined(Q_OS_WIN)
FileSystemResult<FileWrite> writeFile(const AuthorizedPath& path, const QByteArray& bytes,
                                      bool makeParents);
FileSystemResult<bool> deleteFile(const AuthorizedPath& path);
FileSystemResult<FileMove> moveFile(const AuthorizedPath& source,
                                    const AuthorizedPath& destination);
#endif

#if defined(Q_OS_WIN)
void captureIdentity(AuthorizedPath& path);
#endif

} // namespace sentinel::core::securefs
