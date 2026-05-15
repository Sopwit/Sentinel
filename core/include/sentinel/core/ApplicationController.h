#pragma once

#include "sentinel/core/ChatSession.h"
#include "sentinel/core/IChatProvider.h"
#include "sentinel/core/IMemoryStore.h"

#include <QObject>
#include <QStringList>
#include <memory>

namespace sentinel::core {

class ApplicationController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString providerName READ providerName CONSTANT)
    Q_PROPERTY(QString providerStatus READ providerStatus CONSTANT)
    Q_PROPERTY(QStringList chatMessages READ chatMessages NOTIFY chatMessagesChanged)
    Q_PROPERTY(QStringList memoryEntries READ memoryEntries NOTIFY memoryEntriesChanged)

public:
    ApplicationController(std::unique_ptr<IChatProvider> provider,
                          std::unique_ptr<IMemoryStore> memoryStore,
                          std::unique_ptr<ChatSession> chatSession = nullptr,
                          QObject* parent = nullptr);

    QString providerName() const;
    QString providerStatus() const;
    QString memoryStatus() const;
    const QList<ChatMessage>& chatHistory() const;
    QStringList chatMessages() const;
    QStringList memoryEntries() const;

    Q_INVOKABLE bool sendMessage(const QString& message);
    Q_INVOKABLE void clearChat();
    Q_INVOKABLE void remember(const QString& key, const QString& value);

signals:
    void chatMessagesChanged();
    void memoryEntriesChanged();

private:
    std::unique_ptr<IChatProvider> provider_;
    std::unique_ptr<IMemoryStore> memoryStore_;
    std::unique_ptr<ChatSession> chatSession_;
};

} // namespace sentinel::core
