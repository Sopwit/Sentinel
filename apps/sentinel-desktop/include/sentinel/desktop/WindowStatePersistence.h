// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QPointer>
#include <QRect>
#include <QString>

#include <QList>

class QSettings;
class QTimer;
class QWindow;

namespace sentinel::desktop {

// Persisted main-window geometry. Only numeric geometry and a maximized flag are stored:
// no titles, paths, session identifiers, or user content. Width/height/position describe the
// *normal* (non-maximized) frame so un-maximizing restores a sane size.
struct WindowGeometry {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    bool hasPosition = false;
    bool maximized = false;

    bool hasSize() const {
        return width > 0 && height > 0;
    }

    bool operator==(const WindowGeometry& other) const {
        return x == other.x && y == other.y && width == other.width && height == other.height &&
               hasPosition == other.hasPosition && maximized == other.maximized;
    }
};

// Deterministic, platform-independent geometry rules. No display state is cached here, so the
// same saved geometry yields the same result for a given screen topology.
class WindowStateRules {
public:
    static constexpr int kDefaultWidth = 1320;
    static constexpr int kDefaultHeight = 860;
    static constexpr int kMinimumWidth = 780;
    static constexpr int kMinimumHeight = 640;

    // Returns a geometry that is always usable: the minimum size is enforced, the size is
    // clamped to the current screen topology, and an off-screen position is dropped so the
    // window manager can place it. Falls back to default-sized geometry when nothing survives.
    static WindowGeometry resolve(const WindowGeometry& saved, const QList<QRect>& availableScreens,
                                  const QRect& primaryAvailable);

    // True when a frame at least partially overlaps a currently attached screen, i.e. the user
    // can still reach it. Used to detect windows stranded by a display disconnect.
    static bool isReachable(int x, int y, int width, int height,
                            const QList<QRect>& availableScreens);
};

// Persists and restores a single top-level window through Qt's settings backend. Disk writes
// are debounced; position is applied only where the platform honours absolute placement
// (Wayland compositors do not).
class WindowStateController final : public QObject {
    Q_OBJECT
public:
    explicit WindowStateController(QSettings& settings, QObject* parent = nullptr);
    ~WindowStateController() override;

    // Restores any saved state onto the window, then persists later changes.
    void attach(QWindow* window);
    void saveNow();

    WindowGeometry lastRestored() const {
        return restored_;
    }
    bool restoredFromDisk() const {
        return restoredFromDisk_;
    }
    bool positionApplied() const {
        return positionApplied_;
    }

signals:
    void restored(const WindowGeometry& geometry);

private:
    void scheduleSave();
    void handleScreensChanged();
    WindowGeometry captureNormalGeometry() const;
    WindowGeometry capture() const;
    WindowGeometry load() const;
    void store(const WindowGeometry& geometry);
    static bool platformHonoursPositioning();

    QSettings& settings_;
    QTimer* saveTimer_ = nullptr;
    QPointer<QWindow> window_;
    WindowGeometry normalGeometry_;
    WindowGeometry lastStored_;
    WindowGeometry restored_;
    bool restoredFromDisk_ = false;
    bool positionApplied_ = false;
    bool hasStored_ = false;
};

} // namespace sentinel::desktop
