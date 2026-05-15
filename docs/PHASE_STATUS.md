# Phase Status

## Completed / Stable

### Phase 1: Desktop Alpha Foundation

Created the native Qt/QML desktop shell, C++ core interfaces, local provider boundary, runtime memory store, mode handling, and initial documentation.

### Phase 2: Provider And Application Core Foundation

Established provider, controller, chat session, settings, plugin, integration, and context interfaces without adding real network providers.

### Phase 3: SQLite Memory Foundation

Added SQLite-backed memory persistence through `SQLiteMemoryStore` while keeping the `IMemoryStore` contract storage-backend independent.

### Phase 3.1: Persistence Stabilization

Added `JsonSettingsStore`, kept settings and memory persistence separate, expanded SQLite persistence tests, and documented storage boundaries.

### Phase 3.1.5: AI Context & Agent Instruction Layer

Adds lightweight repository-local AI instructions and context documents for Codex, Claude, ChatGPT, and similar coding agents.

Scope:

- Prompt compression.
- Stable architecture guardrails.
- Phase status memory.
- Decision history.
- Agent working instructions.

This phase must not change application logic, SQLite logic, QML architecture, build behavior, or dependencies.

### Phase 3.2: Persistent Chat History Preparation

Added a dedicated chat history persistence boundary and SQLite-backed implementation.

Scope:

- `IChatHistoryStore`.
- `SQLiteChatHistoryStore`.
- Separate chat history database.
- Optional controller persistence loading and appending.
- Runtime chat remains available if persistence is unavailable.
- Tests for chat history storage and controller integration.

Chat history persistence is separate from key-value memory storage.

### Phase 3.3: Chat History UX and Lifecycle Controls

Completed. Adds minimal chat history lifecycle UX on top of the Phase 3.2 persistence boundary.

Scope:

- Generic chat history availability/status in the desktop view model.
- Status bar and chat panel visibility for chat history state.
- Clear-chat confirmation before clearing runtime and persistent history.
- Startup-loaded messages exposed through `ChatMessageListModel`.
- Tests for desktop view-model chat history status and restored messages.

Current limitation:

- Chat history is one local transcript.
- No multi-conversation/thread support.
- No encryption, export, or pruning.

## Current Alignment Work

### Phase 3.4: Cross-platform Architecture Readiness

Completed. Added architecture boundaries and storage maintenance surfaces without platform-specific runtime integrations.

Scope:

- Added platform boundary interfaces:
  - `IPathProvider`
  - `IPlatformService`
  - `INotificationService` (lightweight placeholder)
  - `ISystemIntegrationService` (lightweight placeholder)
- Added default `StandardPathProvider` with Qt `QStandardPaths` ownership for:
  - settings path
  - memory database path
  - chat history database path
- Added storage maintenance controls:
  - clear memory store
  - clear chat history store with runtime-safe fallback
  - settings persistence remains separate and unaffected
- Added controller/view-model generic maintenance status surfaces for QML.
- Added settings page controls for local memory/chat clear actions with confirmation dialogs.
- Added tests for path behavior, clear behavior, unavailable stores, and settings isolation.

Candidate future interfaces:

- `IPlatformService`
- `IPathProvider`
- `INotificationService`
- `ISystemIntegrationService`

## Upcoming

### Phase 4: Agent Core & Tool System

Not started. Do not implement Phase 4 behavior without an explicit task.
