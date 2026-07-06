#include "sentinel/core/ModeManager.h"

namespace sentinel::core {

ModeManager::ModeManager(QObject* parent) : QObject(parent) {}

ModeManager::Mode ModeManager::currentMode() const {
    return currentMode_;
}

void ModeManager::setCurrentMode(Mode mode) {
    if (currentMode_ == mode) {
        return;
    }

    currentMode_ = mode;
    emit currentModeChanged();
}

QString ModeManager::currentModeName() const {
    return modeToName(currentMode_);
}

QStringList ModeManager::availableModes() const {
    return {};
}

void ModeManager::setModeByName(const QString& modeName) {
    Q_UNUSED(modeName)
}

QString ModeManager::modeToName(Mode mode) {
    switch (mode) {
    case Mode::Default:
        return QStringLiteral("Sentinel");
    }

    return QStringLiteral("Sentinel");
}

} // namespace sentinel::core
