# Phase 4 Checkpoint

Phase 4 is complete through the local-safe agent core, tool-system foundation, and AI
orchestration planning checkpoint.

## Completed Scope

- Provider and agent runtime boundaries remain separate.
- Tool metadata registration is isolated behind `IToolRegistry`.
- Tool invocation planning is value-only and metadata-only.
- Approval state is metadata-only and does not grant runtime permission.
- Sandbox capability evaluation is metadata-only and does not enforce an OS sandbox.
- Execution ownership exists only as a placeholder boundary through `IToolExecutor`.
- Runtime context/session owns in-memory metadata copied from the latest pipeline result.
- Activity logging is in-memory and metadata-only.
- Dashboard visibility is read-only and uses QML-safe view-model properties.
- Future AI orchestration direction is documented separately in `docs/AI_ORCHESTRATION_PLAN.md`.

## Known Limitations

- No real tool execution.
- No shell/process or subprocess launch.
- No filesystem or system mutation.
- No networking, API keys, or real provider integrations.
- No plugin loading.
- No real sandbox runtime or OS permission enforcement.
- No approval UX or execution controls.
- No activity log persistence, export, pruning, or durable audit trail.
- No model router, routing policy, model management UI, model downloads, or model execution.
- Chat history remains one local transcript.

## Phase 5 Readiness Criteria

- Keep all Phase 4 execution boundaries non-operational until a later explicit execution phase.
- Preserve provider/agent/tool/sandbox/execution separation during UI work.
- Keep QML behind `DesktopShellViewModel` and QML-safe models.
- Avoid moving business logic, persistence, or platform behavior into QML.
- Keep platform-specific behavior behind explicit service interfaces.
- Keep future `ModelRouter`, `RoutingPolicy`, provider capability profiles, and model-management UI
  separate from `IChatProvider`, `IAgentRuntime`, and tool execution.
- Maintain full test and formatting verification before broad UI changes.

## Explicitly Out Of Scope

- Starting real execution, approval, sandbox, provider, plugin, networking, or platform automation
  work from Phase 5 UI/UX changes.
- Adding dependencies without an explicit architecture decision.
- Treating placeholder execution results as successful real actions.
- Turning AI orchestration planning into provider/model execution without a later explicit phase.
