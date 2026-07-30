#ifndef SENTINEL_DESKTOP_PLATFORMINITIALIZER_H
#define SENTINEL_DESKTOP_PLATFORMINITIALIZER_H

#include <QGuiApplication>
#include <QString>
#include <QTranslator>

namespace sentinel::core {
class AppSettings;
}

namespace sentinel::desktop {

QString preferredUiFontFamily();
void configureDefaultUiFont();

QString effectiveLanguageCode(const sentinel::core::AppSettings& settings);
void installTranslator(QGuiApplication& app, QTranslator& translator, const QString& language);
void installStartupTranslator(QGuiApplication& app, const sentinel::core::AppSettings& settings,
                              QTranslator& translator);

void initializePlatformIntegrations(const QString& crashDumpPath);

} // namespace sentinel::desktop

#endif // SENTINEL_DESKTOP_PLATFORMINITIALIZER_H
