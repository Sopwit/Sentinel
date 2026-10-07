// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QObject>
#include <QStringList>
namespace sentinel::desktop {
// macOS-only desktop boundary. No runtime or permission grants are held here.
class MacNativeNotifications final : public QObject {
    Q_OBJECT
public:
    explicit MacNativeNotifications(QObject* parent = nullptr);
    ~MacNativeNotifications() override;
    void refreshAuthorization();
    void deliver(const QString& title, const QString& body, const QString& session,
                 const QString& page);
signals:
    void statusChanged(const QString& status);
    void clicked(const QString& session, const QString& page);

private:
    void* delegate_ = nullptr;
    QStringList identifiers_;
    bool authorizationRequested_ = false;
};
} // namespace sentinel::desktop
