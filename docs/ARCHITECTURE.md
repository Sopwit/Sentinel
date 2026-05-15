# Sentinel Architecture

Sentinel Desktop Alpha is a modular monolith. The application is split into a native Qt/QML shell and a small C++ core library.

## Layers

- `apps/sentinel-desktop`: desktop application entry point and QML engine setup.
- `core`: provider, memory, mode, context, plugin, and application coordination interfaces.
- `ui/qml`: desktop shell and presentation layer.
- `integrations`: placeholder for future local and provider integrations.
- `plugins`: placeholder for future plugin contracts and loading.
- `tests`: placeholder for automated tests.

## Current Runtime Flow

`main.cpp` creates `ApplicationController`, `LocalEchoProvider`, `SQLiteMemoryStore`, `ModeManager`, `AppSettings`, and `DesktopShellViewModel`, then exposes only the view model to QML.

QML handles layout and user input. C++ owns chat handling, provider calls, mode state, memory state, and settings defaults.

## Desktop View Model

`DesktopShellViewModel` is the QML boundary for the desktop app. It forwards safe operations to the core controller, mode manager, and settings object:

- provider name
- current mode and available modes
- chat messages and send action
- runtime memory entries and write action
- theme/configuration placeholder settings

Raw core objects are not exposed directly to QML.

## QML Structure

The desktop shell is split into small QML components:

- `Main.qml`: application window and high-level layout.
- `components/Sidebar.qml`: page navigation and provider/settings summary.
- `components/HeaderBar.qml`: current mode and mode switcher.
- `components/StatusBar.qml`: local alpha status footer.
- `pages/DashboardPage.qml`: overview and chat panel host.
- `pages/MemoryPage.qml`: runtime memory UI.
- `pages/SettingsPage.qml`: settings placeholder UI.

These files bind to `shellViewModel`. They should not own business rules, provider logic, persistence logic, or platform automation.

## Intentional Boundaries

- Chat provider behavior is hidden behind `IChatProvider`.
- Memory behavior is hidden behind `IMemoryStore`.
- Future plugin behavior starts at `IPlugin`.
- Context construction starts at `IContextEngine`.
- UI code does not call network APIs or own business rules.

## Memory Storage Contract

`IMemoryStore` is intentionally storage-backend independent. It stores exact key/value pairs and returns entries through a small value-based contract. Application-level validation, such as rejecting blank keys, belongs in controllers or services rather than storage implementations.

`InMemoryStore` remains the default lightweight development and test backend. `SQLiteMemoryStore` implements the same `IMemoryStore` contract for desktop persistence without requiring changes to `ApplicationController`.

The desktop app stores memory below Qt's `AppDataLocation` as `memory.sqlite3`. Settings remain separate in `JsonSettingsStore` below Qt's `AppConfigLocation`.

SQLite stores only explicit key-value memory entries. Chat history remains in-memory and is not persisted yet.

## Settings Contract

`ISettingsStore` is the persistence boundary for app settings. `AppSettings` owns defaults and validation. `InMemorySettingsStore` remains the default test backend, while `JsonSettingsStore` provides a lightweight desktop persistence backend.

The desktop app stores settings below Qt's `AppConfigLocation`. Future settings backends should implement `ISettingsStore` without changing QML or the desktop view model.

## Plugin And Integration Boundaries

`IPlugin` describes future in-process plugin lifecycle boundaries. `IIntegration` describes future external or local integration metadata. Neither interface loads code, performs network calls, or talks to operating-system services in the alpha.

## Provider Contract

`IChatProvider` is the local chat pipeline boundary. Providers expose a name, a status, and a deterministic `sendMessage` result. `ApplicationController` owns blank-message validation, unavailable-provider handling, and chat transcript formatting.

`LocalEchoProvider` is the only provider in the alpha. It performs no network calls, reads no API keys, and returns a stable local response for UI and tests.

Future real providers should implement `IChatProvider` behind explicit configuration and status handling. Network transport, credentials, retries, streaming, and model selection are intentionally not part of Phase 2.2.

## Chat Session Pipeline

Chat history is owned by `ChatSession`, an in-memory session model with structured `ChatMessage` entries:

- message id
- role: `system`, `user`, or `assistant`
- content
- timestamp from an injectable clock
- status: `sent`, `received`, or `error`

`ApplicationController::sendMessage` validates input, appends a user message, calls `IChatProvider`, then appends an assistant reply or error message. Blank messages do not mutate history.

The desktop layer exposes history through `ChatMessageListModel`, a QML-safe `QAbstractListModel`. QML reads roles and never receives raw core objects.

Chat history is not persisted yet. Future persistence should be added behind a dedicated storage boundary, not by writing from QML.

## Not Implemented Yet

- Real AI providers.
- Voice input or output.
- Automation agents.
- Cloud sync.
- Runtime plugin loading.
- Wearable or edge-device support.
