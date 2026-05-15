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

```text
Read AGENTS.md, docs/DECISIONS.md, and docs/ARCHITECTURE.md first.

Task:
Implement Phase 4.0 minimal agent runtime skeleton.

Scope:
core agent-runtime interfaces, deterministic local runtime, minimal controller/view-model status wiring, focused tests, and docs.

Do not:
Implement real tools, networking, provider integrations, plugin loading, or privileged automation.

After completion:
Run full tests plus clang-format dry-run and report remaining limitations.
```

```text
Read AGENTS.md, docs/PHASE_STATUS.md, and docs/DECISIONS.md first.

Task:
Implement Phase 4.1 tool descriptor and registry skeleton.

Scope:
Tool descriptor structs/enums, IToolRegistry, InMemoryToolRegistry, metadata-only runtime exposure, focused tests, and docs.

Do not:
Implement tool execution, shell commands, filesystem/system mutation, provider integrations, plugin loading, or privileged automation.

After completion:
Run build/tests and clang-format dry-run, then report limitations and out-of-scope items still enforced.
```

```text
Read AGENTS.md, docs/AI_CONTEXT.md, docs/PHASE_STATUS.md, docs/DECISIONS.md, and
docs/PHASE_4_2_PLAN.md first.

Task:
Audit or stabilize the completed Phase 4.2 tool invocation planning boundary.

Scope:
Core value types, IAgentRuntime planning shape, NullAgentRuntime deterministic planning behavior,
controller/view-model metadata status exposure, focused tests, and docs.

Do not:
Implement tool execution, shell commands, filesystem/system mutation, platform automation,
provider/network integrations, API key handling, plugin loading, or permission prompts that approve
real execution.

After completion:
Run build/tests and report the remaining no-execution limitations.
```

```text
Read AGENTS.md, docs/AI_CONTEXT.md, docs/PHASE_STATUS.md, docs/DECISIONS.md, and
docs/ARCHITECTURE.md first.

Task:
Audit or stabilize the completed Phase 4.3 approval and permission metadata skeleton.

Scope:
Approval metadata value types, IApprovalPolicy, StaticApprovalPolicy, controller/view-model
approval status exposure, focused tests, and docs.

Do not:
Implement tool execution, shell/process launch, filesystem mutation, networking/API keys,
provider integrations, plugin loading, sandbox runtime, or permission prompts that approve real
execution.

After completion:
Run build/tests and clang-format dry-run, then report the remaining no-execution limitations.
```

```text
Read AGENTS.md, docs/AI_CONTEXT.md, docs/PHASE_STATUS.md, docs/DECISIONS.md, and
docs/ARCHITECTURE.md first.

Task:
Audit or stabilize the completed Phase 4.5 execution boundary skeleton.

Scope:
Tool execution request/result/status value types, IToolExecutor, NullToolExecutor,
controller/view-model execution status exposure, focused tests, and docs.

Do not:
Implement real tool execution, shell/process launch, subprocess execution, filesystem mutation,
networking/API keys, provider integrations, plugin loading, real sandbox runtime, OS permission
enforcement, or privileged automation.

After completion:
Run build/tests and clang-format dry-run, then report the remaining placeholder-only execution
limitations.
```

```text
Read AGENTS.md, docs/AI_CONTEXT.md, docs/PHASE_STATUS.md, docs/DECISIONS.md, and
docs/ARCHITECTURE.md first.

Task:
Audit or stabilize the completed Phase 4.6 agent runtime pipeline result model.

Scope:
AgentPipelineResult value state, ApplicationController pipeline routing, desktop view-model generic
status exposure, focused tests, and docs.

Do not:
Implement real tool execution, shell/process launch, subprocess execution, filesystem mutation,
networking/API keys, provider integrations, plugin loading, real sandbox runtime, OS permission
enforcement, or privileged automation.

After completion:
Run build/tests and clang-format dry-run, then report the remaining placeholder-only execution
limitations.
```

