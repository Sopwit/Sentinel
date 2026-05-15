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

`main.cpp` creates `ApplicationController`, `FakeProvider`, `InMemoryStore`, `ModeManager`, `AppSettings`, and `DesktopShellViewModel`, then exposes only the view model to QML.

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

- Provider behavior is hidden behind `IProvider`.
- Memory behavior is hidden behind `IMemoryStore`.
- Future plugin behavior starts at `IPlugin`.
- Context construction starts at `IContextEngine`.
- UI code does not call network APIs or own business rules.

## Memory Storage Contract

`IMemoryStore` is intentionally storage-backend independent. It stores exact key/value pairs and returns entries through a small value-based contract. Application-level validation, such as rejecting blank keys, belongs in controllers or services rather than storage implementations.

`InMemoryStore` remains the default development and test backend. A future `SQLiteMemoryStore` should implement `IMemoryStore` without requiring changes to `ApplicationController`.

## Settings Contract

`ISettingsStore` is the persistence boundary for app settings. `AppSettings` owns defaults and validation. `InMemorySettingsStore` remains the default test backend, while `JsonSettingsStore` provides a lightweight desktop persistence backend.

The desktop app stores settings below Qt's `AppConfigLocation`. Future settings backends should implement `ISettingsStore` without changing QML or the desktop view model.

## Plugin And Integration Boundaries

`IPlugin` describes future in-process plugin lifecycle boundaries. `IIntegration` describes future external or local integration metadata. Neither interface loads code, performs network calls, or talks to operating-system services in the alpha.

## Not Implemented Yet

- Real AI providers.
- SQLite persistence.
- Voice input or output.
- Automation agents.
- Cloud sync.
- Runtime plugin loading.
- Wearable or edge-device support.
