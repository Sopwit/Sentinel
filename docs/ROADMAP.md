# Sentinel Roadmap

Sentinel is a cross-platform Qt/QML personal AI assistant desktop app, optimized first for Linux/Fedora KDE Plasma.

## Phase 1: Foundation

Create the C++20/Qt project structure, core interfaces, local echo provider, runtime memory, mode handling, and documentation base.

## Phase 2: UI Shell Foundation

Build the QML desktop shell, dashboard, chat panel, memory/settings pages, view models, and local-only UX foundation.

## Phase 3: Persistence + Architecture Stabilization

Add settings persistence, SQLite key-value memory, dedicated chat history persistence, diagnostics, lifecycle controls, and AI context docs.

## Phase 3.4: Cross-platform Architecture Readiness

Completed. Prepared architecture for Linux, Windows, and macOS without implementing platform integrations yet.

Focus:

- Portable core boundaries.
- Linux/Fedora KDE optimized path.
- Future platform service interfaces.
- Controlled dependency growth.
- No Phase 4 implementation.

Delivered in this phase:

- Platform boundary interfaces (`IPathProvider`, `IPlatformService`, `INotificationService`,
  `ISystemIntegrationService`).
- Default Qt `QStandardPaths` path ownership through `StandardPathProvider`.
- Local storage maintenance controls for memory/chat with confirmation UX.
- Explicit separation preserved across settings, memory, and chat history.

## Phase 3.5: Pre-agent Architecture Audit and Release Checkpoint

Completed. Stabilization checkpoint before starting Phase 4 implementation.

Focus:

- Architecture consistency audit across core, view-model, persistence, platform boundaries, and QML exposure safety.
- Small safe fixes only (docs, naming, status wording, minor test gaps).
- Verification gate for tests and formatting.
- Explicit no-Phase-4-runtime rule.

Readiness criteria for Phase 4:

- Boundaries and persistence separation remain intact.
- QML exposure remains generic and safe.
- Verification commands and format checks pass.
- Known limitations are documented and unchanged.

Candidate interfaces:

- `IPlatformService`
- `IPathProvider`
- `INotificationService`
- `ISystemIntegrationService`

## Phase 4: Agent Core & Tool System

Started with Phase 4.0 minimal runtime boundary work.

### Phase 4.0: Agent Core Planning and Minimal Runtime Skeleton

Completed.

Delivered:

- `IAgentRuntime` boundary and request/response/status abstractions.
- Deterministic local `NullAgentRuntime` with no tool execution.
- Minimal controller/view-model agent status/placeholder request wiring.

Still out of scope:

- Real tool execution.
- Networking/API keys.
- Real provider integration.
- Plugin loading.
- Privileged or OS-level automation.

### Phase 4.1: Tool Descriptor and Registry Skeleton

Completed.

Delivered:

- Tool metadata abstractions (`ToolDescriptor`, parameters, risk level, execution mode).
- Tool metadata registry boundary (`IToolRegistry`, `InMemoryToolRegistry`).
- Deterministic register/list/find semantics and duplicate-id handling.
- Metadata-only runtime exposure of available tools.

Still out of scope:

- Tool execution runtime.
- Filesystem/system mutation tools.
- Shell command execution.
- Platform automation.
- Networking/API keys and provider integrations.

### Phase 4.2: Tool Invocation Planning Boundary

Completed. Added structured, value-only proposed tool invocation plans without adding execution.

Delivered:

- Proposed invocation value types (`ToolInvocationPlan`, `PlannedToolInvocation`,
  `ToolInvocationArgument`, `ToolInvocationPlanStatus`).
- Agent runtime support for metadata-only proposed plans.
- Deterministic local placeholder planning in `NullAgentRuntime`.
- Generic planning status/summary exposure through controller/view-model.
- Tests that preserve the no-execution boundary.

Still out of scope:

- Tool execution runtime.
- Shell/process execution.
- Filesystem/system mutation.
- Platform automation.
- Provider/network integrations.
- API key handling.
- Plugin loading.
- Sandbox runtime.

### Phase 4.3: Approval and Permission Metadata Skeleton

Completed. Added approval policy metadata for planned tool invocations without execution or
sandboxing.

Delivered:

- Approval metadata value types (`ApprovalStatus`, `ApprovalDecision`, `PermissionDescriptor`,
  `ToolApprovalRequest`).
- Approval policy boundary (`IApprovalPolicy`) and deterministic `StaticApprovalPolicy`.
- Metadata-only evaluation of planned tool invocations.
- Generic approval status/summary exposure through controller/view-model.
- Tests for safe plans, risky approval requirements, approved/denied states, empty plans, and
  deterministic policy behavior.

Still out of scope:

- Tool execution runtime.
- Shell/process execution.
- Filesystem/system mutation.
- Platform automation.
- Provider/network integrations.
- API key handling.
- Plugin loading.
- Sandbox runtime.

