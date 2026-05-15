# AI Orchestration Plan

Phase 4.11 records future AI routing direction only. It does not implement providers, models,
networking, model execution, downloads, or tool execution.

## Future Components

- `ModelRouter`: future coordinator that chooses a model/provider target for a request.
- `RoutingPolicy`: future policy object that applies user mode, task type, privacy, provider
  capability, and device constraints.
- Provider capability profiles: metadata describing local/cloud availability, latency class,
  context limits, modality support, privacy posture, and cost class.
- Task classification: metadata-only classification for chat, summarization, coding, planning,
  long-context, tool-planning, and sensitive-data tasks.

These concepts should remain separate from `IChatProvider`, `IAgentRuntime`, tool execution, and
UI model-management screens. Providers execute a chosen request later; the router decides where a
request should go.

## Routing Inputs

- User routing mode: Auto, Fast, Balanced, Quality, Local Only, Cloud Allowed, or Battery Saver.
- Privacy posture: sensitive data should prefer local-only routing.
- Cloud permission: cloud use must require explicit user permission.
- Device state: battery, thermal/performance class, available RAM, disk availability, and local
  model readiness.
- Provider/model capability metadata: local/cloud badge, installed/downloadable state, required
  RAM/disk, recommended use cases, and quality/latency class.

## Routing Direction

- Auto: choose the best allowed target from task, privacy, and device metadata.
- Fast: prefer low-latency local or lightweight cloud targets, if allowed.
- Balanced: prefer reliable general-purpose targets within privacy and device constraints.
- Quality: prefer highest quality allowed target, including cloud only when permitted.
- Local Only: never route to cloud providers.
- Cloud Allowed: cloud targets may be considered after explicit permission and privacy checks.
- Battery Saver: prefer lightweight local targets or defer heavy local model use.

Fallback should prefer local when data is sensitive, cloud is not allowed, or the device is capable.
Cloud fallback is future work and must remain permission-gated.

## Future Model Management UI

Later UI may show:

- installed models
- downloadable models
- recommended models
- RAM and disk requirements
- local/cloud badges
- readiness and compatibility status

That UI should consume safe metadata from a future model-management boundary. It should not own
routing logic, provider credentials, downloads, or execution.

## Current Separation

Current Phase 4 runtime remains unchanged:

- `IChatProvider` is still the chat provider boundary.
- `IAgentRuntime` is still the metadata-only agent orchestration boundary.
- Tool planning, approval, sandbox, and execution boundaries remain non-operational.
- `NullAgentRuntime` and `NullToolExecutor` still perform no real AI/model/tool execution.

`ModelRouter`, `RoutingPolicy`, provider capability profiles, model installation state, and model
management UI are future architecture concepts only.
