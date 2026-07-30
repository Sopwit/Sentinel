#include "sentinel/core/platform/WinProtocolHandler.h"

#include <QCoreApplication>
#include <QDebug>
#include <QUrl>

#if defined(Q_OS_WIN)
#include <windows.h>
#include <shlobj.h>
#endif

namespace sentinel::core {

#if defined(Q_OS_WIN)

void registerSentinelProtocol() {
    const QString appPath = QCoreApplication::applicationFilePath();
    const std::wstring appPathW = appPath.toStdWString();

    // HKCU\Software\Classes\sentinel
    HKEY hKey = nullptr;
    LSTATUS status = RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Classes\\sentinel", 0,
                                      nullptr, 0, KEY_SET_VALUE, nullptr, &hKey, nullptr);
    if (status == ERROR_SUCCESS) {
        RegSetValueExW(hKey, nullptr, 0, REG_SZ,
                       reinterpret_cast<const BYTE*>(L"URL:Sentinel Protocol"),
                       sizeof(L"URL:Sentinel Protocol"));
        RegSetValueExW(hKey, L"URL Protocol", 0, REG_SZ,
                       reinterpret_cast<const BYTE*>(L""), sizeof(L""));
        RegCloseKey(hKey);
    }

    // HKCU\Software\Classes\sentinel\DefaultIcon
    HKEY hIcon = nullptr;
    status = RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Classes\\sentinel\\DefaultIcon",
                              0, nullptr, 0, KEY_SET_VALUE, nullptr, &hIcon, nullptr);
    if (status == ERROR_SUCCESS) {
        std::wstring iconPath = appPathW + L",0";
        RegSetValueExW(hIcon, nullptr, 0, REG_SZ,
                       reinterpret_cast<const BYTE*>(iconPath.c_str()),
                       static_cast<DWORD>((iconPath.size() + 1) * sizeof(wchar_t)));
        RegCloseKey(hIcon);
    }

    // HKCU\Software\Classes\sentinel\shell\open\command
    HKEY hCmd = nullptr;
    status = RegCreateKeyExW(HKEY_CURRENT_USER,
                              L"Software\\Classes\\sentinel\\shell\\open\\command",
                              0, nullptr, 0, KEY_SET_VALUE, nullptr, &hCmd, nullptr);
    if (status == ERROR_SUCCESS) {
        std::wstring cmdLine = L"\"" + appPathW + L"\" \"%1\"";
        RegSetValueExW(hCmd, nullptr, 0, REG_SZ,
                       reinterpret_cast<const BYTE*>(cmdLine.c_str()),
                       static_cast<DWORD>((cmdLine.size() + 1) * sizeof(wchar_t)));
        RegCloseKey(hCmd);
    }

    // 1. File Association: .sentinel
    HKEY hExt = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Classes\\.sentinel", 0, nullptr, 0, KEY_SET_VALUE, nullptr, &hExt, nullptr) == ERROR_SUCCESS) {
        RegSetValueExW(hExt, nullptr, 0, REG_SZ, reinterpret_cast<const BYTE*>(L"sentinelfile"), sizeof(L"sentinelfile"));
        RegCloseKey(hExt);
    }
    HKEY hFile = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Classes\\sentinelfile\\shell\\open\\command", 0, nullptr, 0, KEY_SET_VALUE, nullptr, &hFile, nullptr) == ERROR_SUCCESS) {
        std::wstring fileCmd = L"\"" + appPathW + L"\" \"%1\"";
        RegSetValueExW(hFile, nullptr, 0, REG_SZ, reinterpret_cast<const BYTE*>(fileCmd.c_str()), static_cast<DWORD>((fileCmd.size() + 1) * sizeof(wchar_t)));
        RegCloseKey(hFile);
    }

    // 2. Explorer Context Menu: "Open with Sentinel"
    HKEY hCtx = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Classes\\*\\shell\\Sentinel", 0, nullptr, 0, KEY_SET_VALUE, nullptr, &hCtx, nullptr) == ERROR_SUCCESS) {
        RegSetValueExW(hCtx, nullptr, 0, REG_SZ, reinterpret_cast<const BYTE*>(L"Open with Sentinel"), sizeof(L"Open with Sentinel"));
        std::wstring iconVal = appPathW + L",0";
        RegSetValueExW(hCtx, L"Icon", 0, REG_SZ, reinterpret_cast<const BYTE*>(iconVal.c_str()), static_cast<DWORD>((iconVal.size() + 1) * sizeof(wchar_t)));
        RegCloseKey(hCtx);
    }
    HKEY hCtxCmd = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Classes\\*\\shell\\Sentinel\\command", 0, nullptr, 0, KEY_SET_VALUE, nullptr, &hCtxCmd, nullptr) == ERROR_SUCCESS) {
        std::wstring ctxCmd = L"\"" + appPathW + L"\" \"%1\"";
        RegSetValueExW(hCtxCmd, nullptr, 0, REG_SZ, reinterpret_cast<const BYTE*>(ctxCmd.c_str()), static_cast<DWORD>((ctxCmd.size() + 1) * sizeof(wchar_t)));
        RegCloseKey(hCtxCmd);
    }

    // 3. Install Path & Version Registry Entry
    HKEY hAppMeta = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Sopwit\\Sentinel Desktop", 0, nullptr, 0, KEY_SET_VALUE, nullptr, &hAppMeta, nullptr) == ERROR_SUCCESS) {
        std::wstring versionStr = L"1.0.0";
        RegSetValueExW(hAppMeta, L"Version", 0, REG_SZ, reinterpret_cast<const BYTE*>(versionStr.c_str()), static_cast<DWORD>((versionStr.size() + 1) * sizeof(wchar_t)));
        RegSetValueExW(hAppMeta, L"InstallPath", 0, REG_SZ, reinterpret_cast<const BYTE*>(appPathW.c_str()), static_cast<DWORD>((appPathW.size() + 1) * sizeof(wchar_t)));
        RegCloseKey(hAppMeta);
    }

    // Notify Windows that shell associations changed
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);

    qInfo().noquote() << "Sentinel protocol, file association, context menu, and registry metadata registered";
}

#else

void registerSentinelProtocol() {
    // No-op on non-Windows
}

#endif

QString extractSentinelUrl(const QStringList& args) {
    const QString prefix = QStringLiteral("sentinel://");
    for (const auto& arg : args) {
        if (arg.startsWith(prefix, Qt::CaseInsensitive)) {
            return arg;
        }
    }
    return {};
}

} // namespace sentinel::core
