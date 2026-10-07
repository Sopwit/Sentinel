// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QObject>
#include <QVariantMap>
namespace sentinel::desktop {
class DesktopRuntimeClient;
// Presentation over the existing client. No connection, runtime or grant ownership.
class QuickPanelController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantMap state READ state NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY changed)
public:
    explicit QuickPanelController(DesktopRuntimeClient& runtime, QObject* parent = nullptr);
    QVariantMap state() const;
    QString error() const {
        return error_;
    }
    Q_INVOKABLE bool ask(const QString& text);
    Q_INVOKABLE bool agent(const QString& text);
    Q_INVOKABLE bool cancel();
    Q_INVOKABLE bool approve(bool allow);
    Q_INVOKABLE void continueConversation();
    Q_INVOKABLE void openApproval();
    Q_INVOKABLE bool openLink(const QString& url);
    static QVariantMap parseLink(const QString& url);
signals:
    void changed();
    void openRequested(const QString& page, const QString& sessionId);
    void notificationRequested(const QString& title, const QString& body, const QString& sessionId);

private:
    bool submit(const QString& text, bool agent);
    void project();
    DesktopRuntimeClient& runtime_;
    QString error_, lastState_, lastRun_, lastApproval_;
};
} // namespace sentinel::desktop
