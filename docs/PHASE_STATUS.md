# Phase Status

## Completed

### Phase 1: Desktop Alpha Foundation

Created the native Qt/QML desktop shell, C++ core interfaces, local provider boundary, runtime memory store, mode handling, and initial documentation.

### Phase 2: Provider And Application Core Foundation

Established provider, controller, chat session, settings, plugin, integration, and context interfaces without adding real network providers.

### Phase 3: SQLite Memory Foundation

Added SQLite-backed memory persistence through `SQLiteMemoryStore` while keeping the `IMemoryStore` contract storage-backend independent.

### Phase 3.1: Persistence Stabilization

Added `JsonSettingsStore`, kept settings and memory persistence separate, expanded SQLite persistence tests, and documented storage boundaries.

## Current

### Phase 3.1.5: AI Context & Agent Instruction Layer

Adds lightweight repository-local AI instructions and context documents for Codex, Claude, ChatGPT, and similar coding agents.

Scope:

- Prompt compression.
- Stable architecture guardrails.
- Phase status memory.
- Decision history.
- Agent working instructions.

This phase must not change application logic, SQLite logic, QML architecture, build behavior, or dependencies.

## Upcoming

### Phase 3.2: Pending

Placeholder only. Do not implement Phase 3.2 behavior during Phase 3.1.5.
