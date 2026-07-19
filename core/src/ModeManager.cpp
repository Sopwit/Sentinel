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
    return {QStringLiteral("Chat"), QStringLiteral("Agent")};
}

void ModeManager::setModeByName(const QString& modeName) {
    const auto trimmed = modeName.trimmed().toLower();
    if (trimmed == QStringLiteral("chat")) {
        setCurrentMode(Mode::Chat);
    } else if (trimmed == QStringLiteral("agent")) {
        setCurrentMode(Mode::Agent);
    }
}

QString ModeManager::modeToName(Mode mode) {
    switch (mode) {
    case Mode::Chat:
        return QStringLiteral("Chat");
    case Mode::Agent:
        return QStringLiteral("Agent");
    }

    return QStringLiteral("Chat");
}

} // namespace sentinel::core
