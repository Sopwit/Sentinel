// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "LoggingInitializer.h"

#include "sentinel/core/app/FileLogger.h"

#include <QLoggingCategory>
#include <cstdio>
#include <cstdlib>

namespace sentinel::desktop {

void configureLogging(bool verbose, bool quiet, const QString& logDir) {
    if (quiet) {
        QLoggingCategory::setFilterRules(
            QStringLiteral("*.debug=false\n*.info=false\n*.warning=false"));
        return;
    }

    if (!verbose && qgetenv("QT_LOGGING_RULES").isEmpty()) {
        QLoggingCategory::setFilterRules(QStringLiteral("*.debug=false\n"
                                                        "*.info=false\n"
                                                        "qt.*=false\n"
                                                        "qt.qml.*=false\n"
                                                        "qt.scenegraph.*=false\n"
                                                        "qt.rhi.*=false\n"
                                                        "qt.pointer.*=false\n"
                                                        "qt.qpa.*=false"));
    }

    sentinel::core::FileLogger::instance().initialize(logDir);
    qInstallMessageHandler([](QtMsgType type, const QMessageLogContext& ctx, const QString& msg) {
        sentinel::core::FileLogger::instance().handleMessage(type, ctx, msg);

        const auto prefix = [](QtMsgType t) -> const char* {
            switch (t) {
            case QtDebugMsg:    return "[DEBUG]";
            case QtInfoMsg:     return "[INFO]";
            case QtWarningMsg:  return "[WARN]";
            case QtCriticalMsg: return "[ERROR]";
            case QtFatalMsg:    return "[FATAL]";
            }
            return "[?]";
        };
        fprintf(type == QtInfoMsg ? stdout : stderr, "%s %s\n", prefix(type),
                msg.toUtf8().constData());
        if (type == QtFatalMsg) {
            fflush(stderr);
            abort();
        }
    });
}

} // namespace sentinel::desktop
