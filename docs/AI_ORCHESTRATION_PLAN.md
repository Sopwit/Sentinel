# AI Orchestration Plan

Phase 4.11 recorded future AI routing direction only. Phase 6.0 began implementing that direction
as metadata-only architecture. Phase 6.1 persists the user routing mode preference. Phase 6.2 adds
a provider/model catalog metadata boundary. Phase 6.3 adds capability graph and task planner
metadata. Phase 6.4 adds static agent registry metadata. Phase 6.5 adds static memory taxonomy
metadata. Phase 6.6 adds a deterministic orchestration snapshot read model. These phases do not
implement providers, networking, model execution, downloads, API keys, autonomous agents, semantic
memory execution, or tool execution. Phase 6.7 adds metadata-only diagnostics and readiness
reporting over those values without probing providers, models, filesystems, networks, or system
services. Phase 6.8 adds a metadata-only conversation/session context layer over current routing,
agent, memory, and snapshot summaries without adding streaming, execution, provider calls, or
background behavior. Phase 6.9 adds a deterministic metadata-only conversation state graph that
records high-level interaction transitions without executing providers, models, tools, approvals,
or runtime work. Phase 6.10 checkpoints the completed Phase 6 metadata foundation before any Phase
7 local runtime boundary work begins. Phase 7.0 adds `ILocalRuntime` as a metadata-only local
runtime boundary that refuses execution. Phase 7.1 adds metadata-only local runtime session
ownership/lifecycle state without allocating models or launching processes.

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
- Memory catalog: static descriptors for future memory categories, retention/privacy metadata,
  recall hints, associations, and task affinity labels.
- Orchestration snapshot: read-only aggregation of current routing, task, provider, agent, memory,
  runtime, and activity metadata for workspace visibility.
- Orchestration diagnostics: deterministic readiness checks over existing metadata for routing,
  catalog availability, task planner availability, snapshot health, local-only privacy posture,
  cloud provider unavailability, and disabled execution capability.
- Conversation session context: high-level interaction/session metadata that summarizes current
  routing mode, preferred agent, memory affinity, attention state, and latest orchestration snapshot
  without storing chat messages or executing runtime work.
- Conversation state graph: deterministic state-transition metadata for the current high-level
  interaction flow. It can accept or reject transitions, but it does not trigger provider/model
  execution, tools, approval prompts, streaming, or autonomous work.
- Local runtime boundary: future local inference/runtime ownership surface. The current
  implementation reports metadata and refuses execution without probing local runtimes.
- Local runtime sessions: future local runtime ownership/session state. The current implementation
  reports deterministic placeholder lifecycle, allocation, and reservation metadata only.

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

Current Phase 7.1 runtime remains metadata-only:

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
- `IMemoryCatalog` and `StaticMemoryCatalog` expose static memory taxonomy descriptors only.
  Planner-selected memory affinity is a label for future recall planning, not semantic search or
  memory mutation.
- `IMemoryStore` and `SQLiteMemoryStore` remain the key-value memory persistence boundary and are
  not replaced by the taxonomy catalog.
- `OrchestrationSnapshot` and `WorkspaceStateSummary` aggregate current metadata into a read model
  for UI visibility. They do not schedule work, execute plans, call providers, refresh in the
  background, search memory, or mutate state.
- `StaticOrchestrationDiagnostics` generates `OrchestrationReadinessReport` values from the
  current snapshot and provider catalog metadata. It only checks already-owned metadata and does not
  probe providers/models, scan the filesystem, call networks, execute tools, load plugins, start
  background workers, or run external processes.
- `ConversationSession` owns higher-level interaction/session metadata only. It is separate from
  `ChatSession` transcript ownership and from Phase 4 `RuntimeSession` agent pipeline metadata.
  It does not persist chat history, stream tokens, call providers/models, search memory, execute
  tools, load plugins, scan filesystems, call networks, or start background workers.
- `ConversationStateGraph` owns deterministic state-transition metadata only. It is separate from
  `ConversationSession`, `ChatSession`, and Phase 4 `RuntimeSession`; accepted transitions are
  status summaries, not execution triggers.
- `ILocalRuntime` owns future local runtime metadata only. `NullLocalRuntime` reports status,
  health, capabilities, and a refusal response; it does not call local providers, execute models,
  launch processes, scan filesystems, stream output, or execute tools.
- `ILocalRuntimeSessionManager` owns future local runtime session metadata only.
  `NullLocalRuntimeSessionManager` reports one deterministic placeholder reserved session; it does
  not allocate models, launch processes, scan filesystems, stream output, call providers, execute
  tools, or load plugins.
- `AppSettings` persists the routing mode through `JsonSettingsStore`; it does not store provider
  credentials or API keys.
- Tool planning, approval, sandbox, and execution boundaries remain non-operational.
- `NullAgentRuntime` and `NullToolExecutor` still perform no real AI/model/tool execution.

Provider integrations, cloud routing, credentials, model downloads, model execution, autonomous
agent runtime, semantic/vector memory, actionable model-management UI, and routing policy
automation remain future work.

## Phase 7 Direction

Phase 7.0 starts with a local runtime boundary skeleton, not full provider/model execution. Any
future local runtime implementation must be explicitly scoped, interface-owned, deterministic in
tests, and still separate from cloud provider integration, credentials, downloads, streaming, tool
execution, plugins, vector memory, and autonomous behavior unless a later phase approves those
capabilities.
