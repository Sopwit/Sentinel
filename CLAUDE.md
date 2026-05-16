# Claude Code Instructions

## Working Style

- Read `AGENTS.md` and `docs/AI_CONTEXT.md` before making changes.
- Inspect the relevant source files before editing.
- Keep edits narrow and consistent with existing patterns.
- Prefer implementation through existing interfaces.
- Avoid broad refactors unless explicitly requested.
- Preserve user changes already present in the worktree.

## Current Completed Phase Summary

Phase 7.1 is complete.

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
- Phase 5.2, Adaptive Layout and Responsive Shell Foundation.
- Phase 5.3, Component Consistency and Visual QA Pass.
- Phase 5.4, Workspace UX Integration.
- Phase 5.5, Visual Identity Reconstruction and QML runtime stabilization.
- Phase 6.0, Functional Workspace and Model-Orchestration Skeleton.
- Phase 6.1, Routing Mode Settings and Persistence.
- Phase 6.2, Provider Catalog Metadata Skeleton.
- Phase 6.3, Capability Graph and Task Planner Skeleton.
- Phase 6.4, Agent System Skeleton.
- Phase 6.5, Memory Taxonomy and Semantic Metadata Skeleton.
- Phase 6.6, Orchestration Snapshot and Workspace State Skeleton.
- Phase 6.7, Orchestration Diagnostics and Readiness Checklist.
- Phase 6.8, Runtime Context Session Layer.
- Phase 6.9, Conversation State Graph Skeleton.
- Phase 6.10, Pre-runtime Architecture Checkpoint and Stabilization.
- Phase 7.0, Local Runtime Boundary Skeleton.
- Phase 7.1, Local Runtime Session Ownership Skeleton.

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
- Do not add real provider/model/agent or semantic memory execution; Phase 6.x model routing,
  catalog data, task planning, agent registry, memory taxonomy, orchestration snapshot metadata,
  diagnostics/readiness/session context metadata, and conversation state transitions are
  metadata-only.
- Do not start Phase 7 runtime execution from checkpoint work. Phase 7.0 should begin with local
  runtime boundary planning/ownership unless explicitly scoped otherwise.
- Phase 7.0 local runtime boundary is metadata-only and refuses execution. Do not add local
  provider calls, model execution, process launch, downloads, streaming, tools, or plugins.
- Phase 7.1 local runtime sessions are ownership/lifecycle metadata only. Do not start processes,
  allocate real models, probe runtimes, scan filesystems, stream output, or execute tools.
- Do not introduce Electron or a Python backend.

## Expected Output After Implementations

When code or documentation changes are complete, report:

- Files changed.
- Purpose of the change.
- Tests run, or why tests were not run.
- Any behavior intentionally left unchanged.
- Any known follow-up work.

Keep summaries concise and factual.
