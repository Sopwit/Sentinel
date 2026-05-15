# Architecture Decisions

## 1. Native Qt Desktop

Decision: Sentinel uses C++20, Qt 6, and QML for the desktop alpha.

Reason: The product is desktop-first and should integrate with native Linux desktop environments without a web-shell runtime.

Avoided:

- Electron.
- Browser-first desktop shell.
- Python backend for the app core.

## 2. Modular Monolith

Decision: Keep the repository as a modular monolith with clear internal boundaries.

Reason: The project is still in alpha. Interfaces give enough separation without the operational cost of separate services or packages.

## 3. C++ Core Owns Business Logic

Decision: Core behavior belongs in C++ classes and interfaces. QML owns presentation and user interaction.

Reason: This keeps behavior testable, deterministic, and independent from UI layout.

## 4. QML Boundary Through View Models

Decision: QML interacts with `DesktopShellViewModel` and QML-safe models.

Reason: Raw core objects should not leak into the UI layer.

## 5. Provider Isolation

Decision: Chat providers are hidden behind `IChatProvider`.

Reason: Real providers, local providers, status handling, and future configuration can evolve without changing UI code.

## 6. Memory Persistence Boundary

Decision: Memory storage is hidden behind `IMemoryStore`.

Reason: In-memory and SQLite implementations must share one small contract.

Storage decision:

- Desktop memory is stored at `QStandardPaths::AppDataLocation + "/memory.sqlite3"`.
- SQLite is accessed through Qt SQL and `QSQLITE`.

## 7. Settings Persistence Boundary

Decision: Settings storage is hidden behind `ISettingsStore`.

Reason: Settings defaults, validation, and persistence can evolve without coupling QML to file IO.

Storage decision:

- Desktop settings are stored at `QStandardPaths::AppConfigLocation + "/settings.json"`.

## 8. Settings And Memory Stay Separate

Decision: Settings and memory persistence use separate stores and separate files.

Reason: Settings are user/application configuration. Memory entries are runtime AI context data. Combining them would blur ownership and migration paths.

## 9. Chat History Persistence Boundary

Decision: Chat history persistence is hidden behind `IChatHistoryStore`.

Reason: Chat messages are ordered conversation records, not key-value memory. They need their own contract, schema, clear operation, and migration path.

Storage decision:

- Desktop chat history is stored at `QStandardPaths::AppDataLocation + "/chat_history.sqlite3"`.
- SQLite is accessed through Qt SQL and `QSQLITE`.
- Stored fields are `id`, `role`, `content`, `timestamp`, and `status`.
- Rows load in ascending `id` order.
- The schema metadata table stores `schema_version = 1`.

Runtime behavior:

- `ApplicationController` loads persisted messages when available.
- New runtime messages are appended to the chat history store when available.
- Clearing chat clears runtime and persistent chat history when available.
- Runtime chat continues if chat persistence is unavailable.

## 10. AI Context Layer

Decision: Add repository-local AI instruction files in Phase 3.1.5.

Reason: Future AI coding sessions need a compact source of truth for phase status, architecture rules, and prompt guidance.

Expected effect:

- Smaller prompts.
- Less repeated context.
- Lower risk of architecture drift.
- Consistent constraints across Codex, Claude, ChatGPT, and similar agents.
