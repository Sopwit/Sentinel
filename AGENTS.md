# Sentinel Agent Instructions

## Project Summary

Sentinel is a cross-platform Qt/QML-based personal AI assistant desktop application, optimized first for Linux/Fedora KDE Plasma. The repository contains the desktop full-version product in active development.

Primary optimization target: Fedora KDE Plasma.

Compatibility target: keep Windows and macOS possible through portable Qt/C++ architecture.

## Current Phase

The desktop prototype/alpha is complete. Sentinel is now in full-version development.

Most recent work: Phase 10.0, full execution enablement (removing the prototype's
metadata-only / execution-disabled boundaries).

Current focus: real local model inference, real tool/agent execution, real cloud provider
clients, real TTS/STT, and real semantic retrieval, while preserving the separation boundaries
described below.

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

Real model inference and real tool/agent execution are now in scope and approved:

- Local model prompt execution (Ollama / LM Studio / llama.cpp endpoints) is operational and
  enabled by default where a healthy local runtime is detected.
- Cloud provider clients (OpenAI, Claude, Gemini, DeepSeek, Groq, Mistral) are real
  REST/SSE implementations, gated by user-provided API credentials.
- Tool execution runs behind the existing approval/sandbox gates; agents execute real tasks
  through the approved tool gateway.
- TTS/STT (Piper, Whisper) execute via local subprocesses when binaries are configured.
- Semantic retrieval uses real local embeddings (Ollama embedding models by default).

Execution boundaries remain: every execution path still flows through
provider/agent/tool/sandbox gates, permissions, and explicit user approval where the policy
requires it. Replace placeholder/fake data with real behavior as the full version is built out;
remove dead code that no longer serves the product.
