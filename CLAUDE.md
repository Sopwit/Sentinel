# Claude Code Instructions

## Working Style

- Read `AGENTS.md` and `docs/AI_CONTEXT.md` before making changes.
- Inspect the relevant source files before editing.
- Keep edits narrow and consistent with existing patterns.
- Prefer implementation through existing interfaces.
- Avoid broad refactors unless explicitly requested.
- Preserve user changes already present in the worktree.

## Current Completed Phase Summary

Phase 3.1 is complete.

Implemented foundation:

- Qt/QML desktop shell.
- C++20 core library.
- `JsonSettingsStore`.
- `SQLiteMemoryStore`.
- SQLite persistence tests.
- Persistence separation between settings and memory.
- Documentation foundation.

Current stabilization phase:

- Phase 3.1.5, AI Context & Agent Instruction Layer.

Not started:

- Phase 3.2.

## Constraints

- Do not modify application logic for Phase 3.1.5.
- Do not modify SQLite behavior.
- Do not modify QML architecture.
- Do not modify build system behavior.
- Do not add dependencies.
- Do not implement Phase 3.2 features.
- Do not introduce Electron or a Python backend.

## Expected Output After Implementations

When code or documentation changes are complete, report:

- Files changed.
- Purpose of the change.
- Tests run, or why tests were not run.
- Any behavior intentionally left unchanged.
- Any known follow-up work.

Keep summaries concise and factual.
