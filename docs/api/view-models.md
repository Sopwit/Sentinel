# QML View-Models & Presentation API

Sentinel exposes C++ view models to QML via Qt's meta-object system (`QObject`, `Q_PROPERTY`, `Q_INVOKABLE`, `NOTIFY` signals).

---

## 1. `MainViewModel`

Exposes main app state, active view navigation, theme switching, and global status.

### Q_PROPERTY API

```cpp
Q_PROPERTY(QString activeWorkspace READ activeWorkspace WRITE setActiveWorkspace NOTIFY activeWorkspaceChanged)
Q_PROPERTY(bool isDarkTheme READ isDarkTheme WRITE setIsDarkTheme NOTIFY themeChanged)
Q_PROPERTY(bool providerHealthy READ providerHealthy NOTIFY providerStatusChanged)
Q_PROPERTY(QString providerVersion READ providerVersion NOTIFY providerStatusChanged)
```

### Q_INVOKABLE Methods

- `void toggleTheme()`: Switches between dark and light liquid glass themes.
- `void openCommandPalette()`: Emits signal to present command palette overlay.
- `void triggerDiagnosticsExport()`: Initiates local user-approved diagnostics export.

---

## 2. `ChatViewModel`

Exposes conversation transcript data, message lists, prompt state, and model selection.

### Q_PROPERTY API

```cpp
Q_PROPERTY(QVariantList messages READ messages NOTIFY messagesChanged)
Q_PROPERTY(QString selectedModel READ selectedModel WRITE setSelectedModel NOTIFY selectedModelChanged)
Q_PROPERTY(QStringList availableModels READ availableModels NOTIFY availableModelsChanged)
```

### Q_INVOKABLE Methods

- `void sendMessage(const QString& text)`: Posts a new user prompt into local conversation transcript.
- `void clearHistory()`: Clears active workspace chat history.
