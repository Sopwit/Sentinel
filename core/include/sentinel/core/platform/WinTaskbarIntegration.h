// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QString>

#include <memory>

namespace sentinel::core {

struct WinTaskbarJumpListItem {
    QString title;
    QString filePath;
    QString iconPath;
    int iconIndex = 0;
};

struct WinTaskbarTask {
    QString title;
    QString appPath;
    QString arguments;
    QString iconPath;
    int iconIndex = 0;
};

class WinTaskbarIntegration : public QObject {
    Q_OBJECT
public:
    explicit WinTaskbarIntegration(QObject* parent = nullptr);
    ~WinTaskbarIntegration() override;

    void setWindowHandle(quintptr hwnd);

    void setProgressValue(quint64 completed, quint64 total);
    void setProgressIndeterminate(bool indeterminate = false);
    void setProgressPaused(bool paused = false);
    void setProgressError(bool error = false);
    void clearProgress();

    void setUserTasks(const QList<WinTaskbarTask>& tasks);
    void addRecentItem(const QString& filePath, const QString& title);
    void setRecentItems(const QList<WinTaskbarJumpListItem>& items);
    void clearJumpList();

private:
    struct Private;
    std::unique_ptr<Private> d;
};

} // namespace sentinel::core
