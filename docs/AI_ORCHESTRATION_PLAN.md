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
ownership/lifecycle state without allocating models or launching processes. Phase 7.2 adds
metadata-only runtime capability negotiation descriptors without activating capabilities or
executing runtime work. Phase 7.3 adds metadata-only runtime permission policy descriptors with a
deterministic default-deny execution posture. Phase 7.4 adds a metadata-only runtime request
pipeline that emits ordered traces and blocked execution summaries only. Phase 7.5 adds
metadata-only runtime safety policy reporting for local-only and no-execution posture. Phase 8.0
through Phase 8.2 add metadata-only execution lifecycle/session coordination with requested,
validating, permission-check, safety-check, coordination, ready-placeholder, and blocked states
while keeping execution disabled. Phase 8.3 through Phase 8.5 add metadata-only local runtime
adapter, provider bridge, and pre-integration readiness boundaries without calling, launching,
discovering, or executing models. Phase 9.0 through Phase 9.2 add a controlled Ollama-local
health/discovery boundary: loopback-only endpoint metadata, safe `/api/version` health checks, and
optional `/api/tags` installed-model metadata. Chat inference, prompt execution, streaming,
downloads, subprocess launch, cloud calls, tools, plugins, and filesystem/system actions remain
disabled.

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
- Runtime capability negotiation: future runtime capability vocabulary and negotiation posture.
  The current implementation reports deterministic capability descriptors, disabled/unavailable
  capability summaries, local-only enforcement, and privacy-safe metadata only.
- Runtime permission policy: future execution permission boundary. The current implementation
  reports deterministic permission request/decision metadata and denies execution requests by
  default.
- Runtime request pipeline: future runtime request ownership path. The current implementation
  reports deterministic stage/status/trace metadata and keeps execution blocked.
- Runtime safety policy: future runtime safety/sandbox posture boundary. The current implementation
  reports deterministic local-only and no-execution safety rules and summaries.
- Execution lifecycle/session coordination: future execution lifecycle vocabulary and ownership
  read model. The current implementation reports deterministic ordered lifecycle traces, rejects
  invalid transitions, exposes read-only coordination snapshots, and keeps execution
  blocked/non-executable.
- Local runtime adapter contract: future adapter boundary for Ollama/local runtime integration. The
  current implementation exposes placeholder metadata only and does not configure endpoints,
  discover models, launch processes, or execute inference.
- Provider runtime bridge: future bridge between provider routing and runtime adapters. The current
  implementation reports not connected/not executable and is not an `IChatProvider`.
- Runtime integration readiness: deterministic pre-integration report describing missing adapter,
  endpoint, discovery, bridge, and execution prerequisites without probing providers, runtimes,
  filesystems, networks, or system services.

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

Current Phase 9.0-9.2 runtime allows only local Ollama health/discovery metadata:

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
- `IRuntimeCapabilityRegistry` owns future runtime capability negotiation metadata only.
  `StaticRuntimeCapabilityRegistry` reports deterministic capability descriptors and enables only
  local-only/privacy-safe safety posture metadata; it does not activate capabilities, execute
  models, call providers, stream output, access filesystems, launch processes, execute tools, load
  plugins, or access networks.
- `IRuntimePermissionPolicy` owns future runtime permission policy metadata only.
  `StaticRuntimePermissionPolicy` denies execution-level runtime permissions by default in
  metadata-only mode.
- `IRuntimeSafetyPolicy` owns future runtime safety posture metadata only.
  `StaticRuntimeSafetyPolicy` reports deterministic local-only/no-execution policy and rules.
- `IRuntimePipeline` owns future runtime request pipeline metadata only.
  `StaticRuntimePipeline` emits deterministic request/permission/safety/execution-boundary traces
  and returns blocked metadata summaries without performing actions.
- `IExecutionLifecycle` owns future execution lifecycle metadata only.
  `StaticExecutionLifecycle` emits deterministic requested, validating, permission-check,
  safety-check, coordination, ready-placeholder, and blocked traces. Ready-placeholder is
  descriptive only; execution remains blocked and non-executable. `ExecutionCoordinator` exposes a
  read-only session coordination snapshot and does not schedule work.
- `ILocalRuntimeAdapter` owns future local runtime adapter contract metadata only.
  `StaticLocalRuntimeAdapter` describes an Ollama placeholder adapter without endpoint
  configuration, model discovery, runtime process ownership, or inference execution.
- `IProviderRuntimeBridge` owns future provider-to-runtime bridge metadata only.
  `StaticProviderRuntimeBridge` reports not connected and not executable; it does not implement
  `IChatProvider`, call providers, call runtimes, or stream responses.
- `StaticRuntimeIntegrationReadiness` reports ordered pre-integration checks from existing metadata
  only. It does not probe Ollama, scan filesystems, read API keys, access networks, discover
  models, launch processes, or execute requests.
- `IOllamaRuntimeClient` owns the Ollama local health/discovery boundary. `NullOllamaRuntimeClient`
  is deterministic unavailable behavior, and `OllamaHttpRuntimeClient` is injectable and limited to
  loopback read-only `/api/version` and `/api/tags` calls. It is not `IChatProvider`, execution
  lifecycle, provider bridge execution, model router execution, agent runtime, tool executor, or
  plugin runtime.
- `AppSettings` persists the routing mode and normalized Ollama endpoint through
  `JsonSettingsStore`; it does not store provider credentials or API keys.
- Tool planning, approval, sandbox, and execution boundaries remain non-operational.
- `NullAgentRuntime` and `NullToolExecutor` still perform no real AI/model/tool execution.

Provider inference, cloud routing, credentials, model downloads, model execution, autonomous agent
runtime, semantic/vector memory, actionable model-management UI, and routing policy automation
remain future work.

## Phase 9 Direction

Phase 7.0 starts with a local runtime boundary skeleton, not full provider/model execution. Phase
7.2 adds capability negotiation vocabulary. Phase 7.3 through Phase 7.5 add metadata-only
permission, request pipeline, and safety policy boundaries while keeping execution blocked by
default. Phase 7.6 checkpoints the runtime architecture in `docs/PHASE_7_CHECKPOINT.md`. Phase
8.0 through Phase 8.2 add execution lifecycle/session coordination metadata only. Phase 8.3 through
Phase 8.5 add adapter, bridge, and pre-integration readiness metadata only. Phase 9.0 through Phase
9.2 add Ollama local health/discovery only. Phase 9.3 may plan an inference boundary, but it must
remain explicitly scoped, interface-owned, policy-gated, deterministic in tests, and separate from
cloud provider integration, credentials, downloads, streaming, tool execution, plugins, vector
memory, and autonomous behavior unless a later phase approves those capabilities.
