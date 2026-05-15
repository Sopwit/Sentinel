# Phase 4.2 Plan: Tool Invocation Planning Boundary

## Goal

Phase 4.2 adds a local-safe planning boundary for tool invocation intent without adding tool
execution.

The phase makes it possible for the agent runtime to describe a proposed tool call in structured
data while keeping all real actions non-operational.

## Scope

Implemented scope:

- Added value types for proposed tool invocation intent.
- Keep invocation data separate from `ToolDescriptor`.
- Keep descriptor registration in `IToolRegistry`.
- Allow `IAgentRuntime` to return zero or more proposed invocations through `plan()`.
- Keep `NullAgentRuntime` deterministic and local-only.
- Expose only generic, QML-safe planning metadata through controller/view-model status and summary
  surfaces.
- Add focused tests for deterministic proposed-invocation behavior and no-execution guarantees.

Value types:

- `ToolInvocationPlan`
- `PlannedToolInvocation`
- `ToolInvocationArgument`
- `ToolInvocationPlanStatus`

Candidate fields:

- tool id
- display title or summary
- arguments as explicit key/value intent data
- status
- user-visible reason

## Non-Goals

Phase 4.2 does not add:

- tool execution runtime
- shell/process execution
- filesystem or system mutation
- platform automation
- provider/network integrations
- API key handling
- plugin loading
- permission prompts that approve real execution

## Boundary Rules

- `ToolDescriptor` describes what a tool is.
- `ToolInvocationPlan` should describe a proposed call to a known tool.
- `IAgentRuntime` may return proposed plans, but no component may execute them.
- `ApplicationController` may expose summaries or counts, not mutable runtime internals.
- QML may display planning status, but must not build or execute invocation logic.
- Unknown tool ids should remain representable as rejected or unavailable planning state, not as
  executable fallback behavior.

## Acceptance Criteria

Phase 4.2 is complete:

- Invocation planning types exist in core as value-only data.
- Agent runtime can return proposed invocation plans.
- `NullAgentRuntime` produces deterministic local placeholder plans.
- Tests prove deterministic plans, empty-tool handling, unknown tool handling, metadata
  preservation, and controller/view-model status exposure.
- Documentation states that execution remains out of scope.
- `cmake --preset tests`, `cmake --build --preset tests`, and `ctest --preset tests` pass.

## Follow-Up Prompt

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
