# Architecture Decisions

## 1. Cross-platform Qt Desktop

Decision: Sentinel uses C++20, Qt 6, and QML for a cross-platform desktop application.

Reason: The product is desktop-first and should remain portable while allowing a Linux/Fedora KDE Plasma optimized experience.

Avoided:

- Electron.
- Browser-first desktop shell.
- Python backend for the app core.
- Linux-only core assumptions.

## 1.1 Platform Abstraction Direction

Decision: Keep platform-specific behavior behind explicit service interfaces.

Reason: Linux integrations may be richer, but the core architecture must remain portable to Windows and macOS.

Planned interfaces:

- `IPlatformService`
- `IPathProvider`
- `INotificationService`
- `ISystemIntegrationService`

Rule: platform services should not leak into storage contracts, provider contracts, controllers, or QML pages.

Phase 3.4 implementation status:

- Added `IPathProvider`.
- Added `IPlatformService`.
- Added `INotificationService` as a lightweight placeholder.
- Added `ISystemIntegrationService` as a lightweight placeholder.
- Kept these as boundaries only; no OS-specific integration/automation implementation yet.

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
- Path ownership now routes through `IPathProvider` (`StandardPathProvider` by default).

Lifecycle decision:

- Memory maintenance includes a clear operation through the `IMemoryStore` boundary.
- UI receives generic maintenance status strings only.

## 7. Settings Persistence Boundary

Decision: Settings storage is hidden behind `ISettingsStore`.

Reason: Settings defaults, validation, and persistence can evolve without coupling QML to file IO.

Storage decision:

- Desktop settings are stored at `QStandardPaths::AppConfigLocation + "/settings.json"`.
- Path ownership now routes through `IPathProvider` (`StandardPathProvider` by default).

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
- If persistent chat storage is unavailable, runtime clear still succeeds with generic runtime-only status.
- Runtime chat continues if chat persistence is unavailable.
- QML receives only generic chat history status, such as `Available` or `Runtime Only`.
- QML receives only generic maintenance statuses, such as `Ready`, `Clear completed`, `Runtime Only`, or
  `Unavailable`.
- The desktop UI confirms before clearing local chat history.

Data ownership rule:

- Local data categories remain explicit and separate:
  - Settings (`ISettingsStore`)
  - Memory (`IMemoryStore`)
  - Chat history (`IChatHistoryStore`)
- Clear actions for memory/chat must not delete settings.

Current limitation:

- Chat history is a single local transcript.
- Multi-conversation/thread support is intentionally not implemented yet.
- Encryption, export, and pruning are intentionally not implemented yet.

## 10. AI Context Layer

Decision: Add repository-local AI instruction files in Phase 3.1.5.

Reason: Future AI coding sessions need a compact source of truth for phase status, architecture rules, and prompt guidance.

Expected effect:

- Smaller prompts.
- Less repeated context.
- Lower risk of architecture drift.
- Consistent constraints across Codex, Claude, ChatGPT, and similar agents.

## 11. UI/UX Roadmap Direction

Decision: Keep current UI as a foundation while planning a more mature Qt Quick experience.

Current state:

- Qt/QML shell.
- Chat, memory, settings, dashboard, sidebar, header, and status bar.
- Minimal lifecycle UX around chat history.

Future direction:

- Smooth animations.
- Responsive layouts.
- Adaptive themes.
- Assistant-like chat interaction.
- Animated panels.
- Dashboard cards.
- Extensible component system.

Rule: future UI polish should stay behind QML/view-model boundaries and must not introduce business logic into QML.

## 12. Pre-agent Release Checkpoint

Decision: Add a stabilization checkpoint before Phase 4 implementation work.

Reason: The architecture needs an explicit audit gate so agent/tool runtime work does not begin on top of stale docs, inconsistent wording, or accidental boundary drift.

Checkpoint criteria:

- Core boundaries and platform boundaries remain explicit and interface-driven.
- QML exposure stays generic and does not leak SQLite/platform internals.
- Persistence separation remains strict across settings, memory, and chat history.
- Coverage exists for path ownership and maintenance status behavior.
- Release verification (`cmake --preset tests`, `cmake --build --preset tests`, `ctest --preset tests`) and formatting checks pass.

Out of scope during this checkpoint:

- Phase 4 agent/tool runtime implementation.
- Real provider/network integration and API keys.
- Plugin loading.
- Privileged automation.

## 13. Provider And Agent Runtime Separation

Decision: Keep provider and agent runtime boundaries separate.

Reason: Chat response generation and orchestration/tool planning are different concerns and should evolve independently without forcing cross-layer coupling.

Boundary rules:

- `IChatProvider` remains the chat generation contract.
- `IAgentRuntime` is the orchestration/runtime boundary for future action flows.
- Controllers and QML consume generic agent status/response surfaces, not runtime internals.
- Phase 4.0 runtime is local deterministic skeleton only (`NullAgentRuntime`) with no networking/tool execution.

## 14. Tool Descriptor And Registry Boundary

Decision: Separate tool metadata registration from tool execution.

Reason: The architecture needs a safe incremental path where tool identity, parameters, and risk metadata can be modeled and tested before any execution runtime is introduced.

Boundary rules:

- `ToolDescriptor` and related descriptor enums/structs model metadata only.
- `IToolRegistry` owns deterministic tool metadata registration and lookup.
- `InMemoryToolRegistry` is the default local deterministic registry implementation.
- Agent runtime may expose tool metadata from the registry, but must not execute tools in Phase 4.1.
