#include <sentinel/desktop/viewmodels/ChatViewModel.h>

namespace sentinel::desktop::viewmodels {

ChatViewModel::ChatViewModel(QObject* parent)
    : QObject(parent) {
}

void ChatViewModel::setPrompt(const QString& prompt) {
    if (m_prompt != prompt) {
        m_prompt = prompt;
        Q_EMIT promptChanged();
    }
}

void ChatViewModel::setIsStreaming(bool streaming) {
    if (m_isStreaming != streaming) {
        m_isStreaming = streaming;
        Q_EMIT isStreamingChanged();
    }
}

void ChatViewModel::clearPrompt() {
    setPrompt(QString());
}

void ChatViewModel::sendPrompt() {
    if (m_prompt.trimmed().isEmpty()) {
        return;
    }
    const QString text = m_prompt;
    clearPrompt();
    Q_EMIT messageReceived(QStringLiteral("user"), text);
}

} // namespace sentinel::desktop::viewmodels
