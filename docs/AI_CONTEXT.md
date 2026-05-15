# AI Context

## Vision

Sentinel is a cross-platform Qt/QML-based personal AI assistant desktop application, optimized first for Linux/Fedora KDE Plasma. The current product is a Desktop Alpha foundation for local UI, provider boundaries, memory storage, settings storage, and future extension points.

## Target Platform

- Primary optimization target: Fedora KDE Plasma.
- Compatibility goal: keep Windows and macOS possible.
- Current implementation: Qt/QML desktop app with portable C++ core boundaries.

## Core Principles

- Native desktop over web shell.
- C++ core with QML presentation.
- Clear interfaces before feature expansion.
- Portable core before platform-specific integration.
- Local-first defaults.
- Minimal dependencies.
- Storage responsibilities stay separated.
- QML must not own business logic or persistence.

## Platform Strategy

- Keep core application logic platform-neutral.
- Use Qt APIs where they preserve portability.
- Put platform-specific behavior behind future service interfaces:
  - `IPlatformService`
  - `IPathProvider`
  - `INotificationService`
  - `ISystemIntegrationService`
- Linux/Fedora KDE integrations may be deeper, but must not leak into shared core logic.

## UI Direction

Current UI foundation:

- Qt Quick/QML shell.
- Dashboard, chat, memory, settings, sidebar, header, and status bar.
- QML-safe view models.

Future UI vision:

- Modern Qt Quick UX.
- Smooth animations and animated panels.
- Responsive layouts and adaptive themes.
- Assistant-like chat-oriented interaction.
- Dashboard cards and extensible components.
- Modern desktop aesthetics without over-styling.

## Current Technical Foundation

- C++20 and Qt 6.
- CMake build.
- QML UI shell.
- `ApplicationController` coordinates core behavior.
- `DesktopShellViewModel` is the QML boundary.
- `IChatProvider` isolates provider behavior.
- `IAgentRuntime` isolates future agent orchestration behavior.
- `IToolRegistry` isolates tool metadata registration and lookup behavior.
- `ToolInvocationPlan` models proposed tool-use intent without execution.
- `IApprovalPolicy` evaluates planned tool invocations as approval metadata without execution.
- `ISandboxPolicy` evaluates planned and approved invocation metadata against capability boundaries
  without execution.
- `IToolExecutor` owns the future execution boundary but currently returns placeholder-only results
  without executing tools.
- `AgentPipelineResult` consolidates the metadata-only runtime pipeline result for planning,
  approval, sandbox, and placeholder execution status exposure.
- `AgentRuntimeContext` and `RuntimeSession` own local in-memory runtime context metadata derived
  from the latest pipeline result without persistence or execution authority.
- `AgentActivityLog` records in-memory metadata-only audit/activity events for the agent pipeline
  without persistence, secrets, paths, or OS interaction.
- Desktop UI shows read-only agent/runtime visibility for latest pipeline state, runtime context,
  active planned tool ids, and activity count/latest summary without execution or approval controls.
- Phase 4 checkpoint documentation records completed scope, known limitations, and Phase 5
  readiness criteria while keeping execution non-operational.
- `IMemoryStore` isolates memory storage.
- `IChatHistoryStore` isolates chat history storage.
- `ISettingsStore` isolates settings storage.
- `SQLiteMemoryStore` persists explicit key-value memory entries.
- `SQLiteChatHistoryStore` persists chat messages.
- `JsonSettingsStore` persists application settings.
- Desktop UI exposes generic chat history status without SQLite details.
- Tests cover core behavior and persistence boundaries.

## Current Phase State

- Completed: Phase 3.1, Phase 3.1.5, Phase 3.2, Phase 3.3, Phase 3.4, Phase 3.5, Phase 4.0, Phase 4.1, Phase 4.2, Phase 4.3, Phase 4.4, Phase 4.5, Phase 4.6, Phase 4.7, Phase 4.8, Phase 4.9, and Phase 4.10.
- Current: Desktop alpha with a stabilized metadata-only agent pipeline:
  registry -> planning -> approval -> sandbox capability metadata -> placeholder execution boundary,
  with local runtime context/session, in-memory activity logging for pipeline metadata, and
  read-only dashboard visibility for that state.
- Next: Phase 5 UI/UX planning with continued local-safe constraints.
- Recent: Phase 4.10, Architecture Checkpoint and Cleanup.

Current runtime still has no real tool execution, shell/process launch, filesystem mutation, networking, API keys, real provider integrations, plugin loading, privileged automation, multi-conversation support, encryption, export, pruning, real sandbox runtime, subprocess execution, or platform-specific service implementations.
