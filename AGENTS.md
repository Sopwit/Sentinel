# Sentinel Agent Instructions

## Project Summary

Sentinel is a Linux-native, desktop-first AI operating layer. The current repository contains the native Desktop Alpha foundation, not the full long-term product.

Primary target: Fedora KDE Plasma.

## Current Phase

Phase 3.1 is completed.

Current work: Phase 3.1.5, AI Context & Agent Instruction Layer.

Phase 3.2 has not started.

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
- Do not introduce Electron, a Python backend, or unnecessary dependencies.

## Persistence Rules

- Settings and memory persistence must remain separate.
- SQLite access must go through Qt SQL.
- Memory path:
  - `QStandardPaths::AppDataLocation + "/memory.sqlite3"`
- Settings path:
  - `QStandardPaths::AppConfigLocation + "/settings.json"`
- Chat history is not persisted yet.
- Do not add Phase 3.2 persistence behavior during Phase 3.1.5.

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

Create and maintain the Phase 3.1.5 AI context layer. This is documentation and context infrastructure only.
