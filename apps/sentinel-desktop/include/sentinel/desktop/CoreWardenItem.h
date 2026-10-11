// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QFutureWatcher>
#include <QImage>
#include <QQuickPaintedItem>
#include <QTimer>
#include <QtQml/qqmlregistration.h>
#include <sentinel/core/interfaces/ISettingsStore.h>

namespace sentinel::desktop {
class CoreWardenPreferences final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString mode READ mode WRITE setMode NOTIFY modeChanged)
public:
    explicit CoreWardenPreferences(core::ISettingsStore& store, QObject* parent = nullptr);
    QString mode() const {
        return m_mode;
    }
    void setMode(const QString& mode);
Q_SIGNALS:
    void modeChanged();

private:
    core::ISettingsStore& m_store;
    QString m_mode;
};

struct WardenFrames {
    QVector<QImage> images;
    QString label;
    QString error;
    bool loop = false;
    int fps = 8;
};

// One outstanding worker per item; stale completions are discarded before painting.
class CoreWardenItem : public QQuickPaintedItem {
    Q_OBJECT
    QML_NAMED_ELEMENT(CoreWardenPlayer)
    Q_PROPERTY(QString animationState READ animationState WRITE setAnimationState NOTIFY
                   configurationChanged)
    Q_PROPERTY(QString mode READ mode WRITE setMode NOTIFY configurationChanged)
    Q_PROPERTY(QString assetRoot READ assetRoot WRITE setAssetRoot NOTIFY configurationChanged)
    Q_PROPERTY(
        bool reducedMotion READ reducedMotion WRITE setReducedMotion NOTIFY configurationChanged)
    Q_PROPERTY(bool animate READ animate WRITE setAnimate NOTIFY configurationChanged)
    Q_PROPERTY(QString statusLabel READ statusLabel NOTIFY frameChanged)
    Q_PROPERTY(QString assetError READ assetError NOTIFY frameChanged)
    Q_PROPERTY(int frameIndex READ frameIndex NOTIFY frameChanged)
    Q_PROPERTY(int cachedFrames READ cachedFrames NOTIFY frameChanged)
    Q_PROPERTY(bool playing READ playing NOTIFY frameChanged)
public:
    explicit CoreWardenItem(QQuickItem* parent = nullptr);
    void paint(QPainter* painter) override;
    QString animationState() const {
        return m_state;
    }
    QString mode() const {
        return m_mode;
    }
    QString assetRoot() const {
        return m_root;
    }
    bool reducedMotion() const {
        return m_reduced;
    }
    bool animate() const {
        return m_animate;
    }
    QString statusLabel() const {
        return m_frames.label;
    }
    QString assetError() const {
        return m_frames.error;
    }
    int frameIndex() const {
        return m_index;
    }
    int cachedFrames() const {
        return m_frames.images.size();
    }
    bool playing() const {
        return m_timer.isActive();
    }
    void setAnimationState(const QString& value);
    void setMode(const QString& value);
    void setAssetRoot(const QString& value);
    void setReducedMotion(bool value);
    void setAnimate(bool value);
    Q_INVOKABLE static QString readinessSummary(bool connected, const QString& readiness);
    static QString discoverRoot();
    static WardenFrames load(const QString& root, const QString& state, bool animated);
    Q_INVOKABLE static QString productionState(bool connected, bool approval,
                                               const QString& sendState);
Q_SIGNALS:
    void configurationChanged();
    void frameChanged();

private:
    void reload();
    void startLoad();
    QString m_state = QStringLiteral("idle");
    QString m_mode = QStringLiteral("auto");
    QString m_root;
    bool m_reduced = false;
    bool m_animate = false;
    quint64 m_generation = 0;
    quint64 m_loadingGeneration = 0;
    int m_index = 0;
    bool m_loading = false;
    WardenFrames m_frames;
    QTimer m_timer;
    QFutureWatcher<WardenFrames> m_loader;
};
} // namespace sentinel::desktop
