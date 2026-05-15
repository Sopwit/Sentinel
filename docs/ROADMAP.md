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

Completed through Phase 4.11 with metadata-only agent/tool boundaries and no real execution.

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

### Phase 4.9: Agent Pipeline UI Visibility

Completed. Added simple read-only desktop visibility for the metadata-only pipeline.

Delivered:

- Dashboard status section for latest pipeline status/summary.
- Runtime context status/summary and active planned tool id visibility.
- Activity count and latest activity summary visibility.
- Focused view-model tests for successful and blocked pipeline visibility.
- QML boundary checks that keep agent visibility properties simple and read-only.

Still out of scope:

- Real tool execution.
- Shell/process execution.
- Subprocess execution.
- Filesystem mutation.
- Execution or approval controls.
- Activity log persistence/export.
- Secret capture or API key handling.
- Provider/network integrations.
- Plugin loading.
- Real sandbox runtime or OS permission enforcement.

### Phase 4.10: Architecture Checkpoint and Cleanup

Completed. Closed Phase 4 with an architecture consistency checkpoint before Phase 5.

Delivered:

- Phase 4 completed-scope and limitation checkpoint documentation.
- Phase 5 readiness criteria.
- Focused reset/fallback tests for existing metadata-only behavior.
- Minor Dashboard label clarity.

Still out of scope:

- Real tool execution.
- Shell/process execution.
- Subprocess execution.
- Filesystem/system mutation.
- Networking/API keys.
- Provider integrations.
- Plugin loading.
- Execution or approval controls.
- Real sandbox runtime or OS permission enforcement.

### Phase 4.11: AI Orchestration Planning Checkpoint

Completed. Planned future AI routing architecture before Phase 5.

Delivered:

- `ModelRouter` and `RoutingPolicy` direction.
- Provider capability profile and task classification concepts.
- Local/cloud fallback, device-aware selection, user routing modes, and privacy-aware routing.
- Future model-management UI metadata scope.
- Clear separation from current Phase 4 runtime, providers, agent runtime, tool execution, and UI
  screens.

Still out of scope:

- Real provider integrations.
- Networking/API keys.
- Ollama/OpenAI/Anthropic integration.
- Model downloads or model execution.
- Plugin loading.
- Filesystem/system actions.
- Real tool execution.

## Phase 5: Advanced UI/UX & Motion System

Evolve the Qt Quick experience with responsive layouts, adaptive themes, assistant-like interaction, animated panels, dashboard cards, and reusable components.

### Phase 5.0: UI/UX Planning and Design System Foundation

Completed. Established design direction and shared QML presentation tokens.

Delivered:

- `docs/UI_UX_PLAN.md` for visual direction, design-system scope, and motion constraints.
- `SentinelTheme.qml` QML singleton for palette, spacing, radius, and typography tokens.
- Minimal QML token adoption while preserving current Dashboard, Chat, Memory, Settings, and
  runtime visibility behavior.

Still out of scope:

- Heavy animation or particle assistant visuals.
- Provider/model integrations or model management execution.
- Real tool execution, approval actions, sandbox runtime, plugin loading, networking, or
  filesystem/system actions.

### Phase 5.1: Motion and Interaction Foundation

Completed. Added lightweight interaction motion without advanced visuals.

Delivered:

- Motion duration/easing tokens in `SentinelTheme.qml`.
- Sidebar hover/focus polish.
- Tokenized command button hover/focus styling.
- Text-field focus-ring transitions.
- Lightweight page opacity transition hooks.
- Updated motion guidelines.

Still out of scope:

- Heavy animations, particle systems, assistant-face rendering, or custom rendering systems.
- Provider/model execution, networking/API keys, plugin loading, filesystem/system actions, real
  tool execution, approval actions, or sandbox runtime.

### Phase 5.2: Adaptive Layout and Responsive Shell Foundation

Completed. Added responsive layout behavior without changing runtime capabilities.

Delivered:

- Compact and wide breakpoint tokens in `SentinelTheme.qml`.
- Adaptive shell margins, sidebar width, header/status behavior, and page spacing.
- Dashboard/card/chat wrapping for narrower windows.
- Memory and settings forms that stack at compact widths.
- UI/UX guidance for practical Linux/KDE and macOS resizing behavior.

Still out of scope:

- Heavy animations, particle systems, assistant-face rendering, or custom rendering systems.
- Provider/model execution, networking/API keys, plugin loading, filesystem/system actions, real
  tool execution, approval actions, or sandbox runtime.

### Phase 5.3: Component Consistency and Visual QA Pass

Completed. Tightened presentation consistency and added manual visual QA guidance.

Delivered:

- Shared `SentinelTextField` and `InfoRow` QML components.
- Consistent input/button sizing and card padding tokens.
- Dashboard and settings read-only status rows aligned to the same component pattern.
- `docs/UI_QA_CHECKLIST.md` with compact, normal, wide, Fedora KDE, and macOS check points.

Still out of scope:

- Heavy animations, particle systems, assistant-face rendering, or custom rendering systems.
- Provider/model execution, networking/API keys, plugin loading, filesystem/system actions, real
  tool execution, approval actions, sandbox runtime, or automated visual driving.

Readiness criteria:

- Preserve Phase 4 no-execution boundaries.
- Treat AI orchestration and model-management screens as metadata-only planning until an explicit
  later implementation phase.
- Keep QML behind view-models and avoid business logic in presentation code.
- Treat agent/runtime visibility as read-only unless a later explicit phase changes it.
- Keep full tests and formatting passing through UI changes.

## Phase 6: Security / Sandbox / Permissions

Add real permission prompts, auditability, sandboxing strategy, secret handling, and safe local automation constraints.

## Phase 7: Packaging / Ecosystem / Extensions

Prepare packaging, update channels, plugin/extension lifecycle, platform-specific integration packages, and distribution workflows.
