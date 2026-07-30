# C++ Core Interfaces Reference

Sentinel's C++ core architecture relies on abstract C++ interfaces to ensure modularity, testability, and decoupled implementation.

---

## 1. `IChatProvider`

Header: `core/include/sentinel/provider/IChatProvider.hpp`

Abstract interface representing AI model provider interaction and endpoint health checking.

```cpp
namespace sentinel::provider {

class IChatProvider {
public:
    virtual ~IChatProvider() = default;

    virtual QString providerName() const = 0;
    virtual bool isAvailable() const = 0;
    virtual QString endpointUrl() const = 0;
    virtual QStringList availableModels() const = 0;
    virtual void checkHealth(std::function<void(bool healthy, const QString& version)> callback) = 0;
    virtual void discoverModels(std::function<void(const QStringList& models)> callback) = 0;
};

} // namespace sentinel::provider
```

---

## 2. `IMemoryStore`

Header: `core/include/sentinel/memory/IMemoryStore.hpp`

Abstract interface for key-value memory store persistence.

```cpp
namespace sentinel::memory {

class IMemoryStore {
public:
    virtual ~IMemoryStore() = default;

    virtual bool initialize(const QString& dbPath) = 0;
    virtual bool setMemory(const QString& key, const QString& value, const QString& scope) = 0;
    virtual std::optional<QString> getMemory(const QString& key, const QString& scope) const = 0;
    virtual bool deleteMemory(const QString& key, const QString& scope) = 0;
    virtual QList<MemoryItem> listMemories(const QString& scope) const = 0;
};

} // namespace sentinel::memory
```

---

## 3. `ISettingsStore`

Header: `core/include/sentinel/settings/ISettingsStore.hpp`

Abstract interface for JSON settings persistence.

```cpp
namespace sentinel::settings {

class ISettingsStore {
public:
    virtual ~ISettingsStore() = default;

    virtual bool load(const QString& configPath) = 0;
    virtual bool save() = 0;
    virtual QVariant getValue(const QString& key, const QVariant& defaultValue = {}) const = 0;
    virtual void setValue(const QString& key, const QVariant& value) = 0;
};

} // namespace sentinel::settings
```
