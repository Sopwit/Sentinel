#pragma once

#include <QObject>
#include <QStringList>

namespace sentinel::core {

class ModeManager final : public QObject {
    Q_OBJECT
    Q_PROPERTY(Mode currentMode READ currentMode WRITE setCurrentMode NOTIFY currentModeChanged)
    Q_PROPERTY(QString currentModeName READ currentModeName NOTIFY currentModeChanged)
    Q_PROPERTY(QStringList availableModes READ availableModes CONSTANT)

public:
    enum class Mode { Chat, Agent };
    Q_ENUM(Mode)

    explicit ModeManager(QObject* parent = nullptr);

    Mode currentMode() const;
    void setCurrentMode(Mode mode);
    QString currentModeName() const;
    QStringList availableModes() const;

    Q_INVOKABLE void setModeByName(const QString& modeName);

signals:
    void currentModeChanged();

private:
    static QString modeToName(Mode mode);

    Mode currentMode_ = Mode::Chat;
};

} // namespace sentinel::core
