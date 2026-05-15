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

`main.cpp` creates `ApplicationController`, `FakeProvider`, `InMemoryStore`, and `ModeManager`, then exposes only the controller and mode manager to QML.

QML handles layout and user input. C++ owns chat handling, provider calls, mode state, and memory state.

## Intentional Boundaries

- Provider behavior is hidden behind `IProvider`.
- Memory behavior is hidden behind `IMemoryStore`.
- Future plugin behavior starts at `IPlugin`.
- Context construction starts at `IContextEngine`.
- UI code does not call network APIs or own business rules.

## Memory Storage Contract

`IMemoryStore` is intentionally storage-backend independent. It stores exact key/value pairs and returns entries through a small value-based contract. Application-level validation, such as rejecting blank keys, belongs in controllers or services rather than storage implementations.

`InMemoryStore` remains the default development and test backend. A future `SQLiteMemoryStore` should implement `IMemoryStore` without requiring changes to `ApplicationController`.

## Not Implemented Yet

- Real AI providers.
- SQLite persistence.
- Voice input or output.
- Automation agents.
- Cloud sync.
- Runtime plugin loading.
- Wearable or edge-device support.
