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
Explain where chat history persistence should be added later.

Do not implement it.
Return only an architecture note with file references.
```

## Anti-Pattern Prompts

Avoid broad prompts:

- "Improve the architecture."
- "Refactor the app."
- "Add memory features."
- "Make it production ready."
- "Start Phase 3.2."

Avoid unclear scope:

- Prompts that allow source, QML, build, and docs changes at the same time.
- Prompts that mix implementation with speculative architecture.
- Prompts that ask for persistence changes without naming the storage boundary.

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
- No implicit dependencies.
- No QML business logic.
- No Phase 3.2 work until explicitly started.
