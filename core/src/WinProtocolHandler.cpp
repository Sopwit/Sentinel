#include "sentinel/core/WinProtocolHandler.h"

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

    // Notify Windows that the protocol association changed
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);

    qInfo().noquote() << "Sentinel protocol handler registered (sentinel://)";
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
