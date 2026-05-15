# Sentinel Agent Instructions

## Project Summary

Sentinel is a cross-platform Qt/QML-based personal AI assistant desktop application, optimized first for Linux/Fedora KDE Plasma. The current repository contains the Desktop Alpha foundation, not the full long-term product.

Primary optimization target: Fedora KDE Plasma.

Compatibility target: keep Windows and macOS possible through portable Qt/C++ architecture.

## Current Phase

Phase 4.8 is complete.

Most recent work: Phase 4.8, Agent Activity Log and Audit Trail Skeleton.

Current planning focus: post-Phase 4.8 stabilization while keeping real tool execution
non-operational.

## Stack

- C++20
- Qt 6
- QML
- CMake
- SQLite through `Qt6::Sql` and `QSQLITE`

## Architecture Rules

- Preserve the modular monolith structure.
- Keep C++ core logic separate from QML presentation.
- Expose QML-safe view models, not raw core objects.
- Keep provider behavior behind `IChatProvider`.
- Keep memory behavior behind `IMemoryStore`.
- Keep settings behavior behind `ISettingsStore`.
- Keep platform-specific behavior behind explicit service interfaces.
- Avoid Linux-only assumptions in core logic.
- Do not introduce Electron, a Python backend, or unnecessary dependencies.

## Platform Strategy

- Core logic should remain portable across Linux, Windows, and macOS.
- Linux integrations may be richer, especially for Fedora KDE Plasma.
- Future platform services should use interfaces such as:
  - `IPlatformService`
  - `IPathProvider`
  - `INotificationService`
  - `ISystemIntegrationService`
- Platform-specific code belongs behind these boundaries, not in controllers, stores, or QML pages.

## Persistence Rules

- Settings and memory persistence must remain separate.
- SQLite access must go through Qt SQL.
- Memory path:
  - `QStandardPaths::AppDataLocation + "/memory.sqlite3"`
- Settings path:
  - `QStandardPaths::AppConfigLocation + "/settings.json"`
- Chat history path:
  - `QStandardPaths::AppDataLocation + "/chat_history.sqlite3"`
- Chat history persistence must stay separate from key-value memory.
- Do not overload `IMemoryStore` for chat messages.
- Chat history is currently one local transcript, not multi-conversation storage.

## Coding Rules

- Follow existing C++ and QML patterns.
- Keep changes small and scoped.
- Prefer existing interfaces over new abstractions.
- Do not move business logic into QML.
- Do not refactor unrelated code.
- Do not modify application logic unless the active task explicitly requires it.

## Build And Test Expectations

Preferred workflow:

```bash
cmake --preset tests
cmake --build --preset tests
ctest --preset tests
```

Equivalent explicit workflow:

```bash
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

Run focused tests when changes are narrow. Run the full suite before broad or shared behavior changes.

## Documentation Rules

- Keep documentation concise and durable.
- Avoid speculative architecture.
- Update phase and decision docs when architecture or phase boundaries change.
- Do not duplicate long explanations already covered elsewhere.

## Current Priority

Maintain provider/agent/tool/sandbox/execution-boundary separation and keep real tool execution
out of scope until explicitly approved.
