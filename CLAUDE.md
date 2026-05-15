# Claude Code Instructions

## Working Style

- Read `AGENTS.md` and `docs/AI_CONTEXT.md` before making changes.
- Inspect the relevant source files before editing.
- Keep edits narrow and consistent with existing patterns.
- Prefer implementation through existing interfaces.
- Avoid broad refactors unless explicitly requested.
- Preserve user changes already present in the worktree.

## Current Completed Phase Summary

Phase 3.3 is complete.

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

Recent stabilization phase:

- Phase 3.1.5, AI Context & Agent Instruction Layer.

Recent implementation phase:

- Phase 3.2, Persistent Chat History Preparation.
- Phase 3.3, Chat History UX and Lifecycle Controls.

## Constraints

- Do not merge chat history into key-value memory.
- Do not expose raw SQLite details to QML.
- Do not add multi-conversation/thread support yet.
- Do not add encryption, export, or pruning yet.
- Do not add dependencies.
- Do not start the next phase without an explicit task.
- Do not introduce Electron or a Python backend.

## Expected Output After Implementations

When code or documentation changes are complete, report:

- Files changed.
- Purpose of the change.
- Tests run, or why tests were not run.
- Any behavior intentionally left unchanged.
- Any known follow-up work.

Keep summaries concise and factual.
