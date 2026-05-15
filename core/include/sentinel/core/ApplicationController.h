#pragma once

#include "sentinel/core/IMemoryStore.h"
#include "sentinel/core/IProvider.h"

#include <QObject>
#include <QStringList>
#include <memory>

namespace sentinel::core {

class ApplicationController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString providerName READ providerName CONSTANT)
    Q_PROPERTY(QStringList chatMessages READ chatMessages NOTIFY chatMessagesChanged)
    Q_PROPERTY(QStringList memoryEntries READ memoryEntries NOTIFY memoryEntriesChanged)

public:
    ApplicationController(std::unique_ptr<IProvider> provider,
                          std::unique_ptr<IMemoryStore> memoryStore, QObject* parent = nullptr);

    QString providerName() const;
    QStringList chatMessages() const;
    QStringList memoryEntries() const;

    Q_INVOKABLE void sendMessage(const QString& message);
    Q_INVOKABLE void remember(const QString& key, const QString& value);

signals:
    void chatMessagesChanged();
    void memoryEntriesChanged();

private:
    std::unique_ptr<IProvider> provider_;
    std::unique_ptr<IMemoryStore> memoryStore_;
    QStringList chatMessages_;
};

} // namespace sentinel::core
