# Controlled Agents

Phase 50C introduces controlled agent tasks: visible, user-approved workflows for multi-step work.
They are not autonomous agents.

## Planning Lifecycle

1. The user asks for a task plan.
2. Sentinel creates a local task record with title, description, workspace, provider, model, steps,
   approval history, and result summary.
3. The plan enters Pending Approval.
4. The user can edit, remove, or reorder steps before approval.

Planning does not execute tools, call cloud services, read files, or start background work.

## Approval Lifecycle

Before running, the UI exposes Task Approval choices:

- Execute all steps.
- Execute selected steps.
- Cancel.

Approval choices are recorded locally. Deny and cancel are terminal user-controlled outcomes.
Approval does not imply background execution; the user still starts and advances visible work.

## Execution Lifecycle

Only one task can run at a time. Execution is foreground-only and advances one visible step per
user action. Each step shows current step, remaining steps, progress, and actions for cancel, skip,
and retry.

Retries are never automatic. Recursive task generation is blocked.

## Permission Model

Tool categories are Files, Clipboard, Terminal, Browser, Calendar, Email, Notes, and Downloads.
Choices are:

- Allow Once.
- Allow For Workspace.
- Deny.

Choices are workspace-scoped metadata in Phase 50C. They do not grant real filesystem, clipboard,
terminal, browser, calendar, email, notes, or download execution.

## Explainability

Each step records:

- Step executed by: selected model metadata.
- Reason: why the step exists.
- Resources used: visible attached/local metadata.
- Tools requested: declared tool categories.
- Outcome: visible result or refusal/skip/cancel summary.

## Privacy Guarantees

- No telemetry.
- No hidden cloud calls.
- No hidden tool use.
- No background task execution.
- No background approvals.
- No autonomous agents.
- No self-modifying tasks.
- No automatic retries.
- No cloud activation.
