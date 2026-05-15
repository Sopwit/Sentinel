# Prompt Guide

## Recommended Format

Use short, bounded prompts:

```text
Read AGENTS.md and docs/AI_CONTEXT.md first.

Task:
<specific change>

Scope:
<allowed files or modules>

Do not:
<explicit exclusions>

After completion:
<tests or summary format>
```

## Example Prompts

```text
Read AGENTS.md and docs/AI_CONTEXT.md first.

Task:
Update documentation for the current phase status.

Scope:
docs/PHASE_STATUS.md and docs/DECISIONS.md only.

Do not modify source, tests, CMake, or QML.
```

```text
Read AGENTS.md first.

Task:
Add focused tests for JsonSettingsStore handling of invalid JSON.

Scope:
tests/core and core settings code only if required.

Do not change memory persistence or QML.
Run the relevant test target after changes.
```

```text
Read AGENTS.md and docs/DECISIONS.md first.

Task:
Update platform architecture notes for Phase 3.4.

Scope:
docs only.

Do not modify runtime code, QML implementation, SQLite code, dependencies, or Phase 4 behavior.
```

```text
Read AGENTS.md, docs/AI_CONTEXT.md, and docs/PHASE_STATUS.md first.

Task:
Run a Phase 3.5 architecture audit checkpoint and fix only small safe issues.

Scope:
core, apps/sentinel-desktop, ui/qml, tests, and docs.

Do not:
Start Phase 4 implementation, add networking/API keys, provider integrations, plugin loading, or privileged automation.

After completion:
Run build/tests and report readiness risks plus a Phase 4 readiness verdict.
```

## Anti-Pattern Prompts

Avoid broad prompts:

- "Improve the architecture."
- "Refactor the app."
- "Add memory features."
- "Make it production ready."
- "Start the next phase."
- "Make it Linux-only."
- "Add animations everywhere."

Avoid unclear scope:

- Prompts that allow source, QML, build, and docs changes at the same time.
- Prompts that mix implementation with speculative architecture.
- Prompts that ask for persistence changes without naming the storage boundary.
- Prompts that add platform-specific logic without naming the abstraction boundary.

## Keeping Prompts Small

- Reference `AGENTS.md` instead of repeating stack and rules.
- Reference `docs/AI_CONTEXT.md` instead of restating project vision.
- Reference `docs/PHASE_STATUS.md` instead of explaining completed phases.
- Reference `docs/DECISIONS.md` instead of repeating architecture decisions.
- State the active task and exclusions clearly.
- Name allowed files or modules when possible.

## Control Rules

- One phase at a time.
- One storage boundary at a time.
- One platform boundary at a time.
- No implicit dependencies.
- No QML business logic.
- No Linux-only core assumptions.
- No next-phase work until explicitly started.
