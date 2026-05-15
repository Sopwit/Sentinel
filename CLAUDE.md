# Claude Code Instructions

## Working Style

- Read `AGENTS.md` and `docs/AI_CONTEXT.md` before making changes.
- Inspect the relevant source files before editing.
- Keep edits narrow and consistent with existing patterns.
- Prefer implementation through existing interfaces.
- Avoid broad refactors unless explicitly requested.
- Preserve user changes already present in the worktree.

## Current Completed Phase Summary

Phase 5.1 is complete.

Implemented foundation:

- Qt/QML desktop shell.
- C++20 core library.
- `JsonSettingsStore`.
- `SQLiteMemoryStore`.
- `IChatHistoryStore`.
- `SQLiteChatHistoryStore`.
- SQLite persistence tests.
- Persistence separation between settings and memory.
- Persistence separation between memory and chat history.
- Chat history status and clear-confirmation UX.
- Documentation foundation.

Current direction:

- Cross-platform Qt/QML desktop application.
- Optimized first for Linux/Fedora KDE Plasma.
- Keep Windows and macOS compatibility possible.
- Prepare platform-specific behavior behind clean service interfaces.

Recent stabilization phase:

- Phase 3.1.5, AI Context & Agent Instruction Layer.

Recent implementation phase:

- Phase 3.2, Persistent Chat History Preparation.
- Phase 3.3, Chat History UX and Lifecycle Controls.
- Phase 3.4, Cross-platform Architecture Readiness and Storage Maintenance.
- Phase 3.5, Pre-agent Architecture Audit and Release Checkpoint.
- Phase 4.0, Agent Core Planning and Minimal Runtime Skeleton.
- Phase 4.1, Tool Descriptor and Registry Skeleton.
- Phase 4.2, Tool Invocation Planning Layer.
- Phase 4.3, Approval and Permission Metadata Skeleton.
- Phase 4.4, Sandbox and Capability Boundary Skeleton.
- Phase 4.5, Execution Boundary Skeleton.
- Phase 4.6, Agent Runtime Pipeline Stabilization.
- Phase 4.7, Runtime Context and Tool Session Skeleton.
- Phase 4.8, Agent Activity Log and Audit Trail Skeleton.
- Phase 4.9, Agent Pipeline UI Visibility.
- Phase 4.10, Architecture Checkpoint and Cleanup.
- Phase 4.11, AI Orchestration Planning Checkpoint.
- Phase 5.0, UI/UX Planning and Design System Foundation.
- Phase 5.1, Motion and Interaction Foundation.

## Constraints

- Do not merge chat history into key-value memory.
- Do not expose raw SQLite details to QML.
- Do not add multi-conversation/thread support yet.
- Do not add encryption, export, or pruning yet.
- Do not hardcode Linux-only assumptions into core logic.
- Do not add platform-specific integrations without an interface boundary.
- Do not add dependencies.
- Do not start full Phase 4 implementation.
- Do not add real tool execution; Phase 4.2 plans, Phase 4.3 approval decisions, Phase 4.4
  sandbox capability decisions, and Phase 4.5 execution results are metadata/placeholder-only.
- Do not introduce Electron or a Python backend.

## Expected Output After Implementations

When code or documentation changes are complete, report:

- Files changed.
- Purpose of the change.
- Tests run, or why tests were not run.
- Any behavior intentionally left unchanged.
- Any known follow-up work.

Keep summaries concise and factual.
