// SPDX-License-Identifier: GPL-3.0-or-later
#include "sentinel/desktop/CoreWardenItem.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QtConcurrent/QtConcurrentRun>

namespace sentinel::desktop {
namespace {
bool validMode(const QString& mode) {
    return QStringList{"off", "static", "animated", "auto"}.contains(mode);
}
QString safePath(const QString& root, const QString& relative) {
    if (relative.isEmpty() || QDir::isAbsolutePath(relative) || relative.contains('\\') ||
        relative.split('/').contains(".."))
        return {};
    const auto base = QFileInfo(root).canonicalFilePath();
    const auto path = QFileInfo(QDir(root).filePath(relative)).canonicalFilePath();
    return !base.isEmpty() && path.startsWith(base + '/') ? path : QString{};
}
QJsonObject json(const QString& root, const QString& relative) {
    const auto path = safePath(root, relative);
    if (path.isEmpty())
        return {};
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly) || file.size() > 131072)
        return {};
    return QJsonDocument::fromJson(file.readAll()).object();
}
} // namespace
CoreWardenPreferences::CoreWardenPreferences(core::ISettingsStore& store, QObject* parent)
    : QObject(parent), m_store(store), m_mode(store.value("coreWardenMode", "auto")) {
    if (!validMode(m_mode))
        m_mode = "auto";
}
void CoreWardenPreferences::setMode(const QString& mode) {
    if (!validMode(mode) || mode == m_mode)
        return;
    m_mode = mode;
    m_store.setValue("coreWardenMode", mode);
    Q_EMIT modeChanged();
}
QString CoreWardenItem::discoverRoot() {
    const auto app = QCoreApplication::applicationDirPath();
    const QString overrideRoot = qEnvironmentVariable("SENTINEL_CORE_WARDEN_ASSET_ROOT");
    if (!overrideRoot.isEmpty())
        return overrideRoot;
    QStringList candidates{app + "/../Resources/core-warden", app + "/core-warden",
                           app + "/../share/sentinel/core-warden"};
    // Relocatable development trees: walk executable ancestors, never embed a home path.
    QDir dir(app);
    for (int i = 0; i < 8; ++i) {
        candidates << dir.filePath("assets/core-warden");
        if (!dir.cdUp())
            break;
    }
    for (const auto& candidate : candidates)
        if (QFileInfo::exists(candidate + "/state-contract.json"))
            return candidate;
    return {};
}
WardenFrames CoreWardenItem::load(const QString& root, const QString& state, bool animated) {
    WardenFrames result;
    result.label = state == "idle" ? QStringLiteral("Inactive") : state;
    auto fail = [&result, &root]() {
        result.images.clear();
        result.error = "Core Warden assets unavailable";
        const auto fallbackPath = safePath(root, "animations/idle/192/frame_000.png");
        if (fallbackPath.isEmpty())
            return result;
        QImageReader fallback(fallbackPath, "png");
        if (fallback.size() == QSize(192, 192)) {
            auto image = fallback.read().convertToFormat(QImage::Format_ARGB32_Premultiplied);
            if (!image.isNull())
                result.images.push_back(std::move(image));
        }
        return result;
    };
    const auto contract = json(root, "state-contract.json");
    const auto manifestPath = contract.value("animation_manifest").toString();
    const auto manifest = json(root, manifestPath);
    if (contract.value("schema_version").toInt() != 1 ||
        contract.value("asset_id").toString() != "sentinel_core_warden" ||
        manifest.value("asset_id").toString() != "sentinel_core_warden")
        return fail();
    const auto entry = contract.value("states").toObject().value(state).toObject();
    if (entry.isEmpty())
        return fail();
    if (state != "idle")
        result.label = entry.value("label").toString(state);
    // CW-03B artwork is accepted for Desktop; TUI mappings remain unchanged.
    auto animation = entry.value("animation").toString();
    if (animation.isEmpty())
        animation = contract.value("review_only_animations")
                        .toObject()
                        .value(state)
                        .toObject()
                        .value("animation")
                        .toString();
    const auto descriptor = manifest.value("states").toObject().value(animation).toObject();
    const int count = descriptor.value("frame_count").toInt();
    const auto paths = descriptor.value("frames").toObject().value("192").toArray();
    if (count < 1 || count > 32 || paths.size() != count || descriptor.value("fps").toInt() != 8 ||
        !descriptor.value("loop").isBool())
        return fail();
    result.loop = descriptor.value("loop").toBool();
    if (descriptor.value("playback_mode").toString() != (result.loop ? "loop" : "once_then_hold"))
        return fail();
    const auto candidate =
        contract.value("review_only_animations").toObject().value(state).toObject();
    if (!candidate.isEmpty() &&
        (candidate.value("frame_count").toInt() != count ||
         candidate.value("fps").toInt() != result.fps ||
         candidate.value("playback_mode") != descriptor.value("playback_mode") ||
         (!result.loop && candidate.value("hold_frame_index").toInt(-1) != count - 1)))
        return fail();
    for (int i = 0; i < (animated ? count : 1); ++i) {
        const auto path = safePath(root, paths[i].toString());
        if (path.isEmpty() || QFileInfo(path).size() > 1048576)
            return fail();
        QImageReader reader(path, "png");
        if (reader.size() != QSize(192, 192))
            return fail();
        auto image = reader.read().convertToFormat(QImage::Format_ARGB32_Premultiplied);
        if (image.isNull())
            return fail();
        result.images.push_back(std::move(image));
    }
    return result;
}
QString CoreWardenItem::productionState(bool connected, bool approval, const QString& sendState) {
    if (!connected)
        return "idle";
    if (approval)
        return "approval";
    if (sendState == "sending" || sendState == "streaming")
        return "thinking";
    if (sendState == "failed")
        return "error";
    return "idle";
}
QString CoreWardenItem::readinessSummary(bool connected, const QString& readiness) {
    if (!connected)
        return tr("Sentinel is open. Connect the daemon to use inference.");
    if (readiness.compare("busy", Qt::CaseInsensitive) == 0)
        return tr("A request is running. Review its status below.");
    if (readiness.compare("ready", Qt::CaseInsensitive) == 0)
        return tr("The daemon reports the provider and model available. Send a message to verify "
                  "inference.");
    return tr("Daemon connected. Provider and model availability is not confirmed; review Models & "
              "Providers.");
}
CoreWardenItem::CoreWardenItem(QQuickItem* parent)
    : QQuickPaintedItem(parent), m_root(discoverRoot()) {
    setAntialiasing(true);
    m_timer.setTimerType(Qt::PreciseTimer);
    connect(&m_timer, &QTimer::timeout, this, [this] {
        if (m_index + 1 < m_frames.images.size())
            ++m_index;
        else if (m_frames.loop)
            m_index = 0;
        else
            m_timer.stop();
        update();
        Q_EMIT frameChanged();
    });
    connect(this, &QQuickItem::visibleChanged, this, &CoreWardenItem::reload);
    connect(&m_loader, &QFutureWatcher<WardenFrames>::finished, this, [this] {
        auto loaded = m_loader.future().takeResult();
        m_loading = false;
        if (m_loadingGeneration != m_generation) {
            startLoad();
            return;
        }
        m_frames = std::move(loaded);
        m_index = 0;
        if (m_frames.images.size() > 1)
            m_timer.start(1000 / m_frames.fps);
        update();
        Q_EMIT frameChanged();
    });
    QTimer::singleShot(0, this, &CoreWardenItem::reload);
}
void CoreWardenItem::reload() {
    ++m_generation;
    m_timer.stop();
    m_frames = {};
    m_index = 0;
    update();
    Q_EMIT configurationChanged();
    Q_EMIT frameChanged();
    if (!m_loading)
        startLoad();
}
void CoreWardenItem::startLoad() {
    if (!isVisible() || m_mode == "off")
        return;
    m_loading = true;
    m_loadingGeneration = m_generation;
    const auto root = m_root;
    const auto state = m_state;
    const bool animated = m_animate && !m_reduced && (m_mode == "auto" || m_mode == "animated");
    m_loader.setFuture(
        QtConcurrent::run([root, state, animated] { return load(root, state, animated); }));
}
void CoreWardenItem::setAnimationState(const QString& value) {
    if (m_state != value) {
        m_state = value;
        reload();
    }
}
void CoreWardenItem::setMode(const QString& value) {
    if (validMode(value) && m_mode != value) {
        m_mode = value;
        reload();
    }
}
void CoreWardenItem::setAssetRoot(const QString& value) {
    if (m_root != value) {
        m_root = value;
        reload();
    }
}
void CoreWardenItem::setReducedMotion(bool value) {
    if (m_reduced != value) {
        m_reduced = value;
        reload();
    }
}
void CoreWardenItem::setAnimate(bool value) {
    if (m_animate != value) {
        m_animate = value;
        reload();
    }
}
void CoreWardenItem::paint(QPainter* painter) {
    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
    if (m_mode == "off")
        return;
    if (!m_frames.images.isEmpty()) {
        const qreal side = qMin(width(), height());
        painter->drawImage(QRectF((width() - side) / 2, (height() - side) / 2, side, side),
                           m_frames.images[m_index]);
    } else {
        painter->setPen(QColor("#A9CAD3"));
        painter->drawText(boundingRect(), Qt::AlignCenter, QStringLiteral("[ S ]"));
    }
}
} // namespace sentinel::desktop
