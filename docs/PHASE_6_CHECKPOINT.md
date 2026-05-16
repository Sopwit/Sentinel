# Phase 6 Checkpoint

Phase 6 is checkpointed through the metadata-only model orchestration foundation. This checkpoint
does not start runtime execution and does not add provider, model, tool, plugin, network, streaming,
filesystem, or autonomous behavior.

## Completed Scope

- Provider/model catalog metadata is available through `IProviderCatalog` and
  `StaticProviderCatalog`.
- Model routing metadata is available through `IModelRouter` and `StaticModelRouter`.
- Routing mode preference is persisted through `AppSettings` and `JsonSettingsStore`.
- Capability graph and task planning metadata are available through `ITaskPlanner` and
  `StaticTaskPlanner`.
- Static agent metadata is available through `IAgentRegistry` and `StaticAgentRegistry`.
- Static memory taxonomy metadata is available through `IMemoryCatalog` and `StaticMemoryCatalog`.
- `OrchestrationSnapshot` aggregates current routing, planning, catalog, agent, memory, runtime,
  and activity metadata into a read model.
- `StaticOrchestrationDiagnostics` produces deterministic readiness reports from existing
  metadata only.
- `ConversationSession` owns high-level interaction/session metadata separately from chat history
  and agent runtime metadata.
- `ConversationStateGraph` owns deterministic conversation transition metadata separately from
  session, chat, and runtime metadata.
- `ApplicationController` owns the Phase 6 metadata objects and exposes summary/status strings.
- `DesktopShellViewModel` remains the QML boundary and exposes only QML-safe read-only strings,
  counts, and string lists for orchestration metadata.

## Architecture Findings

- Phase 6 boundaries remain value-based and deterministic.
- Provider catalog, router, task planner, agent registry, memory taxonomy, snapshot, diagnostics,
  conversation session, and conversation state graph remain separate responsibilities.
- `ChatSession`, `ConversationSession`, Phase 4 `RuntimeSession`, and `ConversationStateGraph`
  are not merged into a single object.
- QML does not receive raw provider catalogs, task plans, agent descriptors, memory shards,
  orchestration snapshots, diagnostics reports, conversation sessions, state graphs, runtime
  contexts, or controller internals.
- Existing UI visibility is read-only metadata display and does not expose setup, approval,
  execution, provider, model, tool, plugin, download, or scan controls.

## Known Limitations

- No real provider integration.
- No Ollama, OpenAI, Anthropic, or other provider calls.
- No networking, API keys, credentials, endpoints, retries, or streaming.
- No model downloads, model loading, model probing, or model execution.
- No real tool execution, shell/process launch, subprocess launch, filesystem/system mutation, or
  platform automation.
- No plugin loading or extension runtime.
- No vector database, embeddings, semantic search, or autonomous memory write/recall.
- No autonomous agents, background workers, timers, or dynamic discovery.
- Chat history remains one local transcript; multi-conversation persistence is not implemented.
- Runtime context, activity, orchestration, conversation session, and state graph metadata remain
  in-memory/read-only surfaces unless a later explicit phase changes that boundary.

## Phase 7 Readiness Criteria

- Preserve `IChatProvider`, `IAgentRuntime`, `IToolExecutor`, `IModelRouter`, `IProviderCatalog`,
  `ITaskPlanner`, `IAgentRegistry`, `IMemoryCatalog`, `IMemoryStore`, `IChatHistoryStore`, and
  `ISettingsStore` boundaries.
- Start with local runtime boundary planning and implementation only when Phase 7 explicitly
  scopes it.
- Keep any future local runtime capability behind explicit interfaces and deterministic tests.
- Keep cloud providers, credentials, networking, downloads, streaming, embeddings, vector search,
  plugins, and real tool execution out of scope until separately approved.
- Keep QML behind `DesktopShellViewModel` with QML-safe values only.
- Keep metadata-only readiness and snapshot surfaces deterministic.
- Require full tests and formatting verification before advancing from planning to runtime work.

## Recommended Phase 7 Breakdown

- Phase 7.0: Local runtime boundary planning and ownership map. Define what a local runtime
  boundary may own without enabling model/provider/tool execution. Completed as a metadata-only
  skeleton with `ILocalRuntime` and `NullLocalRuntime`.
- Phase 7.1: Local runtime session ownership metadata. Completed as a deterministic placeholder
  lifecycle/session skeleton without probing processes, scanning files, downloading models, or
  calling providers.
- Phase 7.2: Runtime capability negotiation layer. Completed as a metadata-only capability
  vocabulary and negotiation posture without activating runtime capabilities.
- Phase 7.3: Runtime permission and policy planning. Document how future execution, filesystem,
  network, plugin, and model capabilities would be explicitly gated.
- Phase 7.4: Local provider adapter skeleton if explicitly approved. Keep it disabled and
  metadata-only unless the phase scope allows real local provider calls.
- Phase 7.5: Execution-readiness UX planning. Keep any UI read-only until approval and execution
  boundaries are implemented in C++.

## Strictly Out Of Scope For The Checkpoint

- Adding product features.
- Starting real provider/model execution.
- Adding networking, API key handling, credentials, endpoints, downloads, or streaming.
- Adding real tool execution, shell/process launch, subprocess launch, filesystem/system actions,
  OS automation, or privileged behavior.
- Adding plugin loading.
- Adding vector databases, embeddings, semantic search, or autonomous memory behavior.
- Adding autonomous workers, background scanning, timers, or dynamic discovery.
- Redesigning the UI or adding execution/setup controls.
