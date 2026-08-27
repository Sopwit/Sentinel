// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SENTINEL_DESKTOP_VIEWMODELS_CHATVIEWMODEL_H
#define SENTINEL_DESKTOP_VIEWMODELS_CHATVIEWMODEL_H

#include <QObject>
#include <QString>
#include <sentinel/core/chat/ChatMessage.h>
#include <sentinel/core/chat/IChatHistoryStore.h>
#include <sentinel/core/interfaces/IChatProvider.h>

namespace sentinel::desktop::viewmodels {

class ChatViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString prompt READ prompt WRITE setPrompt NOTIFY promptChanged)
    Q_PROPERTY(bool isStreaming READ isStreaming NOTIFY isStreamingChanged)

public:
    explicit ChatViewModel(sentinel::core::IChatProvider* provider,
                           sentinel::core::IChatHistoryStore* historyStore,
                           QObject* parent = nullptr);
    ~ChatViewModel() override = default;

    [[nodiscard]] QString prompt() const {
        return m_prompt;
    }
    void setPrompt(const QString& prompt);

    [[nodiscard]] bool isStreaming() const {
        return m_isStreaming;
    }
    void setIsStreaming(bool streaming);

public Q_SLOTS:
    void clearPrompt();
    void sendPrompt();

Q_SIGNALS:
    void promptChanged();
    void isStreamingChanged();
    void messageReceived(const QString& sender, const QString& content);

private:
    QString m_prompt;
    bool m_isStreaming{false};
    sentinel::core::IChatProvider* m_provider{nullptr};
    sentinel::core::IChatHistoryStore* m_historyStore{nullptr};
};

} // namespace sentinel::desktop::viewmodels

#endif // SENTINEL_DESKTOP_VIEWMODELS_CHATVIEWMODEL_H
