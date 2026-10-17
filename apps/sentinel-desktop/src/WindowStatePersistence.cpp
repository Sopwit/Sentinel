// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/desktop/WindowStatePersistence.h"

#include <QGuiApplication>
#include <QScreen>
#include <QSettings>
#include <QTimer>
#include <QWindow>

namespace sentinel::desktop {

namespace {

constexpr auto kSettingsGroup = "mainWindow";
constexpr int kSettingsSchemaVersion = 1;
constexpr int kSaveDebounceMs = 750;

bool intersectsAnyScreen(const QRect& frame, const QList<QRect>& screens) {
    for (const auto& screen : screens)
        if (screen.intersects(frame))
            return true;
    return false;
}

// The screen the saved frame overlaps most; keeps a partially off-screen window reachable.
QRect bestScreen(const QRect& frame, const QList<QRect>& screens, const QRect& fallback) {
    QRect best;
    int bestArea = -1;
    for (const auto& screen : screens) {
        const QRect overlap = screen.intersected(frame);
        const int area = overlap.width() * overlap.height();
        if (area > bestArea) {
            bestArea = area;
            best = screen;
        }
    }
    return best.isValid() ? best : fallback;
}

QRect largestScreen(const QList<QRect>& screens) {
    QRect best;
    for (const auto& screen : screens) {
        if (!best.isValid() || screen.width() * screen.height() > best.width() * best.height()) {
            best = screen;
        }
    }
    return best;
}

QList<QRect> currentAvailableScreens() {
    QList<QRect> screens;
    for (auto* screen : QGuiApplication::screens()) {
        if (screen)
            screens.append(screen->availableGeometry());
    }
    return screens;
}

QRect currentPrimaryAvailable() {
    auto* primary = QGuiApplication::primaryScreen();
    return primary ? primary->availableGeometry() : QRect();
}

} // namespace

WindowGeometry WindowStateRules::resolve(const WindowGeometry& saved,
                                         const QList<QRect>& availableScreens,
                                         const QRect& primaryAvailable) {
    QList<QRect> screens;
    screens.reserve(availableScreens.size());
    for (const auto& screen : availableScreens) {
        if (screen.isValid())
            screens.append(screen);
    }

    const QRect primary =
        primaryAvailable.isValid()
            ? primaryAvailable
            : (screens.isEmpty() ? QRect(0, 0, kDefaultWidth, kDefaultHeight) : screens.first());

    WindowGeometry resolved;
    resolved.maximized = saved.maximized;

    int width = saved.width > 0 ? saved.width : kDefaultWidth;
    int height = saved.height > 0 ? saved.height : kDefaultHeight;
    width = qMax(width, kMinimumWidth);
    height = qMax(height, kMinimumHeight);

    // Clamp to the current topology so a reduced resolution never yields an unusable window.
    const QRect largest = largestScreen(screens);
    const QRect bound = largest.isValid() ? largest : primary;
    width = qMin(width, qMax(bound.width(), kMinimumWidth));
    height = qMin(height, qMax(bound.height(), kMinimumHeight));
    resolved.width = width;
    resolved.height = height;

    if (!saved.hasPosition || !saved.hasSize() || screens.isEmpty()) {
        resolved.hasPosition = false;
        return resolved;
    }

    const QRect frame(saved.x, saved.y, width, height);
    if (!WindowStateRules::isReachable(saved.x, saved.y, width, height, screens)) {
        // Saved display is gone (disconnected) or the position is unusable: let the WM place it.
        resolved.hasPosition = false;
        return resolved;
    }

    const QRect target = bestScreen(frame, screens, primary);
    resolved.x = qBound(target.left(), saved.x, qMax(target.left(), target.right() - width + 1));
    resolved.y = qBound(target.top(), saved.y, qMax(target.top(), target.bottom() - height + 1));
    resolved.hasPosition = true;
    return resolved;
}

bool WindowStateRules::isReachable(int x, int y, int width, int height,
                                   const QList<QRect>& availableScreens) {
    QList<QRect> screens;
    screens.reserve(availableScreens.size());
    for (const auto& screen : availableScreens) {
        if (screen.isValid())
            screens.append(screen);
    }
    return intersectsAnyScreen(QRect(x, y, width, height), screens);
}

WindowStateController::WindowStateController(QSettings& settings, QObject* parent)
    : QObject(parent), settings_(settings) {
    saveTimer_ = new QTimer(this);
    saveTimer_->setSingleShot(true);
    saveTimer_->setInterval(kSaveDebounceMs);
    connect(saveTimer_, &QTimer::timeout, this, &WindowStateController::saveNow);
}

WindowStateController::~WindowStateController() = default;

bool WindowStateController::platformHonoursPositioning() {
    const QString platform = QGuiApplication::platformName();
    // Wayland compositors own placement, so absolute x/y is deliberately not applied there.
    return !platform.startsWith(QStringLiteral("wayland"), Qt::CaseInsensitive);
}

void WindowStateController::attach(QWindow* window) {
    if (!window || window_ == window)
        return;
    window_ = window;

    const QList<QRect> screens = currentAvailableScreens();
    const QRect primary = currentPrimaryAvailable();

    const WindowGeometry saved = load();
    restoredFromDisk_ = saved.hasSize();
    const WindowGeometry resolved = WindowStateRules::resolve(saved, screens, primary);
    restored_ = resolved;
    normalGeometry_ = resolved;

    window_->resize(resolved.width, resolved.height);
    if (resolved.hasPosition && platformHonoursPositioning()) {
        window_->setPosition(resolved.x, resolved.y);
        positionApplied_ = true;
    }
    if (resolved.maximized)
        window_->showMaximized();

    connect(window_, &QWindow::widthChanged, this, [this] { scheduleSave(); });
    connect(window_, &QWindow::heightChanged, this, [this] { scheduleSave(); });
    connect(window_, &QWindow::xChanged, this, [this] { scheduleSave(); });
    connect(window_, &QWindow::yChanged, this, [this] { scheduleSave(); });
    connect(window_, &QWindow::windowStateChanged, this, [this](Qt::WindowState state) {
        if (state == Qt::WindowNoState)
            normalGeometry_ = captureNormalGeometry();
        scheduleSave();
    });
    // A display disconnect or topology change can strand the window; re-home it when the
    // platform allows absolute placement.
    if (auto* guiApp = qGuiApp) {
        connect(guiApp, &QGuiApplication::screenRemoved, this,
                &WindowStateController::handleScreensChanged);
        connect(guiApp, &QGuiApplication::screenAdded, this,
                &WindowStateController::handleScreensChanged);
        connect(guiApp, &QGuiApplication::primaryScreenChanged, this,
                &WindowStateController::handleScreensChanged);
    }
    connect(QGuiApplication::instance(), &QGuiApplication::aboutToQuit, this,
            &WindowStateController::saveNow);

    emit restored(resolved);
}

void WindowStateController::handleScreensChanged() {
    if (!window_)
        return;
    const QList<QRect> screens = currentAvailableScreens();
    if (screens.isEmpty())
        return;
    if (WindowStateRules::isReachable(window_->x(), window_->y(), window_->width(),
                                      window_->height(), screens)) {
        return;
    }
    // The screen holding the window is gone. Wayland compositors own placement, so only act
    // where the platform honours absolute positioning.
    if (!platformHonoursPositioning())
        return;
    const QRect primary = currentPrimaryAvailable();
    const QRect target = primary.isValid() ? primary : screens.first();
    const int width = qMin(window_->width(), qMax(target.width(), WindowStateRules::kMinimumWidth));
    const int height =
        qMin(window_->height(), qMax(target.height(), WindowStateRules::kMinimumHeight));
    window_->setPosition(target.left(), target.top());
    window_->resize(width, height);
    normalGeometry_ = captureNormalGeometry();
    scheduleSave();
}

void WindowStateController::scheduleSave() {
    // Restarting the timer debounces a continuous resize/move into a single write.
    saveTimer_->start();
}

WindowGeometry WindowStateController::captureNormalGeometry() const {
    WindowGeometry geometry;
    if (!window_)
        return geometry;
    geometry.x = window_->x();
    geometry.y = window_->y();
    geometry.width = window_->width();
    geometry.height = window_->height();
    geometry.hasPosition = true;
    return geometry;
}

WindowGeometry WindowStateController::capture() const {
    WindowGeometry geometry = normalGeometry_;
    if (window_) {
        const bool maximized = window_->windowStates().testFlag(Qt::WindowMaximized);
        if (!maximized && !window_->windowStates().testFlag(Qt::WindowFullScreen))
            geometry = captureNormalGeometry();
        geometry.maximized = maximized;
    }
    return geometry;
}

void WindowStateController::saveNow() {
    if (saveTimer_)
        saveTimer_->stop();
    if (!window_)
        return;
    const WindowGeometry geometry = capture();
    if (!geometry.hasSize())
        return;
    if (hasStored_ && geometry == lastStored_)
        return;
    store(geometry);
    lastStored_ = geometry;
    hasStored_ = true;
}

WindowGeometry WindowStateController::load() const {
    WindowGeometry geometry;
    settings_.beginGroup(QLatin1String(kSettingsGroup));
    const int schema = settings_.value(QStringLiteral("schema"), 0).toInt();
    if (schema == kSettingsSchemaVersion && settings_.status() == QSettings::NoError) {
        geometry.width = settings_.value(QStringLiteral("width"), 0).toInt();
        geometry.height = settings_.value(QStringLiteral("height"), 0).toInt();
        geometry.x = settings_.value(QStringLiteral("x"), 0).toInt();
        geometry.y = settings_.value(QStringLiteral("y"), 0).toInt();
        geometry.hasPosition = settings_.value(QStringLiteral("hasPosition"), false).toBool();
        geometry.maximized = settings_.value(QStringLiteral("maximized"), false).toBool();
    }
    settings_.endGroup();
    return geometry;
}

void WindowStateController::store(const WindowGeometry& geometry) {
    settings_.beginGroup(QLatin1String(kSettingsGroup));
    settings_.setValue(QStringLiteral("schema"), kSettingsSchemaVersion);
    settings_.setValue(QStringLiteral("width"), geometry.width);
    settings_.setValue(QStringLiteral("height"), geometry.height);
    settings_.setValue(QStringLiteral("x"), geometry.x);
    settings_.setValue(QStringLiteral("y"), geometry.y);
    settings_.setValue(QStringLiteral("hasPosition"), geometry.hasPosition);
    settings_.setValue(QStringLiteral("maximized"), geometry.maximized);
    settings_.endGroup();
    settings_.sync();
}

} // namespace sentinel::desktop
