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
    return {
        modeToName(Mode::Companion), modeToName(Mode::Focus),   modeToName(Mode::Mission),
        modeToName(Mode::System),    modeToName(Mode::Minimal), modeToName(Mode::Tactical),
    };
}

void ModeManager::setModeByName(const QString& modeName) {
    const auto modes = availableModes();
    const auto index = modes.indexOf(modeName);

    if (index < 0) {
        return;
    }

    setCurrentMode(static_cast<Mode>(index));
}

QString ModeManager::modeToName(Mode mode) {
    switch (mode) {
    case Mode::Companion:
        return QStringLiteral("Companion Mode");
    case Mode::Focus:
        return QStringLiteral("Focus Mode");
    case Mode::Mission:
        return QStringLiteral("Mission Mode");
    case Mode::System:
        return QStringLiteral("System Mode");
    case Mode::Minimal:
        return QStringLiteral("Minimal Mode");
    case Mode::Tactical:
        return QStringLiteral("Tactical Mode");
    }

    return QStringLiteral("Companion Mode");
}

} // namespace sentinel::core
