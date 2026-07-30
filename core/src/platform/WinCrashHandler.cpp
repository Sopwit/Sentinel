#include "sentinel/core/platform/WinCrashHandler.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QFileInfoList>

#if defined(Q_OS_WIN)
// clang-format off
#include <windows.h>
#include <dbghelp.h>
// clang-format on
#endif

namespace sentinel::core {

#if defined(Q_OS_WIN)

static QString crashDumpDir_;
static LPTOP_LEVEL_EXCEPTION_FILTER previousFilter_ = nullptr;

static LONG WINAPI sentinelExceptionHandler(EXCEPTION_POINTERS* exceptionInfo) {
    const QString dumpPath = QDir(crashDumpDir_)
                                 .filePath(QStringLiteral("sentinel-crash-%1.dmp")
                                               .arg(QDateTime::currentDateTime().toString(
                                                   QStringLiteral("yyyyMMdd-hhmmss-zzz"))));

    HANDLE hFile = CreateFileW(dumpPath.toStdWString().c_str(), GENERIC_WRITE, 0, nullptr,
                               CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION mei;
        mei.ThreadId = GetCurrentThreadId();
        mei.ExceptionPointers = exceptionInfo;
        mei.ClientPointers = FALSE;

        MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile,
                          MINIDUMP_TYPE(MiniDumpWithDataSegs | MiniDumpWithIndirectlyReferencedMemory),
                          exceptionInfo ? &mei : nullptr, nullptr, nullptr);
        CloseHandle(hFile);
    }

    // Call previous filter if any
    if (previousFilter_) {
        return previousFilter_(exceptionInfo);
    }
    return EXCEPTION_EXECUTE_HANDLER;
}

void installWinCrashHandler(const QString& crashDumpDir) {
    crashDumpDir_ = crashDumpDir;

    QDir dir(crashDumpDir_);
    if (!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    }

    previousFilter_ = SetUnhandledExceptionFilter(sentinelExceptionHandler);
}

#else

void installWinCrashHandler(const QString&) {}

#endif

bool hasPendingCrashDump(const QString& crashDumpDir) {
    return !pendingCrashDumps(crashDumpDir).isEmpty();
}

QStringList pendingCrashDumps(const QString& crashDumpDir) {
    QDir dir(crashDumpDir);
    if (!dir.exists()) {
        return {};
    }

    QStringList result;
    const auto files = dir.entryInfoList(QStringList{QStringLiteral("sentinel-crash-*.dmp")},
                                         QDir::Files, QDir::Time);
    for (const auto& fi : files) {
        result.append(fi.absoluteFilePath());
    }
    return result;
}

void acknowledgeCrashDump(const QString& dumpPath) {
    QFile::remove(dumpPath);
}

} // namespace sentinel::core