```text
Read AGENTS.md, docs/AI_CONTEXT.md, docs/PHASE_STATUS.md, docs/DECISIONS.md, and
docs/ARCHITECTURE.md first.

Task:
Audit or stabilize the completed Phase 4.7 runtime context and tool session skeleton.

Scope:
AgentRuntimeContext/RuntimeSession value state, ApplicationController runtime context ownership,
desktop view-model read-only exposure, focused tests, and docs.

Do not:
Implement real tool execution, shell/process launch, subprocess execution, filesystem mutation,
networking/API keys, provider integrations, plugin loading, runtime context persistence, real
sandbox runtime, OS permission enforcement, or privileged automation.

After completion:
Run build/tests and clang-format dry-run, then report the remaining placeholder-only execution
limitations.
```

```text
Read AGENTS.md, docs/AI_CONTEXT.md, docs/PHASE_STATUS.md, docs/DECISIONS.md, and
docs/ARCHITECTURE.md first.

Task:
Audit or stabilize the completed Phase 4.8 agent activity log and audit trail skeleton.

Scope:
AgentActivityLog value state, ApplicationController metadata event logging, desktop view-model
read-only exposure, focused tests, and docs.

Do not:
Implement real tool execution, shell/process launch, subprocess execution, filesystem mutation,
networking/API keys, provider integrations, plugin loading, activity log persistence/export, real
sandbox runtime, OS permission enforcement, or privileged automation.

After completion:
Run build/tests and clang-format dry-run, then report the remaining in-memory metadata-only audit
limitations.
```

```text
Read AGENTS.md, docs/AI_CONTEXT.md, docs/PHASE_STATUS.md, docs/DECISIONS.md, and
docs/ARCHITECTURE.md first.

Task:
Audit or stabilize the completed Phase 4.9 agent pipeline UI visibility layer.

Scope:
Dashboard read-only pipeline/runtime/activity visibility, DesktopShellViewModel QML-safe property
exposure, focused tests, and docs.

Do not:
Implement real tool execution, shell/process launch, subprocess execution, filesystem mutation,
networking/API keys, provider integrations, plugin loading, execution controls, approval controls,
activity log persistence/export, real sandbox runtime, OS permission enforcement, or privileged
automation.

After completion:
Run build/tests and clang-format dry-run, then report the remaining read-only/no-execution
limitations.
```

```text
Read AGENTS.md, docs/AI_CONTEXT.md, docs/PHASE_STATUS.md, docs/DECISIONS.md,
docs/PROMPT_GUIDE.md, docs/ROADMAP.md, docs/ARCHITECTURE.md, and
docs/PHASE_4_CHECKPOINT.md first.

Task:
Audit or stabilize the completed Phase 4.10 architecture checkpoint.

Scope:
Phase 4 architecture consistency, checkpoint docs, focused tests for existing metadata-only
behavior, and minor wording cleanup.

Do not:
Add product features, real tool execution, shell/process launch, subprocess execution, filesystem
mutation, networking/API keys, provider integrations, plugin loading, execution controls, approval
controls, activity log persistence/export, real sandbox runtime, OS permission enforcement,
privileged automation, or Phase 5 UI/UX implementation.

After completion:
Run build/tests and clang-format dry-run, then report Phase 5 readiness and remaining limitations.
```

```text
Read AGENTS.md, docs/AI_CONTEXT.md, docs/PHASE_STATUS.md, docs/DECISIONS.md, and
docs/ARCHITECTURE.md first.

Task:
Audit or stabilize the completed Phase 4.4 sandbox and capability boundary skeleton.

Scope:
Sandbox/capability metadata value types, ISandboxPolicy, StaticSandboxPolicy,
controller/view-model sandbox status exposure, focused tests, and docs.

Do not:
Implement tool execution, shell/process launch, filesystem mutation, networking/API keys,
provider integrations, plugin loading, real sandbox runtime, OS permission enforcement, or
privileged automation.

After completion:
Run build/tests and clang-format dry-run, then report the remaining no-execution limitations.
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
