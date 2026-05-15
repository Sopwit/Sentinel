# AI Orchestration Plan

Phase 4.11 recorded future AI routing direction only. Phase 6.0 began implementing that direction
as metadata-only architecture. Phase 6.1 persists the user routing mode preference. Phase 6.2 adds
a provider/model catalog metadata boundary. Phase 6.3 adds capability graph and task planner
metadata. Phase 6.4 adds static agent registry metadata. These phases do not implement providers,
networking, model execution, downloads, API keys, autonomous agents, or tool execution.

## Future Components

- `ModelRouter`: metadata coordinator that chooses a future model/provider target for a request.
- `RoutingPolicy`: future policy object that applies user mode, task type, privacy, provider
  capability, and device constraints.
- Provider capability profiles: metadata describing local/cloud availability, latency class,
  context limits, modality support, privacy posture, and cost class.
- Task classification: metadata-only classification for chat, summarization, coding, planning,
  long-context, tool-planning, and sensitive-data tasks.
- Task planner: metadata planner that can assemble high-level task plan steps from task type,
  routing mode, provider/model catalog availability, privacy posture, and local/cloud suitability.
- Agent registry: static descriptors for future orchestration roles, task affinities, privacy
  posture, and local/cloud metadata affinity.

These concepts remain separate from `IChatProvider`, `IAgentRuntime`, tool execution, and UI
model-management screens. Providers may execute a chosen request in a later phase; the router only
decides where a request should go, the task planner only describes high-level metadata steps, and
the agent registry only labels future role/capability metadata.

## Routing Inputs

- User routing mode: Auto, Fast, Balanced, Quality, Local Only, Cloud Allowed, or Battery Saver.
- The persisted default is Local Only to keep the desktop alpha privacy-safe.
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

Current Phase 6.4 runtime remains metadata-only:

- `IChatProvider` is still the chat provider boundary.
- `IAgentRuntime` is still the metadata-only agent orchestration boundary.
- `IModelRouter` only chooses provider/model descriptors for future execution.
- `StaticModelRouter` returns a deterministic placeholder route based on the persisted routing mode.
- `IProviderCatalog` and `StaticProviderCatalog` describe provider/model availability metadata only.
  Cloud placeholders are visible as not configured and cannot be selected by the current static
  router.
- `ITaskPlanner` and `StaticTaskPlanner` create high-level task plan metadata only. They do not
  call providers, execute models, execute tools, download models, load plugins, access the network,
  or touch the filesystem/system.
- `IAgentRegistry` and `StaticAgentRegistry` expose static agent descriptors only. Planner-selected
  agents are labels for metadata visibility, not runtime workers.
- `AppSettings` persists the routing mode through `JsonSettingsStore`; it does not store provider
  credentials or API keys.
- Tool planning, approval, sandbox, and execution boundaries remain non-operational.
- `NullAgentRuntime` and `NullToolExecutor` still perform no real AI/model/tool execution.

Provider integrations, cloud routing, credentials, model downloads, model execution, autonomous
agent runtime, actionable model-management UI, and routing policy automation remain future work.
