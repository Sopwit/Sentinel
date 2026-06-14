# Security

Sentinel defaults to explicit user authority.

## Phase 51 Boundary

- No telemetry by default.
- No silent updates.
- No automatic downloads or installs.
- No hidden uploads.
- No hidden indexing.
- No hidden cloud activation.
- No autonomous product behavior.

Onboarding, About, Updates, Notifications, Accessibility, Exports, Recovery, Diagnostics, and
Brain Insights are foreground product surfaces. Manual update checks and download confirmations
record local state only until a later update phase explicitly implements networking and package
verification. Diagnostics export writes only to the app-controlled export directory after user
action.

## Phase 50C Boundary

- No autonomous execution.
- No hidden tools.
- No background approvals.
- No self-modifying tasks.
- No recursive task generation.
- No automatic retries without approval.
- No cloud activation.
- Single active controlled task only.

Controlled tasks require explicit user approval before start and explicit user action for each
visible step. Tool permissions are workspace-scoped metadata choices and do not grant filesystem,
clipboard, terminal, browser, calendar, email, notes, or download execution in this phase.

## Phase 50B Boundary

- No telemetry.
- No hidden cloud calls.
- No autonomous agents.
- No folder import or recursive filesystem scanning.
- No background document processing.
- No automatic embedding generation.
- No automatic knowledge-base activation.

File chat and Local RAG accept only explicit foreground user actions. Local RAG metadata is
workspace-scoped and stored in SQLite through Qt SQL.

## Execution Separation

Workspace, attachment, and retrieval metadata do not enable tool execution, subprocess execution,
filesystem mutation, cloud retrieval, or agent autonomy. Local Ollama foreground chat remains the
only real provider execution path from Phase 50A.
