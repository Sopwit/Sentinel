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
- `IMemoryStore` isolates memory storage.
- `IChatHistoryStore` isolates chat history storage.
- `ISettingsStore` isolates settings storage.
- `SQLiteMemoryStore` persists explicit key-value memory entries.
- `SQLiteChatHistoryStore` persists chat messages.
- `JsonSettingsStore` persists application settings.
- Desktop UI exposes generic chat history status without SQLite details.
- Tests cover core behavior and persistence boundaries.

## Current Phase State

- Completed: Phase 3.1, Phase 3.1.5, Phase 3.2, Phase 3.3, Phase 3.4, Phase 3.5, and Phase 4.0.
- Current: Desktop alpha with minimal local agent runtime skeleton and explicit provider/agent separation.
- Next: Phase 4.1 planning with continued local-safe constraints.
- Recent: Phase 4.0, Agent Core Planning and Minimal Runtime Skeleton.

Current runtime still has no networking, API keys, real provider integrations, plugin loading, privileged automation, multi-conversation support, encryption, export, pruning, or platform-specific service implementations.