### Phase 4.4: Sandbox and Capability Boundary Skeleton

Completed. Added sandbox capability policy metadata for planned and approved invocations without
execution or real sandbox enforcement.

Delivered:

- Capability metadata abstractions (`CapabilityDescriptor`, sandbox status/result records).
- Sandbox policy boundary (`ISandboxPolicy`) and deterministic `StaticSandboxPolicy`.
- Metadata-only evaluation of planned capabilities against allowed capability ids.
- Generic sandbox status/summary exposure through controller/view-model.
- Tests for metadata-only allowance, unknown capability denial, empty plans,
  approved-but-not-capable blocking, and deterministic policy behavior.

Still out of scope:

- Tool execution runtime.
- Shell/process execution.
- Filesystem/system mutation.
- Platform automation.
- Provider/network integrations.
- API key handling.
- Plugin loading.
- Real sandbox runtime or OS permission enforcement.

### Phase 4.5: Execution Boundary Skeleton

Completed. Added a placeholder execution ownership boundary without real tool execution.

Delivered:

- Execution abstractions (`ToolExecutionRequest`, `ToolExecutionResult`,
  `ToolExecutionStatus`).
- Execution interface boundary (`IToolExecutor`) and deterministic `NullToolExecutor`.
- Controller/view-model placeholder execution status and summary exposure.
- Tests for placeholder success, blocked paths, empty plans, unknown tools, deterministic results,
  and status exposure.

Still out of scope:

- Real tool execution.
- Shell/process execution.
- Subprocess execution.
- Filesystem/system mutation.
- Platform automation.
- Provider/network integrations.
- API key handling.
- Plugin loading.
- Real sandbox runtime or OS permission enforcement.

### Phase 4.6: Agent Runtime Pipeline Stabilization

Completed. Stabilized the metadata-only controller pipeline after the placeholder execution
boundary.

Delivered:

- Aggregate `AgentPipelineResult` value model for planning, approval, sandbox, execution, and
  summary state.
- Centralized safe summary/status exposure for controller and desktop view-model use.
- Clear controller route through:
  registry -> planning -> approval -> sandbox capability metadata -> placeholder execution.
- Focused tests for success, approval-blocked, sandbox-blocked, empty-plan, unknown-tool, and
  deterministic status/summary output.

Still out of scope:

- Real tool execution.
- Shell/process execution.
- Subprocess execution.
- Filesystem/system mutation.
- Platform automation.
- Provider/network integrations.
- API key handling.
- Plugin loading.
- Real sandbox runtime or OS permission enforcement.

### Phase 4.7: Runtime Context and Tool Session Skeleton

Completed. Added deterministic local runtime context ownership without adding execution.

Delivered:

- `AgentRuntimeContext` value state for latest pipeline metadata, active planned tool ids, status,
  summary, session id, and revision.
- `RuntimeSession` as the in-memory owner for runtime context metadata.
- Controller/view-model read-only runtime context status, summary, session id, and active planned
  tool id exposure.
- Tests for deterministic context creation, pipeline attachment, planned tool ordering, reset
  behavior, and controller/view-model exposure.

Still out of scope:

- Real tool execution.
- Shell/process execution.
- Subprocess execution.
- Filesystem/system mutation.
- Platform automation.
- Provider/network integrations.
- API key handling.
- Plugin loading.
- Runtime context persistence.
- Real sandbox runtime or OS permission enforcement.

### Phase 4.8: Agent Activity Log and Audit Trail Skeleton

Completed. Added in-memory metadata-only activity logging for the agent pipeline.

Delivered:

- `AgentActivityEntry`, `AgentActivityType`, and `AgentActivityStatus` value metadata.
- `AgentActivityLog` with deterministic in-memory sequence ordering and clear behavior.
- Controller logging for request received, plan created, approval evaluated, sandbox evaluated,
  placeholder execution evaluated, and pipeline completed/blocked.
- Desktop view-model read-only activity count and latest summary exposure.
- Tests for deterministic ordering, clear behavior, successful and blocked pipeline logging, and
  controller/view-model exposure.

Still out of scope:

- Real tool execution.
- Shell/process execution.
- Subprocess execution.
- Filesystem/system mutation.
- Activity log persistence/export.
- Secret capture or API key handling.
- Provider/network integrations.
- Plugin loading.
- Real sandbox runtime or OS permission enforcement.

## Phase 5: Advanced UI/UX & Motion System

Evolve the Qt Quick experience with responsive layouts, adaptive themes, assistant-like interaction, animated panels, dashboard cards, and reusable components.

## Phase 6: Security / Sandbox / Permissions

Add real permission prompts, auditability, sandboxing strategy, secret handling, and safe local automation constraints.

## Phase 7: Packaging / Ecosystem / Extensions

Prepare packaging, update channels, plugin/extension lifecycle, platform-specific integration packages, and distribution workflows.
