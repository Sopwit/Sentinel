# AI Context

## Vision

Sentinel is a native desktop-first AI operating layer. The current product is a Desktop Alpha foundation for local UI, provider boundaries, memory storage, settings storage, and future extension points.

## Target Platform

- Primary: Fedora KDE Plasma.
- Current implementation: Linux-native Qt desktop app.
- Long-term platforms are out of scope for current implementation work unless a phase explicitly adds them.

## Core Principles

- Native desktop over web shell.
- C++ core with QML presentation.
- Clear interfaces before feature expansion.
- Local-first defaults.
- Minimal dependencies.
- Storage responsibilities stay separated.
- QML must not own business logic or persistence.

## Current Technical Foundation

- C++20 and Qt 6.
- CMake build.
- QML UI shell.
- `ApplicationController` coordinates core behavior.
- `DesktopShellViewModel` is the QML boundary.
- `IChatProvider` isolates provider behavior.
- `IMemoryStore` isolates memory storage.
- `ISettingsStore` isolates settings storage.
- `SQLiteMemoryStore` persists explicit key-value memory entries.
- `JsonSettingsStore` persists application settings.
- Tests cover core behavior and persistence boundaries.

## Current Phase State

- Completed: Phase 3.1.
- Current: Phase 3.1.5, AI Context & Agent Instruction Layer.
- Not started: Phase 3.2.

Phase 3.1.5 is documentation and context infrastructure only. It must not change runtime behavior.
