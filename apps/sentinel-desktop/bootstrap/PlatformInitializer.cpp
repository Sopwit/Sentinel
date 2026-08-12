// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "PlatformInitializer.h"

#include "sentinel/core/app/AppSettings.h"
#include "sentinel/core/platform/WinCrashHandler.h"
#include "sentinel/core/platform/WinProtocolHandler.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFontDatabase>
#include <QLocale>

#if defined(Q_OS_WIN)
#ifndef WINVER
#define WINVER 0x0601
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#include <windows.h>
#include <shellapi.h>
#endif

namespace sentinel::desktop {

QString preferredUiFontFamily() {
#if defined(Q_OS_WIN)
    const QStringList preferredFamilies = {
        QStringLiteral("Segoe UI Variable"), QStringLiteral("Segoe UI"), QStringLiteral("Arial"),
        QStringLiteral("Noto Sans"), QStringLiteral("Helvetica Neue")};
#elif defined(Q_OS_MACOS)
    const QStringList preferredFamilies = {
        QStringLiteral("SF Pro Display"), QStringLiteral("SF Pro Text"),
        QStringLiteral("Helvetica Neue"), QStringLiteral("Noto Sans"), QStringLiteral("Arial")};
#else
    const QStringList preferredFamilies = {
        QStringLiteral("Noto Sans"), QStringLiteral("DejaVu Sans"), QStringLiteral("Ubuntu"),
        QStringLiteral("Segoe UI"),  QStringLiteral("Helvetica Neue"), QStringLiteral("Arial")};
#endif

    for (const QString& family : preferredFamilies) {
        if (QFontDatabase::hasFamily(family)) {
            return family;
        }
    }

    return QString();
}

void configureDefaultUiFont() {
    const QString family = preferredUiFontFamily();
    if (family.isEmpty()) {
        return;
    }

    QFont font = QGuiApplication::font();
    if (font.family().compare(family, Qt::CaseInsensitive) == 0) {
        return;
    }

    font.setFamily(family);
    QGuiApplication::setFont(font);
}

QString effectiveLanguageCode(const sentinel::core::AppSettings& settings) {
    const auto configured = settings.appLanguage();
    const auto systemLanguage = QLocale::system().name().left(2).toLower();
    const QStringList supported = {
        QStringLiteral("en"), QStringLiteral("tr"), QStringLiteral("de"),
        QStringLiteral("es"), QStringLiteral("fr"), QStringLiteral("zh"),
        QStringLiteral("ja"), QStringLiteral("ar")
    };
    if (supported.contains(configured)) {
        return configured;
    }
    return supported.contains(systemLanguage) ? systemLanguage : QStringLiteral("en");
}

void installTranslator(QGuiApplication& app, QTranslator& translator, const QString& language) {
    app.removeTranslator(&translator);
    // English also installs sentinel_en.qm so that non-English source strings
    // (e.g. HomeChatSurface greeting/example chips) render in English.
    if (translator.load(QStringLiteral(":/i18n/sentinel_%1.qm").arg(language))) {
        app.installTranslator(&translator);
    }
}

void installStartupTranslator(QGuiApplication& app, const sentinel::core::AppSettings& settings,
                              QTranslator& translator) {
    installTranslator(app, translator, effectiveLanguageCode(settings));
}

void initializePlatformIntegrations(const QString& crashDumpPath) {
    sentinel::core::installWinCrashHandler(crashDumpPath);

#if defined(Q_OS_WIN)
    sentinel::core::registerSentinelProtocol();

    using SetCurrentProcessExplicitAppUserModelIDProc =
        HRESULT(WINAPI*)(PCWSTR AppID);
    auto setAppUserModelId =
        reinterpret_cast<SetCurrentProcessExplicitAppUserModelIDProc>(
            ::GetProcAddress(::GetModuleHandleW(L"shell32.dll"),
                             "SetCurrentProcessExplicitAppUserModelID"));
    if (setAppUserModelId) {
        HRESULT hr = setAppUserModelId(L"dev.sentinel.Sentinel");
        if (FAILED(hr)) {
            qWarning().noquote() << "Failed to set AppUserModelID:" << hr;
        }
    } else {
        qWarning().noquote() << "SetCurrentProcessExplicitAppUserModelID not available";
    }

    HKEY hKey;
    LSTATUS regStatus = RegOpenKeyExW(HKEY_CURRENT_USER,
                                       L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                                       0, KEY_SET_VALUE, &hKey);
    if (regStatus == ERROR_SUCCESS) {
        const QString appPath = QCoreApplication::applicationFilePath();
        const std::wstring appPathW = appPath.toStdWString();
        RegSetValueExW(hKey, L"Sentinel Desktop", 0, REG_SZ,
                       reinterpret_cast<const BYTE*>(appPathW.c_str()),
                       static_cast<DWORD>((appPathW.size() + 1) * sizeof(wchar_t)));
        RegCloseKey(hKey);
    }
#endif
}

} // namespace sentinel::desktop
