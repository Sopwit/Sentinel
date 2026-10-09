// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QObject>
#include <memory>
namespace sentinel::desktop {
// Native microphone / speech permission and capture belong to the GUI process.
class SystemDictationService final : public QObject {
    Q_OBJECT
public:
    explicit SystemDictationService(QObject* parent = nullptr);
    ~SystemDictationService() override;
    bool active() const;
    bool busy() const;
    void start(const QString& language);
    void stop();
    void cancel();
signals:
    void activeChanged();
    void completed(const QString& text);
    void failed(const QString& message);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
