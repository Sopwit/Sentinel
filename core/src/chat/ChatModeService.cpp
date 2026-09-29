// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/chat/ChatModeService.h"

#include <QDateTime>
#include <QMetaObject>
#include <QSet>

namespace sentinel::core {

ChatModeService::ChatModeService(ModelService& models, ChatSession& session,
                                 IConversationStore& store, QObject* parent)
    : QObject(parent), models_(models), session_(session), store_(store) {}

ChatModeService::~ChatModeService() {
    if (cancellation_) cancellation_->store(true);
    if (worker_) worker_->wait();
}

bool ChatModeService::busy() const { return activeMessageId_ != 0; }
ChatProviderErrorCategory ChatModeService::lastError() const { return lastError_; }

ChatMessage ChatModeService::activeMessage() const {
    for (const auto& message : session_.messages())
        if (message.id == activeMessageId_) return message;
    return {};
}

bool ChatModeService::persist(const QString& conversationId, const ChatMessage& message) {
    return store_.appendMessage({conversationId, message.id, message.role, message.content,
        message.timestamp, message.status, message.providerUsed, message.modelUsed,
        message.replyToMessageId, message.replacesMessageId, message.partial,
        message.errorCategory});
}

ChatProviderErrorCategory ChatModeService::bindingError(ModelBindingError error) {
    switch (error) {
    case ModelBindingError::AuthenticationRequired:
        return ChatProviderErrorCategory::AuthenticationRequired;
    case ModelBindingError::ModelNotFound:
        return ChatProviderErrorCategory::ModelNotFound;
    case ModelBindingError::ProviderUnavailable:
        return ChatProviderErrorCategory::ProviderUnavailable;
    case ModelBindingError::ConfigurationInvalid:
        return ChatProviderErrorCategory::RequestRejected;
    default: return ChatProviderErrorCategory::ProviderUnavailable;
    }
}

bool ChatModeService::send(const QString& conversationId, const QString& text,
                           const QList<ChatAttachment>& attachments,
                           const ModelSelection& selection, bool requireLocal) {
    if (busy() || conversationId.isEmpty() || text.trimmed().isEmpty()) return false;
    if (text.size() > 12000) {
        lastError_ = ChatProviderErrorCategory::RequestRejected;
        emit requestStateChanged();
        return false;
    }
    if (!attachments.isEmpty()) {
        lastError_ = ChatProviderErrorCategory::CapabilityUnsupported;
        emit requestStateChanged();
        return false;
    }
    const auto previous = session_.messages();
    auto user = session_.appendUserMessage(text.trimmed());
    user.status = ChatMessageStatus::Completed;
    session_.updateMessage(user);
    if (!persist(conversationId, user)) {
        session_.loadMessages(previous);
        return false;
    }
    store_.autoTitleConversation(conversationId, text.simplified().left(64));
    emit messagesChanged();
    return runTurn(conversationId, user.id, 0, selection, requireLocal);
}

bool ChatModeService::sendExisting(const QString& conversationId, int userMessageId,
                                   int replacesMessageId, const ModelSelection& selection,
                                   bool requireLocal) {
    if (busy() || conversationId.isEmpty()) return false;
    return runTurn(conversationId, userMessageId, replacesMessageId, selection, requireLocal);
}

bool ChatModeService::regenerate(const QString& conversationId, int userMessageId,
                                 const ModelSelection& selection, bool requireLocal) {
    int previous = 0;
    for (const auto& message : session_.messages())
        if (message.role == ChatRole::Assistant && message.replyToMessageId == userMessageId)
            previous = message.id;
    return sendExisting(conversationId, userMessageId, previous, selection, requireLocal);
}

bool ChatModeService::retry(const QString& conversationId, int assistantMessageId,
                            const ModelSelection& selection, bool requireLocal) {
    for (const auto& message : session_.messages()) {
        if (message.id != assistantMessageId || message.role != ChatRole::Assistant ||
            (message.status != ChatMessageStatus::Failed &&
             message.status != ChatMessageStatus::Interrupted &&
             message.status != ChatMessageStatus::Error)) continue;
        return sendExisting(conversationId, message.replyToMessageId, message.id,
                            selection, requireLocal);
    }
    return false;
}

QString ChatModeService::contextFor(const QString& conversationId, int userMessageId) const {
    QList<ChatMessage> accepted;
    const auto summary = store_.loadSummaryMetadata(conversationId);
    const bool useSummary = !summary.summaryText.isEmpty() &&
        (summary.readinessState == QLatin1String("Ready") ||
         summary.readinessState == QLatin1String("Truncated")) &&
        summary.coveredFirstMessageId > 0 && summary.coveredLastMessageId < userMessageId;
    QSet<int> replaced;
    for (const auto& message : session_.messages())
        if (message.replacesMessageId > 0 &&
            (message.status == ChatMessageStatus::Completed ||
             message.status == ChatMessageStatus::Received))
            replaced.insert(message.replacesMessageId);
    for (const auto& message : session_.messages()) {
        if (message.id > userMessageId) break;
        if (useSummary && message.id <= summary.coveredLastMessageId) continue;
        if (message.role == ChatRole::Assistant &&
            (replaced.contains(message.id) ||
             (message.status != ChatMessageStatus::Completed &&
              message.status != ChatMessageStatus::Received))) continue;
        if (message.role == ChatRole::System) continue;
        accepted.append(message);
    }
    QStringList lines;
    int budget = 10000;
    for (int i = accepted.size() - 1; i >= 0 && lines.size() < 24 && budget > 0; --i) {
        const auto& message = accepted.at(i);
        const auto content = message.id == userMessageId ? message.content
            : message.content.right(qMin(budget, 3000));
        budget -= content.size();
        lines.prepend(QStringLiteral("%1: %2")
            .arg(message.role == ChatRole::User ? QStringLiteral("User")
               : message.role == ChatRole::Assistant ? QStringLiteral("Assistant")
                                                      : QStringLiteral("System"), content));
    }
    if (useSummary)
        lines.prepend(QStringLiteral("Earlier conversation summary: %1")
                          .arg(summary.summaryText.left(2000)));
    return lines.join(QStringLiteral("\n\n"));
}

bool ChatModeService::runTurn(const QString& conversationId, int userMessageId,
                              int replacesMessageId, const ModelSelection& preferredSelection,
                              bool requireLocal) {
    ChatMessage user;
    for (const auto& message : session_.messages())
        if (message.id == userMessageId && message.role == ChatRole::User) user = message;
    if (user.id == 0) return false;
    const auto selection = preferredSelection.isValid() ? preferredSelection : models_.selectedModel();
    const bool blockedByLocalPolicy = requireLocal &&
        models_.currentModelMetadata(selection.providerId, selection.modelId).providerKind ==
            ProviderKind::Cloud;
    auto resolved = blockedByLocalPolicy ? ModelBindingResolution{} : models_.resolve(selection);
    auto assistant = session_.appendAssistantMessage({}, ChatMessageStatus::Queued);
    assistant.replyToMessageId = user.id;
    assistant.replacesMessageId = replacesMessageId;
    assistant.providerUsed = selection.providerId;
    assistant.modelUsed = selection.modelId;
    session_.updateMessage(assistant);
    if (!persist(conversationId, assistant)) {
        assistant.status = ChatMessageStatus::Failed;
        assistant.errorCategory = ChatProviderErrorCategory::ProviderUnavailable;
        session_.updateMessage(assistant);
        emit messagesChanged();
        return false;
    }
    activeConversationId_ = conversationId;
    activeMessageId_ = assistant.id;
    lastError_ = ChatProviderErrorCategory::None;
    emit messagesChanged();
    emit requestStateChanged();
    if (blockedByLocalPolicy || !resolved.ok() ||
        resolved.provider->status() != ChatProviderStatus::Ready) {
        assistant.status = ChatMessageStatus::Failed;
        if (blockedByLocalPolicy)
            assistant.content = QStringLiteral("Local Only requires a local provider and model. The configured selection is cloud-based.");
        assistant.errorCategory = blockedByLocalPolicy
            ? ChatProviderErrorCategory::RequestRejected : resolved.ok()
            ? ChatProviderErrorCategory::ProviderUnavailable : bindingError(resolved.error);
        lastError_ = assistant.errorCategory;
        session_.updateMessage(assistant);
        persist(conversationId, assistant);
        activeMessageId_ = 0;
        emit messagesChanged();
        emit requestStateChanged();
        return true;
    }
    assistant.status = ChatMessageStatus::Sending;
    assistant.providerUsed = resolved.binding.providerId;
    assistant.modelUsed = resolved.binding.modelId;
    session_.updateMessage(assistant);
    if (!persist(conversationId, assistant)) {
        assistant.status = ChatMessageStatus::Failed;
        assistant.errorCategory = ChatProviderErrorCategory::ProviderUnavailable;
        session_.updateMessage(assistant);
        activeMessageId_ = 0;
        emit messagesChanged();
        emit requestStateChanged();
        return false;
    }
    const auto prompt = contextFor(conversationId, user.id);
    const auto provider = resolved.provider;
    cancellation_ = std::make_shared<std::atomic_bool>(false);
    const auto cancellation = cancellation_;
    const auto messageId = assistant.id;
    worker_ = QThread::create([this, provider, prompt, cancellation, conversationId, messageId]() {
        auto delta = [this, conversationId, messageId](const QString& text) {
            QMetaObject::invokeMethod(this, [this, conversationId, messageId, text]() {
                acceptDelta(conversationId, messageId, text);
            }, Qt::QueuedConnection);
        };
        ChatProviderReply reply;
        if (provider->supportsStreaming())
            reply = provider->sendMessageStreaming(prompt, delta, cancellation);
        else {
            ChatRequestOptions options;
            options.cancellationToken = cancellation;
            reply = provider->sendRequest(prompt, options);
        }
        QMetaObject::invokeMethod(this, [this, conversationId, messageId, reply]() {
            acceptResult(conversationId, messageId, reply);
        }, Qt::QueuedConnection);
    });
    auto* worker = worker_;
    connect(worker, &QThread::finished, this, [this, worker]() {
        if (worker_ == worker) worker_ = nullptr;
    });
    connect(worker_, &QThread::finished, worker_, &QObject::deleteLater);
    worker_->start();
    emit requestStateChanged();
    return true;
}

void ChatModeService::acceptDelta(const QString& conversationId, int messageId,
                                  const QString& delta) {
    if (activeConversationId_ != conversationId || activeMessageId_ != messageId ||
        !cancellation_ || cancellation_->load() || delta.isEmpty()) return;
    auto message = activeMessage();
    message.content += delta;
    message.status = ChatMessageStatus::Streaming;
    message.partial = true;
    session_.updateMessage(message);
    const auto now = QDateTime::currentMSecsSinceEpoch();
    if (now - lastPersistMs_ >= 250) {
        persist(conversationId, message);
        lastPersistMs_ = now;
    }
    emit messagesChanged();
    emit requestStateChanged();
}

void ChatModeService::acceptResult(const QString& conversationId, int messageId,
                                   const ChatProviderReply& reply) {
    if (activeConversationId_ != conversationId || activeMessageId_ != messageId) return;
    auto message = activeMessage();
    if (reply.success &&
        (reply.lifecycle == ChatRequestLifecycle::Completed ||
         reply.lifecycle == ChatRequestLifecycle::Pending) &&
        (!reply.message.isEmpty() || !message.content.isEmpty())) {
        if (!reply.message.isEmpty()) message.content = reply.message;
        message.status = ChatMessageStatus::Completed;
        message.partial = false;
        message.errorCategory = ChatProviderErrorCategory::None;
    } else {
        const bool cancelled = reply.lifecycle == ChatRequestLifecycle::Cancelled ||
            reply.category == ChatProviderErrorCategory::Cancelled ||
            (cancellation_ && cancellation_->load() && !reply.success);
        message.status = cancelled ? ChatMessageStatus::Cancelled : ChatMessageStatus::Failed;
        message.partial = !message.content.isEmpty();
        message.errorCategory = cancelled ? ChatProviderErrorCategory::Cancelled
            : reply.success ? ChatProviderErrorCategory::MalformedResponse
            : reply.category != ChatProviderErrorCategory::None ? reply.category
            : reply.lifecycle == ChatRequestLifecycle::TimedOut
                ? ChatProviderErrorCategory::Timeout
            : reply.lifecycle == ChatRequestLifecycle::RateLimited
                ? ChatProviderErrorCategory::RateLimited
                : ChatProviderErrorCategory::ProviderFailure;
        lastError_ = message.errorCategory;
    }
    session_.updateMessage(message);
    if (!persist(conversationId, message)) {
        message.status = ChatMessageStatus::Failed;
        message.partial = !message.content.isEmpty();
        message.errorCategory = ChatProviderErrorCategory::ProviderFailure;
        lastError_ = message.errorCategory;
        session_.updateMessage(message);
    }
    activeMessageId_ = 0;
    cancellation_.reset();
    emit messagesChanged();
    emit requestStateChanged();
}

bool ChatModeService::stop() {
    if (!busy() || !cancellation_) return false;
    cancellation_->store(true);
    return true;
}

} // namespace sentinel::core
