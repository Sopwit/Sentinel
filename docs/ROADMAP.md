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

## Phase 9.3-9.5: Controlled Local Inference Boundary

Completed. Added a narrow, explicit local-only inference boundary for Ollama.

Delivered:

- Value-only local inference request, options, response, status, error, and trace records.
- `ILocalInferenceClient` with deterministic null refusal and a loopback-only Ollama
  implementation.
- Non-streaming `/api/generate` support only after controller permission and safety checks.
- Controller and desktop view-model QML-safe local inference status, summary, last-response
  summary, and trace exposure.
- Tests that use fakes and never require a real Ollama service.

Still out of scope:

- Streaming, model management, downloads/pulls/deletes, cloud endpoints, API keys, autonomous
  loops, tools/plugins, filesystem/system actions, subprocess launch, and broad UI changes.

## Phase 9.6-9.8: Model Selection, Runtime UX State, And Streaming Skeleton

Completed. Added selected-model metadata and runtime UX visibility without expanding execution.

Delivered:

- Persisted selected local model setting.
- Effective local model resolution from explicit model, selected model, or safe discovered-model
  fallback.
- Safe validation against discovered local Ollama model metadata when available.
- QML-safe selected-model summary, active runtime/model badge, idle/busy/error state, latency
  summary, and trace exposure.
- Disabled streaming skeleton types and client boundary for future token streaming work.
- Minimal Settings and Chat metadata visibility only.
- Tests for default/fallback behavior, invalid selected model behavior, busy/state metadata,
  disabled streaming skeleton, view-model exposure, and fake inference success.

Still out of scope:

- Model downloads, pulls, deletes, process launch, cloud calls, API keys, real streaming UI,
  automatic chat-to-Ollama routing, real tools/plugins, filesystem/system actions, autonomous
  loops, and broad UI redesign.

## Phase 10.0-10.2: Explicit Chat-To-Ollama Routing

Completed. Chat can now route to the controlled local inference boundary only through an explicit
opt-in setting.

Delivered:

- Persisted local chat inference enablement, disabled by default.
- QML-safe routing status and summary for Settings and Chat surfaces.
- `ApplicationController::sendMessage` preserves user-message append/history behavior, then uses
  local inference only when the opt-in is enabled, the selected/effective local model is valid, the
  Ollama endpoint is loopback HTTP, and permission/safety gates pass.
- Safe assistant responses for local inference refusals and errors, with no duplicate chat
  messages.
- Tests for disabled default behavior, missing/invalid model handling, fake local inference
  success, inference errors, non-loopback endpoint blocking, persistence, and view-model exposure.

Still out of scope:

- Streaming chat, model management/download/pull/delete UI, cloud provider routing, API keys,
  tools/plugins, filesystem/system actions, subprocess launch, and autonomous loops.

## Phase 10.3-10.5: Streaming Chat Boundary And Live Response UX

Completed. Added a local-only streaming response boundary and minimal live chat visibility.

Delivered:

- Persisted local inference streaming opt-in, disabled by default.
- `ILocalInferenceStreamClient` supports ordered chunk callbacks and safe stream result metadata.
- `OllamaLocalInferenceStreamClient` supports loopback-only `/api/generate` streaming with no
  redirects, no cloud endpoints, no credentials, no model-management actions, and no subprocesses.
- Chat streaming activates only after the local chat inference opt-in, streaming opt-in,
  valid-model resolution, loopback endpoint validation, and runtime permission/safety gates pass.
- Streaming chunks accumulate into a single final assistant message and are persisted once.
- Desktop view model exposes QML-safe streaming enabled/status/active/live-text/summary values.
- ChatPanel shows active streaming text without a broad redesign or model-management controls.
- Tests cover disabled default fallback, fake ordered chunks, malformed chunks, cancellation,
  duplicate-message protection, and view-model exposure without requiring Ollama.

Still out of scope:

- Cloud calls, API keys, model downloads/pulls/deletes, broad model management UI, tools/plugins,
  filesystem/system actions, subprocess launch, autonomous loops, and broad UI redesign.

## Phase 10.6-10.8: Model Selection And Local Runtime Management UX

Completed. Added action-light local model selection and runtime status visibility without adding
model-management operations.

Delivered:

- Settings lists discovered local Ollama model names when read-only `/api/tags` metadata is
  available.
- Selecting a discovered local model updates the persisted selected local model setting.
- Controller and desktop view model expose QML-safe selected model status, selected metadata
  summary, model names, and model summaries.
- Model metadata summaries include model name, size when present, modified date when present, and
  Local Only status.
- Invalid selected models are surfaced clearly and remain blocked by existing validation before
  inference or streaming can call the local client.
- Runtime UX now shows health, model count, selected-model status, chat inference enablement,
  streaming enablement, runtime badge, inference state, latency, and streaming status together.
- Tests use fakes and do not require a real Ollama service.

Still out of scope:

- Model downloads, pulls, deletes, installs, process/subprocess launch, cloud providers, API keys,
  endpoint expansion, filesystem/system actions, tools/plugins, autonomous loops, and broad model
  management.

## Phase 11.0-11.2: Lightweight Local Model Management Readiness

Completed. Prepared local model management UX and metadata without enabling management actions.

Delivered:

- `IModelManagementService` with deterministic `StaticModelManagementService` metadata.
- Value-only action, status, request, result, recommendation, and requirement summary types.
- Deterministic recommended local model metadata with approximate descriptive RAM/disk values.
- Controller and desktop view-model exposure through strings and string lists only.
- Read-only Settings model management readiness section showing installed model count through
  existing discovery, selected/effective model metadata, recommendations, requirements, and action
  unavailability.
- Tests for deterministic recommendations, unavailable action behavior, and QML-safe exposure.

Still out of scope:

- Real model downloads, pulls, deletes, installs, subprocess/process launch, filesystem/system
  scans or actions, cloud calls, API keys, tools/plugins, autonomous loops, and provider setup.

## Phase 11.3-11.6: Local AI Experience Stabilization And Runtime UX Polish

Completed. Polished the current local AI user flow without adding new runtime authority.

Delivered:

- More explicit local inference and streaming summaries for disabled, missing model, invalid model,
  endpoint-blocked, permission-blocked, safety-blocked, unavailable-client, timeout, invalid
  response, and request-failure cases.
- Streaming live text is now temporary: it is visible only while streaming is active, then cleared
  once the final assistant response is written through normal chat history.
- Chat history remains one local transcript, with user messages appended before routing and one
  assistant message appended from either the successful local response or a safe refusal/error.
- Settings model selection now makes no-selection/fallback/invalid/available states clearer and
  keeps recommendation summaries close to the selector.
- Tests use fake local clients and fake Ollama metadata; no real Ollama service is required.

Still out of scope:

- Model downloads, pulls, deletes, installs, real cancellation controls, cloud providers, API keys,
  endpoint expansion, subprocess launch, filesystem/system actions, tools/plugins, autonomous
  loops, and broad model-management workflows.

## Phase 11.7-11.9: Local AI Usability Checkpoint And Runtime QA

Completed. Checkpointed the current local AI user flow and runtime QA posture before Phase 12.

Delivered:

- Reviewed unavailable Ollama, no selected model, invalid selected model, disabled local chat
  inference, disabled/enabled streaming, permission/safety blocking, fake non-streaming success,
  and fake streaming success flows.
- Added focused tests for local AI settings persistence through JSON reload, safety-blocked local
  inference, stream-error live-preview cleanup, and expanded QML-safe local AI property exposure.
- Added `docs/PHASE_11_CHECKPOINT.md` with completed scope, current user flow, known limitations,
  safety guardrails, and Phase 12 readiness criteria.

Still out of scope:

- New product features, model pull/delete/install, cloud providers/API keys, filesystem/system
  actions, tools/plugins, subprocess launch, UI redesign, autonomous loops, and broad model
  management.

## Phase 17.0-17.3: Semantic Provider Planning And Local Selection

Completed. Prepared local-only semantic retrieval activation metadata without enabling semantic
retrieval.

Delivered:

- Semantic provider descriptors, selection metadata, readiness/health/capability summaries,
  activation policy, activation readiness, and refused activation result records.
- Planned provider modes: Disabled, Fake/InMemory test provider, Local Ollama embeddings provider,
  and Local file/vector index.
- Disabled remains the desktop default. Activation readiness refuses by default and reports the
  required steps for a later explicit activation phase.
- Controller/view-model/QML exposure for selected provider name/mode, capability summaries,
  disabled-by-default state, and activation readiness.
- Tests proving default disabled selection, fake metadata selection, planned inactive Ollama
  readiness, refused activation, deterministic retrieval stability, prompt stability, and
  QML-safe exposure.

## Phase 18.0-18.3: Agent Task Runtime Foundation

Completed. Prepared agent/task orchestration architecture without executing tools, filesystem
actions, shell commands, plugins, subprocesses, cloud/API calls, or autonomous actions.

Delivered:

- Value-only `AgentTask` records, ids, type/status/priority/source enums, plans, steps, results,
  traces, safety policy, and runtime status metadata.
- `IAgentTaskRuntime` with deterministic `StaticAgentTaskRuntime`.
- Static metadata tasks for conversation summarization, memory status inspection, response
  planning, retrieval-context preparation, voice-response preparation, and export-action
  preparation.
- Controller/view-model exposure for task runtime status, task count, latest task summary, and
  trace summaries.
- Compact read-only Agents page status with no execution buttons or approval/tool controls.

Still out of scope:

- Real task execution, tools/plugins, filesystem or system actions, shell/subprocess execution,
  background workers, cloud/API keys, provider/model execution, autonomous loops, semantic
  authority expansion, and broad UI redesign.

## Phase 18.4-18.6: Agent Task Queue And Lifecycle Metadata

Completed. Added deterministic task queue and lifecycle metadata without expanding runtime
authority.

Delivered:

- Value-only queue, queue status, queue summary, lifecycle, lifecycle event, queue policy, and
  queue result records.
- Static metadata queue ordered by priority, queue sequence, and task id.
- Metadata-only transitions for queued, planned, blocked, completed-as-metadata, and refused
  tasks.
- Controller/view-model exposure for queue counts, planned/active/blocked/completed/refused
  counts, latest lifecycle summary, and QML-safe task summaries.
- Compact read-only Agents page queue/readiness metadata with no execution, approval, tool,
  plugin, shell, filesystem, or background-worker controls.

Still out of scope:

- Real task execution, autonomous loops, background workers, approval flows, tools/plugins,
  filesystem or system actions, shell/subprocess execution, provider/model calls, cloud/API keys,
  and broad UI redesign.

## Phase 18.7-18.9: Agent Planning Session And Safety Arbitration Foundation

Completed. Added bounded planning-session and safety arbitration metadata without expanding
runtime authority.

Delivered:

- Value-only planning session, session id/status/summary/policy, result, candidate, budget,
  arbitration, refusal, safety-report, and fallback records.
- Deterministic planning candidates derived from the ordered task queue.
- Candidate, step, and summary budgets with deterministic omitted-candidate fallback summaries.
- Refusal metadata for unsafe plans, with safe blocked/refused summaries exposed to QML.
- Controller/view-model exposure for planning status, candidate/refusal counts, arbitration
  summaries, refusal summaries, and fallback summary.
- Compact read-only Agents page planning/arbitration visibility with no execution, approval,
  tool, plugin, shell, filesystem, or background-worker controls.

Still out of scope:

- Autonomous execution, approval workflows, tools/plugins, filesystem or system actions,
  shell/subprocess execution, background workers, provider/model calls, cloud/API keys, and broad
  UI redesign.

## Phase 18.10-18.12: Agent Capability Registry Foundation

Completed. Added deterministic, metadata-only capability registry records for future agent runtime
capabilities.

Delivered:

- Value-only capability id, type, status, scope, policy, requirement, restriction, readiness,
  safety, registry status, and registry summary records.
- Deterministic local metadata capabilities for conversation summarization, memory inspection,
  retrieval preparation, semantic supplement preparation, export preparation, and voice response
  preparation.
- Future filesystem access, shell execution, and plugin runtime entries that remain disabled or
  refused with safe summaries.
- Controller, desktop view-model, and Agents page visibility for counts, summaries, readiness, and
  safety metadata.

Still out of scope:

- Tools, plugins, filesystem actions, shell/subprocess execution, provider/model calls, cloud/API
  calls, approval flows, autonomous loops, and any runtime permission grant.

## Phase 18.13-18.15: Tool Contract And Permission Foundation

Completed. Added deterministic, metadata-only tool contract records for future tool/runtime
boundaries.

Delivered:

- Value-only tool contract id, type, status, scope, policy, permission, sandbox, restriction,
  readiness, safety, registry status, and registry summary records.
- Deterministic local metadata contracts for conversation summary, memory inspection, retrieval
  preparation, semantic supplement preparation, voice response preparation, and export
  preparation.
- Permission metadata for local-only, approval-required, sandbox-required, read-only, disabled,
  refused, future filesystem access, future subprocess execution, future plugin runtime, and
  future export action states.
- Sandbox readiness summaries for not-required, required-metadata, and denied states without
  starting any sandbox or runtime.
- Controller, desktop view-model, and Agents page visibility for contract counts, summaries,
  permission summaries, sandbox summaries, readiness summaries, and safety summaries.

Still out of scope:

- Real tools, plugin loading, filesystem actions, subprocess execution, export execution,
  approval flows, sandbox implementation, provider/model calls, cloud/API calls, autonomous loops,
  and any runtime permission grant.

Still out of scope:

- Semantic ranking/search, real embedding calls, vector database writes, prompt mutation, semantic
  prompt injection, cloud/API keys, downloads, filesystem scans, tools/plugins, and system actions.

## Phase 17.4-17.6: Hybrid Arbitration Simulation And Embedding Runtime Planning

Completed. Added safe semantic arbitration simulation metadata and local embedding runtime
planning without enabling semantic retrieval.

Delivered:

- Metadata records for semantic arbitration policy/status/result, simulated candidate scores,
  arbitration budget summaries, embedding runtime plan/budget/readiness, and compact future
  selection summaries.
- Deterministic simulated scoring with fixed source weights, bounded length buckets, stable source
  order, and candidate-id tie behavior.
- Controller/view-model/QML exposure for simulated arbitration readiness, budget summary,
  selection summaries, embedding runtime readiness, estimated runtime cost, local-only
  requirements, and disabled constraints.
- Tests proving deterministic scoring, stable ordering, deterministic tie handling, no retrieval
  planning mutation, prompt stability, runtime planning summaries, and QML-safe exposure.

Still out of scope:

- Real embeddings, vector search, vector database writes, filesystem indexing, Ollama embedding
  calls, provider/model inference, semantic prompt injection, cloud/API keys, tools/plugins,
  autonomous actions, and prompt mutation.

## Phase 17.7-17.9: Isolated Local Embedding Runtime Activation Foundation

Completed. Added an isolated local embedding generation foundation for readiness validation only.

Delivered:

- Runtime metadata records for status, health, session, generation policy, generation result,
  isolation policy, and generation readiness.
- Deterministic fake-provider isolated generation tests with no cloud provider, no API key, no
  vector database, and no filesystem indexing.
- Policy gates that require local-only mode, explicit semantic readiness, local/fake provider
  scope, blocked cloud providers, disabled filesystem indexing, disabled prompt integration,
  disabled retrieval ranking mutation, disabled automatic memory writes, disabled vector
  persistence, and disabled background indexing.
- Bounded runtime outcomes for success, failure, deterministic timeout, stale request, busy
  session, and policy refusal.
- Controller/view-model/QML exposure for readiness, last isolated test status, provider health,
  local-only bounded state, and safety checks.

Still out of scope:

- Semantic retrieval activation, vector search, vector database persistence, semantic prompt
  injection, retrieval ranking mutation, prompt assembly mutation, automatic memory writes,
  filesystem indexing, cloud/API providers or keys, provider downloads, autonomous actions, tools,
  and plugins.

## Phase 17.10-17.12: Local Vector Persistence Foundation

Completed. Added disabled-by-default local vector persistence lifecycle infrastructure.

Delivered:

- `VectorPersistencePolicy`, `VectorPersistenceStatus`, `VectorPersistenceHealth`,
  `VectorPersistenceReadiness`, `VectorPersistenceSession`, `VectorPersistenceBudget`,
  `VectorPersistenceResult`, `VectorIndexLifecycle`, and `VectorIndexSnapshotSummary`.
- A local-only deterministic lifecycle helper with explicit create, reset, clear, and bounded
  acceptance of successful isolated embedding runtime output metadata.
- Stable empty-index, stale-session, busy-state, bounded-limit, lifecycle revision, and snapshot
  summaries.
- Controller/view-model/QML exposure for readiness, lifecycle status, bounded/local-only state,
  disabled-by-default state, indexed item count, and safety checks.
- Tests proving deterministic lifecycle behavior, create/reset/clear, empty-index handling,
  stale/busy refusal, bounded overflow, unchanged retrieval planning, unchanged prompt assembly,
  and QML-safe exposure.

Still out of scope:

- Semantic retrieval activation, semantic ranking/search, semantic prompt injection, automatic
  indexing, filesystem scanning, background ingestion, automatic memory conversion, retrieval
  planning mutation, prompt assembly mutation, cloud/API/vector services, raw vector UI, paths,
  debug payloads, autonomous actions, tools, and plugins.

## Phase 17.13-17.15: Controlled Local Semantic Search Activation

Completed. Enabled a bounded local semantic candidate-search foundation for readiness validation
and hybrid orchestration testing.

Delivered:

- `SemanticSearchPolicy`, `SemanticSearchStatus`, `SemanticSearchResult`,
  `SemanticSearchCandidate`, `SemanticSearchBudget`, `SemanticSearchSession`,
  `SemanticSearchReadiness`, and `SemanticSearchArbitrationSummary`.
- Local-only deterministic search over local vector persistence entries only, gated by successful
  isolated embedding runtime output metadata.
- Bounded candidate count, bounded timeout metadata, bounded similarity scores, stale request
  protection, busy-state refusal, stable tie handling, and safe empty-index behavior.
- Hybrid arbitration summaries that report semantic candidates as metadata-only while deterministic
  retrieval remains final prompt authority.
- Controller/view-model/QML exposure for readiness, runtime state, candidate counts, bounded
  summaries, arbitration boundaries, and safety checks.
- Tests for deterministic candidate ordering, limits, timeout, stale/busy handling, empty index,
  local-only/non-authoritative enforcement, unchanged retrieval planning and prompt assembly, and
  QML-safe exposure.

Still out of scope:

- Semantic prompt authority, semantic prompt injection, retrieval-planning mutation, prompt block
  mutation, deterministic ranking override, filesystem indexing, background ingestion, cloud/API
  providers, provider downloads, autonomous actions, tools, plugins, raw vectors, and debug payload
  dumps.

## Phase 17.16-17.18: Hybrid Retrieval Bridge Foundation

Completed. Added bounded bridge metadata that lets deterministic retrieval planning optionally
consume semantic candidate suggestions without making semantic retrieval authoritative.

Delivered:

- `HybridRetrievalBridgePolicy`, `HybridRetrievalBridgeStatus`,
  `HybridRetrievalBridgeResult`, `HybridBridgeCandidate`, `HybridBridgeBudget`,
  `HybridBridgeReadiness`, `HybridBridgeArbitration`, and `HybridBridgeSourceSummary`.
- Deterministic-first bridge arbitration: deterministic retrieval candidates fill bridge capacity
  first, semantic candidates can only fill unused bounded metadata capacity, and deterministic
  candidates win ties/conflicts.
- Safe deterministic fallback for disabled, empty, stale, busy, timed-out, or refused semantic
  sources.
- Explicit non-mutation checks for `RetrievalPlanningResult`, `PromptContextBlock` values, prompt
  assembly, and semantic prompt authority.
- Controller/view-model/QML exposure for bridge status, readiness, candidate counts,
  deterministic-vs-semantic participation, arbitration summaries, fallback summaries, and checks.
- Tests for deterministic authority preservation, semantic-disabled fallback, bounded candidate
  limits, stable ordering, stale/busy/timeout behavior, no prompt/retrieval mutation, and
  controller/view-model exposure.

Still out of scope:

- Semantic prompt authority, semantic prompt injection, deterministic ranking override, filesystem
  indexing, background ingestion, cloud/API/vector providers, provider downloads, autonomous
  actions, tools/plugins, raw vectors, raw prompt payloads, provider handles, filesystem paths, and
  debug dumps.

## Phase 17.19-17.21: Deterministic Semantic Acceptance Layer

Completed. Added bounded deterministic acceptance for semantic advisory candidates.

Delivered:

- `SemanticAcceptancePolicy`, `SemanticAcceptanceStatus`, `SemanticAcceptanceResult`,
  `SemanticAcceptedCandidate`, `SemanticAcceptanceBudget`, `SemanticAcceptanceReadiness`,
  `SemanticAcceptanceArbitration`, `SemanticAcceptanceFallback`, and
  `SemanticAcceptanceSourceSummary`.
- Deterministic approval gates over retrieval planning, hybrid bridge metadata, and semantic
  search metadata.
- Supplemental-only acceptance: deterministic candidates stay primary, semantic candidates can
  only fill unused supplement capacity, and deterministic retrieval wins conflicts.
- Bounded acceptance count, bounded supplement-character budget, deterministic ordering, timeout,
  stale, busy, disabled, semantic-error, and deterministic-only fallback metadata.
- Controller/view-model/QML exposure for readiness/status, approved supplement counts,
  deterministic-vs-semantic participation, fallback state, arbitration summaries, bounded budgets,
  and local-only/non-authoritative checks.
- Tests proving deterministic authority, supplement-only behavior, stable ordering, limits,
  fallback states, mutation blocking, no prompt/retrieval mutation, no deterministic replacement,
  and controller exposure.

Still out of scope:

- Semantic prompt authority, raw semantic/vector prompt payloads, retrieval-planning mutation,
  prompt block mutation, deterministic candidate replacement, source-priority mutation,
  filesystem indexing, cloud/API/vector providers, autonomous actions, tools/plugins, provider
  handles, filesystem paths, and debug dumps.

## Phase 17.22-17.24: Semantic Supplement Prompt Assembly Readiness

Completed. Prepared accepted semantic supplements for future safe prompt assembly without granting
semantic prompt authority.

Delivered:

- `SemanticSupplementBlock`, `SemanticSupplementBundle`,
  `SemanticSupplementAssemblyPolicy`, `SemanticSupplementAssemblyStatus`,
  `SemanticSupplementAssemblyResult`, `SemanticSupplementBudget`,
  `SemanticSupplementReadiness`, and `SemanticSupplementSafetyReport`.
- Disabled-by-default assembly that reads `SemanticAcceptanceResult` and returns safe fallback
  metadata unless a guarded test-only assembly policy is explicitly enabled.
- Separate bounded supplement metadata bundles with deterministic ordering and deterministic
  truncation.
- Authority checks proving supplements cannot mutate `PromptContextBlock`, mutate
  `RetrievalPlanningResult`, replace/reorder deterministic context, override conversation
  windows/summaries/committed memory/runtime metadata, or enter live prompts.
- Controller/view-model/QML exposure for readiness/status, supplement count, budget summary,
  safety summary, disabled-by-default state, and non-authoritative checks.
- Tests for disabled fallback, bounded assembly, deterministic ordering/truncation, prompt
  behavior stability, deterministic context separation, and controller/view-model exposure.

Still out of scope:

- Live semantic prompt authority, semantic prompt injection, raw prompt block display, raw
  vectors/scores, provider handles, filesystem paths, debug dumps, filesystem indexing,
  cloud/API/vector providers, autonomous actions, tools/plugins, and broad UI redesign.

## Phase 17.25-17.27: Semantic Prompt Authority Policy Foundation

Completed. Added the disabled-by-default policy gate for future semantic prompt inclusion
decisions.

Delivered:

- `SemanticPromptAuthorityPolicy`, `SemanticPromptAuthorityStatus`,
  `SemanticPromptAuthorityResult`, `SemanticPromptAuthorityReadiness`,
  `SemanticPromptAuthorityDecision`, `SemanticPromptAuthoritySafetyReport`,
  `SemanticPromptAuthorityFallback`, and `SemanticPromptAuthorityAuditSummary`.
- Default Disabled/Denied authority posture with deterministic-only fallback and audit summaries.
- Evaluation over `SemanticSupplementAssemblyResult` only, with no mutation of
  `PromptContextBlock`, `RetrievalPlanningResult`, or default prompt assembly.
- Test-only "would include metadata" readiness when local-only semantic search, deterministic
  acceptance, bounded bundles, explicit prompt-injection enablement, explicit policy allow, and a
  passing safety report all hold.
- Controller/view-model/QML exposure for compact status, decision, readiness, safety, fallback,
  audit, block count, and checks.

Still out of scope:

- Live semantic prompt injection, semantic authority escalation, raw prompt or supplement block
  display, raw vectors/scores, filesystem indexing, cloud/API/vector providers, provider handles,
  debug dumps, autonomous actions, tools/plugins, and broad UI redesign.

## Phase 17.28-17.30: Controlled Semantic Prompt Inclusion

Completed. Added the disabled-by-default live inclusion gate for bounded semantic supplements.

Delivered:

- `SemanticPromptInclusionPolicy`, `SemanticPromptInclusionStatus`,
  `SemanticPromptInclusionResult`, `SemanticPromptInclusionBudget`,
  `SemanticPromptInclusionSafetyReport`, `SemanticPromptInclusionFallback`, and
  `SemanticPromptInclusionAuditSummary`.
- Inclusion activates only when context injection is enabled, semantic prompt authority approves,
  supplement assembly is bounded and safe, local-only mode is active, and safety passes.
- Semantic supplements, when active, are appended after deterministic context in a separate
  supplemental/non-authoritative block with deterministic count and character budgets.
- Disabled, denied, unsafe, empty, stale, busy, timeout, and refused states fall back to the exact
  deterministic-only prompt.
- Controller/view-model/QML exposure for inclusion enabled/status, included count, budget,
  fallback, audit, and deterministic-authority-preserved summaries without exposing raw prompt or
  supplement payloads.

Still out of scope:

- Filesystem indexing, cloud/API/vector providers, raw vectors/scores, raw prompt display,
  provider handles, debug dumps, autonomous actions, tools/plugins, deterministic context
  replacement/reordering, and broad UI redesign.

## Phase 17.31-17.33: Semantic Retrieval And Prompt Inclusion Checkpoint

Completed. Audited the complete Phase 17 semantic retrieval and controlled inclusion architecture
without adding feature expansion.

Delivered:

- Added `docs/PHASE_17_SEMANTIC_CHECKPOINT.md`.
- Confirmed deterministic retrieval remains the final prompt authority.
- Confirmed semantic inclusion is disabled by default, explicit opt-in, bounded, local-only, and
  policy-gated.
- Confirmed semantic supplement inclusion falls back to deterministic-only prompts for disabled,
  denied, unsafe, empty, stale, busy, timed-out, and refused semantic states.
- Confirmed QML exposure remains status/count/summary/check metadata only, with no raw prompts,
  supplement content, vectors, scores, provider handles, filesystem paths, or debug dumps.
- Confirmed existing focused tests cover the checkpoint guarantees, so no redundant QA tests were
  added.

Still out of scope:

- Filesystem indexing, cloud/API/vector provider activation, provider downloads, tools/plugins,
  autonomous actions, prompt-authority expansion, raw semantic payload UI, and broad UI redesign.

## Phase 15.8: Async Local Runtime Worker Foundation

Completed. Local Ollama chat inference now crosses an async worker boundary before real
non-streaming or streaming generation is executed.

Delivered:

- `ILocalInferenceWorker` wraps the existing local inference and stream clients.
- Real Ollama generate/stream calls are dispatched off the controller/UI thread and completed via
  request-id guarded callbacks.
- Chat behavior remains stable: one user message, one final assistant message on success, no
  partial assistant persistence on failure, preview cleanup after streaming error/completion, and
  duplicate-send rejection while busy.
- Metadata-only cancellation invalidates the active request id so stale async results are ignored.
- Tests use fake workers/clients and require no real Ollama service.

Still out of scope:

- Launching Ollama, managing model loads, downloading/pulling/deleting models, cloud providers,
  API keys, tools/plugins, filesystem/system actions, autonomous loops, microphone access,
  playback, Piper changes, and Whisper execution.

## Phase 15.9: Conversation Runtime State And Session Continuity

Completed. Tightened the chat/session continuity layer around async local inference.

Delivered:

- Conversation-runtime state now summarizes current graph state, request id, active model, active
  route, streaming activity, last success, last error/refusal, and latency.
- The desktop view model exposes only QML-safe strings, string lists, and booleans.
- Chat shows a compact current session/route/request status without debug traces.
- Restart loading keeps persisted chat history stable and does not add duplicate startup system or
  assistant messages when stored history exists.
- Clear Chat clears runtime state and persistent chat consistently through the existing
  chat-history boundary.
- Tests cover async success/failure state updates, stale result behavior, clear/reset continuity,
  persisted startup loading, duplicate-message avoidance, and QML-safe exposure.

## Phase 15.11-15.13: Conversation Search, Export Readiness, And Transcript QA

Completed. Added narrow transcript QA metadata without expanding storage or filesystem authority.

Delivered:

- Literal case-insensitive search over the current in-memory single transcript.
- QML-safe search query/status/summary/result-count/result-summary exposure.
- Clear Chat resets search metadata to the empty-query state.
- Disabled export readiness metadata for Plain Text, Markdown, and JSON formats.
- Metadata-only export requests return disabled/not-implemented results and write no files.
- Compact Chat and Settings visibility for search/export state, plus a small current-transcript
  search field in Chat.
- Focused tests for user/assistant matches, empty query behavior, no mutation during search, clear
  reset, export no side effects, and view-model exposure.

Still out of scope:

- Persisted search, SQLite FTS, vector search, semantic search, embeddings, export file writes,
  file pickers, import, transcript browser, multi-conversation storage, encryption, pruning,
  cloud/API keys, tools/plugins, filesystem/system actions, Piper changes, Whisper execution,
  playback, microphone access, and autonomous loops.

## Phase 15.14-15.16: Local Transcript Export Implementation

Completed. Implemented controlled local export for the current single transcript.

Delivered:

- Markdown and JSON export actions for the current transcript only.
- App-controlled export directory below Qt `AppDataLocation` via the path-provider boundary.
- Sanitized timestamped filenames with uniqueness handling to avoid surprise overwrites.
- Export refusals for empty transcripts with no user or assistant messages.
- QML-safe result metadata: status, safe filename, exported message count, last export timestamp,
  and concise error/refusal summaries.
- Small Chat and Settings export actions without broad redesign.
- Focused tests for Markdown content, JSON structure, empty-refusal behavior, filename behavior,
  view-model exposure, and no arbitrary output path action.

Still out of scope:

- Import, arbitrary file picker/output path, transcript browser, multi-conversation export,
  encryption, pruning, cloud/API sync, external processes, tools/plugins, semantic/vector search,
  cloud/API keys, tools/plugins, filesystem/system actions, and model-management operations.

## Phase 15.17-15.19: Conversation Browser Metadata Foundation

Completed. Added a single-transcript conversation-list foundation for future browser UX.

Delivered:

- Value-only metadata records for browser-readiness:
  `ConversationDisplayTitle`, `ConversationListEntry`, `ConversationListSummary`,
  and `ConversationBrowserStatus`.
- Deterministic single-entry exposure for the active local transcript: title, message count,
  persistence status, last updated/saved summary, and search/export availability summaries.
- QML-safe controller/view-model properties for compact Settings visibility.
- Compact Settings “Current Transcript” readiness rows without sidebar/thread redesign.
- Focused tests for deterministic single entry, empty summary, count summary, clear-chat update,
  search/export availability reflection, and QML-safe exposure.

Still out of scope:

- Multi-conversation/thread storage, full transcript browser UI, database migration, import changes,
  arbitrary export paths, cloud sync, tools/plugins, or model/runtime boundary expansion.

## Phase 15.20-15.22: Multi-Conversation Planning Skeleton

Completed. Prepared multi-conversation migration planning metadata without changing active storage.

Delivered:

- Planning abstractions: `ConversationId`, `ConversationDescriptor`,
  `ConversationLifecycleStatus`, `ConversationStorageMode`, `ConversationMigrationReadiness`, and
  `ConversationSchemaPlan`.
- Deterministic readiness/status metadata exposing:
  current storage mode (`Single Transcript`), future storage mode (`Multi Conversation`), migration
  readiness (`Not Started`), migration status summary (`Not Started / Planned`), and schema status
  summary.
- QML-safe forwarding in `DesktopShellViewModel` and compact read-only Settings visibility via a
  “Multi-conversation readiness” row.
- Focused tests for deterministic metadata, unchanged single-transcript behavior, no storage
  mutation from readiness reads, and QML-safe exposure.

Still out of scope:

- SQLite schema migration, multi-conversation persistence, transcript import, cloud sync, and full
  browser/thread controls.

## Phase 15.23-15.25: Multi-Conversation Storage Foundation

Completed. Added a real multi-conversation store boundary while preserving current
single-transcript chat behavior.

Delivered:

- `IConversationStore` plus conversation/message record, status, and error value types.
- In-memory and SQLite conversation stores with create/list/load, append-message, deterministic
  ordering, rename/archive/delete metadata operations, and SQLite persistence across instances.
- Controller and desktop view-model read-only exposure for store status, conversation count, active
  conversation summary, and conversation summaries.
- Settings read-only storage-readiness rows, with no browser/thread workflow switch.
- Tests for store behavior, SQLite persistence, no destructive migration of `IChatHistoryStore`,
  current single-transcript behavior, and controller/view-model exposure.

Still out of scope:

- Cloud sync, transcript import, arbitrary filesystem writes, broad UI redesign, and using the
  multi-conversation store as the active chat transcript path.

## Phase 15.26-15.29: Conversation Browser UI Foundation And Safe Session Switching

Completed. Activated the multi-conversation store as the active local transcript source behind a
small Chat browser surface.

Delivered:

- App-owned SQLite conversation database path (`conversations.sqlite3`) and desktop wiring for
  `SQLiteConversationStore`.
- Controller-owned active conversation lifecycle: initialize a valid active conversation, copy
  legacy single-transcript startup messages into an empty conversation store when available, load
  selected conversation messages into `ChatSession`, and keep archived conversations read-only for
  sending.
- Compact Chat browser showing title, updated summary, message count, archived state, and actions
  for create, switch, rename, archive, and unarchive.
- Session switching semantics that cancel/invalidate active async request metadata, clear streaming
  preview text, reset conversation runtime/search state, and load the selected transcript without
  duplicate user or assistant insertion.
- Stale async result protection remains request-id based; completions from a previous active
  conversation are ignored after switching.
- Focused controller, SQLite, and desktop view-model tests without real Ollama.

Still out of scope:

- Permanent delete UI, cloud sync, import, multi-conversation export, embeddings/vector DB,
  semantic memory, tools/plugins/system execution, and changes to Ollama/runtime safety policy.

## Phase 15.30-15.32: Conversation Browser Polish And Safe Delete Readiness

Completed. Improved the compact browser UX and prepared permanent-delete safety metadata without
enabling destructive deletion.

Delivered:

- Clearer current-conversation row treatment, visually muted archived rows, active/inactive row
  summaries, active and archived counts, and empty-state copy when no user-created conversations
  exist.
- Compact rename feedback for saved/refused rename attempts.
- Archived active conversation hinting: sending is disabled with explicit copy until unarchived.
- `ConversationDeletePolicy`, `ConversationDeleteReadiness`, and `ConversationDeleteResult`
  metadata, forwarded through `ApplicationController` and `DesktopShellViewModel` as QML-safe
  strings, booleans, counts, and string lists.
- Archive-first policy is documented and visible. Permanent delete is disabled by default, and the
  delete request path refuses safely without mutating storage.
- Focused tests for archived send blocking, archive/unarchive summaries, disabled delete
  readiness, no-mutation delete refusal, active-id validity, and QML-safe exposure.

Still out of scope:

- Destructive permanent delete UI or storage mutation, cloud sync, import, multi-conversation
  export, arbitrary filesystem writes, broad UI redesign, model/voice/tool/plugin behavior, and
  runtime safety-policy changes.

## Phase 15.33-15.35: Conversation Browser Runtime QA And Checkpoint

Completed. Audited the current conversation browser runtime path and checkpointed Phase 15
conversation readiness before Phase 16.

Delivered:

- Reviewed multi-conversation store/browser/session switching architecture.
- Added `docs/PHASE_15_CONVERSATION_CHECKPOINT.md`.
- Added focused QA tests for SQLite soft-delete row retention and controller permanent-delete
  refusal without SQLite mutation.
- Confirmed single-transcript compatibility startup behavior, non-destructive SQLite conversation
  storage, archived-send blocking, stale async result protection after switching, and disabled
  permanent-delete readiness.

Still out of scope:

- Semantic memory, embeddings/vector DB, cloud sync, import/export changes, permanent delete
  execution, broad UI redesign, model/voice/tool/plugin changes, and runtime authority expansion.

## Phase 12.0-12.2: Voice Boundary And TTS/STT Planning Skeleton

Completed. Prepared voice architecture without enabling real audio I/O or voice runtime work.

Delivered:

- `VoiceCapability`, `VoiceProviderDescriptor`, `VoiceProviderStatus`, `VoiceRuntimeMode`,
  `VoiceRequest`, `VoiceResponse`, and `VoiceReadinessReport`.
- `ITextToSpeechProvider` and `ISpeechToTextProvider` boundaries.
- Deterministic null TTS/STT providers that refuse safely while disabled.
- Controller and desktop view-model exposure through QML-safe strings, string lists, and booleans.
- Read-only Settings Voice Readiness metadata with no voice action buttons.
- Tests for disabled null-provider behavior, readiness summaries, controller exposure, and
  view-model exposure.

Future Piper/Whisper path:

- Piper should enter behind `ITextToSpeechProvider` only after an explicit audio-output/runtime
  phase defines local binary ownership, model paths, permission prompts, playback ownership, and
  safety policy.
- Whisper should enter behind `ISpeechToTextProvider` only after an explicit audio-input/runtime
  phase defines microphone permission, capture lifecycle, local model ownership, transcription
  privacy, and cancellation/error handling.
- Both integrations must remain local-first, injectable for tests, and separated from QML.

Still out of scope:

- Recording, playback, Whisper/Piper execution, subprocess/process launch, downloads,
  filesystem/system actions, cloud calls, API keys, voice buttons, record/speak buttons, and broad
  UI redesign.

## Phase 12.3-12.6: Voice Runtime Planning And Session Orchestration Skeleton

Completed. Added a metadata-only voice runtime/session orchestration skeleton without enabling
audio I/O or runtime execution.

Delivered:

- `VoiceSession`, `VoiceSessionId`, `VoiceSessionState`, `VoicePipelineStage`,
  `VoicePipelineStatus`, `VoicePipelineTrace`, `IVoiceRuntimeCoordinator`, and
  `StaticVoiceRuntimeCoordinator`.
- Deterministic voice pipeline metadata for idle, preparing, awaiting-input,
  transcribing-placeholder, inference-placeholder, synthesis-placeholder, completed, blocked, and
  error states.
- Runtime/readiness summaries for runtime unavailable, TTS unavailable, STT unavailable,
  microphone disabled, playback disabled, local-only policy active, and process execution disabled.
- Controller and desktop view-model exposure through QML-safe strings, string lists, and booleans.
- Read-only Settings voice runtime/session/pipeline metadata with no microphone, playback, voice
  activation, setup, or model-management controls.
- Tests for pipeline transitions, blocked/error metadata, controller exposure, view-model exposure,
  and no-runtime posture.

Future Piper/Whisper runtime boundary:

- Piper may only enter through `ITextToSpeechProvider` after a later explicit phase defines binary
  ownership, model path ownership, playback lifecycle, cancellation, permission prompts, and
  safety policy.
- Whisper may only enter through `ISpeechToTextProvider` after a later explicit phase defines
  microphone permission, capture lifecycle, local model ownership, transcription privacy,
  cancellation, and error handling.
- Voice runtime coordination must remain injectable and testable; QML must continue consuming
  strings, string lists, and booleans rather than provider/runtime objects.

Still out of scope:

- Microphone access, audio playback, Piper/Whisper execution, subprocess/process launch,
  filesystem/system actions, downloads, cloud calls, API keys, voice activation, autonomous loops,
  and broad UI redesign.

## Phase 12.7-12.9: Voice Checkpoint And Local Voice Integration Planning

Completed. Checkpointed Phase 12 voice architecture and documented the prerequisites for future
local Piper and Whisper integration without adding real audio behavior.

Delivered:

- Reviewed TTS/STT provider boundaries, voice session/pipeline metadata, static runtime
  coordination, controller ownership, and QML-safe view-model exposure.
- Added `docs/PHASE_12_CHECKPOINT.md` with completed scope, current architecture, known
  limitations, safety guardrails, future Piper plan, future Whisper plan, and Phase 13 readiness
  criteria.
- Confirmed existing voice tests cover deterministic pipeline traces, null provider refusal,
  controller/view-model exposure, blocked/error metadata, and disabled runtime posture.

Still out of scope:

- Microphone access, audio playback, Piper/Whisper execution, subprocess/process launch,
  filesystem/system actions, downloads, cloud calls, API keys, voice buttons, activation flows,
  autonomous voice loops, and broad UI redesign.

## Phase 13.0-13.2: Local Voice Runtime Safety, Binary Ownership, And Model Ownership Skeleton

Completed. Prepared local Piper/Whisper ownership metadata before any executable voice runtime
work.

Delivered:

- `VoiceBinaryDescriptor`, `VoiceBinaryStatus`, `VoiceModelDescriptor`, `VoiceModelStatus`,
  `VoiceRuntimePermission`, and `VoiceRuntimeSafetyReport`.
- `IVoiceRuntimeEnvironment` plus deterministic null/static environment implementations.
- Missing/not-configured Piper and Whisper binary/model summaries for expected future ownership.
- Default-deny voice runtime permission metadata for microphone, playback, process execution, and
  model reads.
- Safety reporting that blocks execution by default and explicitly blocks microphone access,
  playback, process execution, filesystem-wide scans, downloads, and cloud/API-key behavior.
- Controller and desktop view-model exposure through QML-safe strings, string lists, and booleans.
- Read-only Settings visibility for voice environment, binary, model, permission, and safety
  metadata, with no setup controls.

Future Piper/Whisper ownership:

- Piper remains behind `ITextToSpeechProvider`; this phase only records binary/model path
  expectations and execution safety posture.
- Whisper remains behind `ISpeechToTextProvider`; this phase only records binary/model path
  expectations and blocked microphone/transcription posture.
- A later explicit phase must define user-controlled path selection, local model compatibility,
  permission prompts, lifecycle, cancellation, errors, and cleanup before execution.

Still out of scope:

- Microphone access, audio playback, Piper/Whisper execution, subprocess/process launch,
  filesystem-wide scans, model loading, downloads, cloud calls, API keys, voice controls, path
  pickers, and broad UI redesign.

## Phase 13.3-13.5: Piper TTS Adapter Skeleton

Completed. Added a safe Piper text-to-speech adapter boundary without enabling audio playback or
Piper process execution.

Delivered:

- `PiperTtsConfig`, `PiperVoiceModelDescriptor`, `PiperTtsRequest`, `PiperTtsResult`,
  `PiperTtsStatus`, `IPiperTtsClient`, `NullPiperTtsClient`, and
  `PiperTextToSpeechProvider`.
- Default-disabled/not-configured Piper readiness metadata.
- Deterministic refusal for missing Piper binary or missing Piper voice model metadata.
- Provider status summaries that remain behind `ITextToSpeechProvider`.
- Controller and desktop view-model exposure through QML-safe strings, string lists, and booleans.
- Read-only Settings Piper readiness rows with no voice action controls.
- Tests that do not require Piper, voice models, audio devices, subprocess execution, or downloads.

Still out of scope:

- Audio playback, Piper execution, file-output synthesis, microphone access, Whisper/STT,
  downloads, cloud/API-key behavior, filesystem-wide scans, automatic runtime probing, model
  loading, speak buttons, path/model pickers, and broad UI redesign.

Next:

- A later explicit phase can add controlled file-output TTS only after user-controlled binary/model
  paths, enabling policy, lifecycle, cancellation, cleanup, and runtime safety checks are defined.

## Phase 13.6-13.8: Controlled Piper TTS File-Output Boundary

Completed. Added explicit local-only Piper file-output synthesis behind the existing provider and
client interfaces, while preserving disabled-by-default behavior.

Delivered:

- Piper file-output request/result metadata for controlled output path, timeout, exit code, error,
  and trace summaries.
- A configured/succeeded/failed/timeout/refused status vocabulary for the file-output boundary.
- Strict gates for enabled configuration, valid executable Piper binary, valid readable voice
  model, app-controlled output directory, explicit process permission, local-only request posture,
  playback disabled, microphone disabled, no downloads, no cloud/API-key behavior, and no
  filesystem-wide scans.
- `ProcessPiperTtsClient` as the real local file-output client behind `IPiperTtsClient`, plus
  deterministic fake-client test coverage.
- Controller/view-model QML-safe Piper file-output status and summary exposure.
- Settings readiness visibility only; no speak/play control, path picker, or model picker.

Still out of scope:

- Audio playback, microphone access, automatic voice loops, Whisper/STT, model downloads,
  cloud/API-key behavior, broad filesystem scans, uncontrolled subprocess execution, and broad UI
  redesign.

## Phase 13.9: Voice/Piper Checkpoint And Readiness Review

Completed. Reviewed the current voice/Piper architecture and documented Phase 14 readiness without
adding runtime behavior.

Delivered:

- Added `docs/PHASE_13_CHECKPOINT.md`.
- Confirmed the current TTS path is `text -> Piper provider -> gated file-output metadata`.
- Confirmed Piper remains disabled by default.
- Confirmed file-output synthesis is local-only and reachable only behind explicit provider,
  request, binary/model, controlled-output-path, permission, and safety gates.
- Confirmed no playback, microphone access, Whisper execution, downloads, cloud/API-key behavior,
  or autonomous voice loop exists.
- Documented future Piper path/model configuration UX, controlled playback, Whisper STT adapter,
  and voice conversation loop phases.

Still out of scope:

- New runtime behavior, audio playback, microphone access, Whisper/STT execution, downloads,
  cloud/API-key behavior, autonomous voice loops, path/model pickers, speak/play/record controls,
  and broad UI redesign.

## Phase 14.0-14.3: Local Voice Configuration UX For Piper And Whisper

Completed. Added persisted local Piper/Whisper path configuration and compact Settings visibility
without enabling voice execution.

Delivered:

- Persisted Piper binary path, Piper model path, Whisper binary path, and Whisper model
  directory/path settings.
- Settings Voice Configuration section with text fields, current configured values, readiness
  summary, binary/model metadata, and existing Piper readiness rows.
- Exact configured-path validation only: exists/missing, readable/unreadable, and
  executable/non-executable for binaries.
- `ApplicationController` and `DesktopShellViewModel` QML-safe exposure for configured path
  strings and readiness summaries.
- Piper readiness updates from configured metadata but remains blocked by the existing safety
  posture unless a later execution phase explicitly changes the gates.
- Tests for defaults, fake configured files/directories, missing paths, executable and
  non-executable binaries, persistence reload, view-model exposure, and no execution side effects.

Still out of scope:

- Running Piper, running Whisper, microphone capture, audio playback, model downloads,
  filesystem-wide scans, cloud/API keys, autonomous voice loops, path picker integration, and voice
  action controls.

## Phase 14.4-14.6: Voice Configuration UX Polish And Safe Auto-Detection Hints

Completed. Improved the Piper/Whisper configuration surface and added safe read-only path hints
without enabling voice execution.

Delivered:

- Settings shows each configured path clearly with short help text, compact status badges, and
  concise read-only hints.
- Piper and Whisper binary hints check only fixed known Homebrew/local locations and never run the
  binaries.
- Piper and Whisper model hints use configured-path readability only; no recursive or broad model
  discovery is performed.
- `ApplicationController` and `DesktopShellViewModel` expose hint/status values as strings and
  string lists only.
- Tests cover deterministic configured-path hint behavior, missing-path badges, QML-safe exposure,
  and the no-execution boundary.

Still out of scope:

- Automatic settings writes from hints, path pickers, running Piper, running Whisper, microphone
  capture, audio playback, downloads, cloud/API keys, filesystem-wide scans, and autonomous voice
  loops.

## Phase 14.7-15.0: Controlled Local Ollama Runtime Activation

Completed. Activated the existing controlled Ollama path in the desktop app for explicit
local-only chat inference.

Delivered:

- Real desktop wiring for the loopback-only Ollama runtime health/discovery client.
- Real desktop wiring for non-streaming local Ollama `/api/generate` inference.
- Streaming-capable local Ollama client remains opt-in and architecture-ready.
- Persisted selected local model remains the chat inference target when valid.
- A narrow local-only runtime permission policy allows only explicit local inference and denies
  tools, providers, external processes, filesystem access, broader network access, and plugins.
- Dashboard, chat, and settings continue to expose QML-safe runtime availability, selected model,
  inference status, and streaming status.
- Tests remain deterministic with fake runtime and inference clients.

Still out of scope:

- Cloud providers/API keys, model downloads/pulls/deletes, Ollama process management, autonomous
  agents, tools/plugins, shell execution, filesystem-wide actions, microphone access, playback,
  Piper/Whisper execution, and autonomous voice loops.

## Phase 6.7: Orchestration Diagnostics and Readiness Checklist

Completed. Added deterministic readiness diagnostics over existing orchestration metadata only.

Delivered:

- Value-only diagnostic levels, diagnostic entries, readiness checks, and readiness reports.
- Static readiness generation for routing, selected route, catalogs, planner availability, snapshot
  health, local-only privacy posture, cloud provider unavailability, and disabled execution
  capability.
- Read-only controller/view-model status, summary, and diagnostic string exposure.
- Minimal Dashboard and Settings visibility without setup or execution controls.

Still out of scope:

- Provider/model execution, provider probing, networking/API keys, model downloads, filesystem or
  system scans/actions, real tool execution, plugin loading, embeddings, vector search, and
  autonomous workers.

## Phase 6.8: Runtime Context Session Layer

Completed. Added a metadata-only conversation/session context layer above chat history and agent
runtime metadata.

Delivered:

- Value-only conversation session id/status, interaction mode, attention state, context scope, and
  context-window metadata.
- Deterministic session context summaries from current routing mode, preferred agent, memory
  affinity, and orchestration snapshot summary.
- Read-only controller/view-model exposure and minimal Dashboard/Settings visibility.
- Tests for session defaults, context metadata, routing updates, QML-safe exposure, and separation
  from `ChatSession` and Phase 4 `RuntimeSession`.

Still out of scope:

- Provider/model execution, streaming, networking/API keys, model downloads, filesystem or system
  actions, real tool execution, plugin loading, embeddings, vector search, semantic search,
  autonomous workers, and multi-conversation persistence.

## Phase 6.9: Conversation State Graph Skeleton

Completed. Added a deterministic metadata-only conversation state graph for high-level
interaction flow.

Delivered:

- Value-only conversation states, transitions, transition results, transition status, graph
  interface, and static deterministic graph implementation.
- Safe transition rules from Idle through Listening, Planning, Routing, response readiness,
  response completion, approval-wait metadata, and Error.
- Invalid transition rejection with deterministic summaries.
- Read-only controller/view-model exposure and minimal Dashboard/Settings visibility.
- Tests for valid/invalid transitions, error transitions, QML-safe exposure, and separation from
  `ConversationSession`, `ChatSession`, and Phase 4 `RuntimeSession`.

Still out of scope:

- Provider/model execution, streaming, networking/API keys, model downloads, filesystem or system
  actions, real tool execution, approval actions, plugin loading, embeddings, vector search,
  semantic search, autonomous workers, and multi-conversation persistence.

## Phase 6.10: Pre-runtime Architecture Checkpoint

Completed. Checkpointed the Phase 6 metadata orchestration foundation before Phase 7.

Delivered:

- `docs/PHASE_6_CHECKPOINT.md` with completed scope, architecture findings, known limitations,
  Phase 7 readiness criteria, strict out-of-scope list, and recommended Phase 7 breakdown.
- Review across provider catalog, model router, task planner, agent registry, memory taxonomy,
  orchestration snapshot, diagnostics/readiness, conversation session, conversation state graph,
  controller ownership, and QML boundary.
- Phase/status/context docs updated to mark Phase 6 as checkpointed.
- Phase 7.0 defined as local runtime boundary planning/implementation, not full model execution
  unless explicitly scoped later.

Still out of scope:

- Provider/model execution, streaming, networking/API keys, model downloads, filesystem or system
  actions, real tool execution, approval actions, plugin loading, embeddings, vector search,
  semantic search, autonomous workers, and broad UI redesign.

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
- Dock hover/focus polish.
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
- Adaptive shell margins, dock sizing, header/status behavior, and page spacing.
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

### Phase 5.4: Workspace UX Integration

Completed. Used the former `lovable-tasarim` design reference only long enough to translate its
workspace direction into native Qt/QML.

Delivered:

- Left navigation/status rail, central Sentinel presence workspace, right chat panel, and compact
  top status composition.
- Centralized colors, spacing, radius, opacity, border, glow, and animation tokens.
- Mode-aware visual treatment for Companion, Focus, Mission, System, Minimal, and Tactical modes.
- Lightweight QML atmosphere and presence motion without web, heavy rendering, or runtime behavior
  changes.

Still out of scope:

- React/Vite/Tailwind/Node/WebView integration.
- Real tool execution, approval actions, sandbox runtime, automation, voice, hardware, provider
  execution, networking/API keys, plugin loading, advanced particle systems, or Qt Quick 3D.

### Phase 5.4.5: Architecture and UI Risk Audit

Stabilization checkpoint before Phase 5.5.

Focus:

- Re-audit core/view-model/persistence/provider-agent/tool-boundary separation after the Phase 5.4
  workspace changes.
- Re-audit QML component structure, design-token usage, mode-aware visuals, responsive behavior,
  and manual QA expectations.
- Fix only small safe documentation, naming, duplicated styling, binding, or checklist issues.

Still out of scope:

- New product features, UI redesign, provider/model execution, real tools, approval controls,
  sandbox runtime, automation, voice, hardware, plugin loading, networking/API keys, filesystem or
  system actions, React/WebView/Node/Tailwind/Vite integration, assistant-face rendering, advanced
  particles, heavy motion, or Qt Quick 3D.

### Phase 5.5: Visual Identity Reconstruction

Completed. Rebuilt the native QML shell around the translated Sentinel visual identity.

Delivered:

- Presence-first Dashboard/Core composition with a larger central Sentinel orb scene.
- Floating bottom dock navigation, bottom-right settings command, and a softer right AI bridge surface.
- Reusable QML primitives for the dock, orb, and floating telemetry readouts.
- Expanded visual tokens for glass surfaces, ambient glow, spacing, and dock sizing.
- Reduced utility-dashboard density by moving runtime/status information into small scene readouts.

Still out of scope:

- React/Vite/Tailwind/Node/WebView integration.
- Provider/model execution, networking/API keys, real tool execution, approval actions, sandbox
  runtime, automation, voice, hardware, plugin loading, filesystem/system actions, assistant-face
  rendering, advanced particles, heavy custom rendering, or Qt Quick 3D.

## Phase 6: Functional Workspace And Model Orchestration

### Phase 6.0: Metadata-Only Model Routing Skeleton

Completed. Added a local-safe model/provider routing architecture without execution.

Delivered:

- `ModelDescriptor`, `ProviderDescriptor`, `ProviderCapabilityProfile`, `RoutingMode`,
  `TaskType`, task classification, and route result metadata.
- `IModelRouter` boundary and deterministic `StaticModelRouter`.
- Minimal controller/view-model read-only routing status and selected model/provider summary.
- Tests for descriptor metadata, deterministic local routing, local-only behavior, unknown task
  fallback, and QML-safe exposure.

Still out of scope:

- Provider integrations, Ollama/OpenAI/Anthropic calls, API keys, networking, model downloads,
  model execution, model-management UI, real tool execution, plugin loading, filesystem/system
  actions, or broad UI redesign.

### Phase 6.1: Routing Mode Settings and Persistence

Completed. Persisted routing mode preference as local settings metadata.

Delivered:

- `AppSettings` routing mode preference with `Local Only` default.
- JSON-backed persistence through the existing settings store.
- Mutable metadata routing mode in `StaticModelRouter`.
- Controller/view-model route metadata refresh when routing mode changes.
- Minimal Settings page selector for Auto, Fast, Balanced, Quality, Local Only, Cloud Allowed, and
  Battery Saver.
- Tests for defaults, persistence, invalid fallback, route summary updates, and QML-safe exposure.

Still out of scope:

- Provider setup, credentials/API keys, networking, Ollama/OpenAI/Anthropic calls, model downloads,
  model execution, model-management UI, real tool execution, plugin loading, filesystem/system
  actions, or broad UI redesign.

### Phase 6.2: Provider Catalog Metadata Skeleton

Completed. Added the first provider/model catalog boundary without provider integration.

Delivered:

- `IProviderCatalog`, provider/model catalog entry metadata, and deterministic
  `StaticProviderCatalog`.
- Available local metadata placeholder plus not-configured placeholders for Ollama Local,
  OpenAI Cloud, and Anthropic Cloud.
- Read-only controller/view-model and Settings page catalog summaries.
- Router seeding from available catalog metadata while cloud placeholders remain unavailable and
  non-routable.

Still out of scope:

- Provider setup, credentials/API keys, endpoints, networking, provider calls, model downloads,
  model execution, model-management actions, real tool execution, plugin loading, filesystem/system
  actions, or broad UI redesign.

### Phase 6.3: Capability Graph and Task Planner Skeleton

Completed. Added metadata-only capability graph and task planning boundaries.

Delivered:

- `CapabilityNode`, `CapabilityGraph`, `TaskPlan`, `PlannedTaskStep`, and `TaskPlanStatus`.
- `ITaskPlanner` and deterministic `StaticTaskPlanner`.
- Planner metadata over task type, routing mode, provider/model catalog availability, local/cloud
  classification, privacy sensitivity, and resource hints.
- Read-only controller/view-model and Settings page task plan status, summary, and step count.
- Tests for deterministic planning, privacy-local behavior, cloud-unavailable fallback, unknown
  fallback, unavailable cloud blocking, and step ordering.

Still out of scope:

- Provider setup, credentials/API keys, endpoints, networking, provider calls, model downloads,
  model execution, tool execution, plugin loading, filesystem/system actions, or broad UI redesign.

### Phase 6.4: Agent System Skeleton

Completed. Added static agent metadata and registry boundaries without autonomous behavior.

Delivered:

- `AgentDescriptor`, role/state/priority enums, capability profile metadata, and task affinity
  metadata.
- `IAgentRegistry` and deterministic `StaticAgentRegistry`.
- Static descriptors for Atlas, Orin, Vela, Kaze, Nyx, and Sol.
- Planner annotation with preferred agent metadata where task affinity is available.
- Read-only controller/view-model and QML exposure for registered agent count, active agent
  summaries, and current preferred agent summary.
- Tests for registry determinism, unique ids, task mapping, planner interaction, and QML-safe
  exposure.

Still out of scope:

- Real agent execution, autonomous loops, threads/background workers, provider integrations,
  credentials/API keys, networking, downloads, model execution, tool execution, plugin loading,
  filesystem/system actions, or broad UI redesign.

Later Phase 6 work may revisit permission prompts, auditability, sandboxing strategy, secret
handling, and safe local automation constraints only after explicit approval.

### Phase 6.5: Memory Taxonomy and Semantic Metadata Skeleton

Completed. Added static memory taxonomy metadata without semantic memory execution.

Delivered:

- `MemoryType`, memory shard status, retention, privacy, recall hint, affinity, and association
  metadata.
- `IMemoryCatalog` and deterministic `StaticMemoryCatalog`.
- Static categories for Episodic, Semantic, Procedural, Reflective, and Ambient memory.
- Planner annotation with preferred memory affinity metadata.
- Read-only controller/view-model and QML exposure for memory category count, taxonomy summaries,
  and current memory affinity summary.
- Tests for catalog determinism, unique ids/types, retention/privacy preservation, planner
  affinity metadata, and QML-safe exposure.

Still out of scope:

- Vector databases, embeddings, semantic search, autonomous memory writes, provider/model
  execution, networking/API keys, downloads, tool execution, plugin loading, filesystem/system
  actions, or replacing `IMemoryStore`/`SQLiteMemoryStore` key-value persistence.

### Phase 6.6: Orchestration Snapshot and Workspace State Skeleton

Completed. Added a deterministic metadata aggregation read model without execution behavior.

Delivered:

- `OrchestrationSnapshot`, `WorkspaceStateSummary`, `OrchestrationHealthStatus`, and
  `OrchestrationSignal` value metadata.
- Snapshot aggregation over current routing mode/status, selected provider/model summary, task plan
  status/summary, preferred agent summary, memory affinity summary, provider catalog count, agent
  count, memory taxonomy count, runtime context status, and activity metadata.
- Read-only controller/view-model exposure for snapshot status, summary, and signal strings.
- Minimal Dashboard snapshot visibility using existing shell panel patterns.
- Tests for deterministic snapshot construction, routing-mode updates, included provider/agent/
  memory counts, preferred agent/memory/task summaries, and QML-safe exposure.

Still out of scope:

- Real provider integration, Ollama/OpenAI/Anthropic calls, API keys, networking, model downloads,
  model execution, tool execution, filesystem/system actions, plugin loading, vector databases,
  embeddings, semantic search, autonomous background workers, timers, or broad UI redesign.

## Phase 7.0: Local Runtime Boundary Skeleton

Completed. Added the future local runtime ownership boundary without treating Phase 7.0 as full
model execution.

Delivered:

- `ILocalRuntime` boundary for future local inference/runtime ownership.
- Value-only local runtime descriptor, status, health, capability, request, and response metadata.
- `NullLocalRuntime` deterministic metadata and placeholder refusal behavior.
- Read-only controller/view-model exposure and minimal Settings visibility.
- Tests for metadata determinism, safe refusal, controller exposure, and QML-safe view-model
  properties.

Still out of scope:

- Cloud provider integration, API keys, networking, model downloads, model execution, streaming,
  real tool execution, plugin loading, vector databases, embeddings, semantic search, autonomous
  workers, filesystem/system actions, or broad UI redesign.

## Phase 7.1: Local Runtime Session Ownership Skeleton

Completed. Added deterministic local runtime session ownership/lifecycle metadata without real
runtime execution.

Delivered:

- `LocalRuntimeSession`, session id/status/health metadata, allocation metadata, and reservation
  metadata.
- `ILocalRuntimeSessionManager` and `NullLocalRuntimeSessionManager`.
- One deterministic placeholder reserved session for read-only visibility.
- Controller/view-model QML-safe session status, health, count, summary, allocation, reservation,
  and summary-list exposure.
- Tests for lifecycle names, deterministic placeholder metadata, summary ordering, and QML-safe
  exposure.

Still out of scope:

- Provider calls, API keys, networking, downloads, model execution, streaming, process/subprocess
  launch, filesystem/system actions, tools/plugins, autonomous workers, or broad UI redesign.

## Phase 7.2: Runtime Capability Negotiation Layer

Completed. Added deterministic runtime capability negotiation metadata without enabling
capability activation or runtime execution.

Delivered:

- `RuntimeCapabilityDescriptor`, capability state/group metadata, negotiation profile, and
  negotiation result values.
- `IRuntimeCapabilityRegistry` and `StaticRuntimeCapabilityRegistry`.
- Static descriptors for future local inference, streaming, multimodal, embeddings, semantic
  memory, tool bridge, plugin bridge, memory binding, filesystem access, external process
  execution, cloud relay support, local-only enforcement, and privacy-safe mode.
- Read-only controller/view-model exposure for capability counts, enabled/disabled summaries,
  negotiation summaries, and local-only enforcement metadata.
- Minimal Settings visibility without toggles, activation actions, setup flows, downloads, or
  execution controls.
- Tests for deterministic metadata, disabled/unavailable handling, local-only enforcement, and
  QML-safe exposure.

Still out of scope:

- Capability activation, provider calls, API keys, networking, downloads, model execution,
  streaming, process/subprocess launch, filesystem/system actions, tools/plugins, autonomous
  workers, or broad UI redesign.

## Phase 7.3: Runtime Permission Metadata Skeleton

Completed. Added runtime permission metadata boundaries (`RuntimePermission`,
`RuntimePermissionLevel`, `RuntimePermissionDecision`, `RuntimePermissionRequest`) plus
`IRuntimePermissionPolicy` and `StaticRuntimePermissionPolicy`.

Delivered:

- Deterministic default-deny runtime permission policy for execution-level requests.
- Read-only controller/view-model permission decision and summary exposure.
- Tests for deterministic naming and default-deny policy behavior.

Still out of scope:

- Any runtime execution, provider/model calls, process/tool/plugin execution, filesystem/system
  actions, networking/API keys, or approval/setup UI.

## Phase 7.4: Runtime Request Pipeline Skeleton

Completed. Added metadata-only runtime request pipeline boundaries (`RuntimePipelineRequest`,
`RuntimePipelineStage`, `RuntimePipelineResult`, `RuntimePipelineStatus`, `RuntimePipelineTrace`)
plus `IRuntimePipeline` and `StaticRuntimePipeline`.

Delivered:

- Ordered deterministic pipeline traces for request, permission, safety, and blocked execution
  boundary stages.
- Read-only controller/view-model pipeline status/summary/trace exposure.
- Tests for deterministic stage naming, ordered traces, and blocked execution metadata.

Still out of scope:

- Provider/model runtime execution, process launch, networking, filesystem/system actions, or real
  tool/plugin execution.

## Phase 7.5: Runtime Safety Policy Skeleton

Completed. Added runtime safety posture metadata (`RuntimeSafetyPolicy`, `RuntimeSafetyRule`,
`RuntimeSafetyDecision`, `RuntimeSafetyReport`) plus `IRuntimeSafetyPolicy` and
`StaticRuntimeSafetyPolicy`.

Delivered:

- Deterministic local-only and no-execution safety report metadata.
- Read-only settings/dashboard-adjacent runtime safety visibility through existing view-model
  boundaries.
- Tests for deterministic report output and QML-safe exposure through controller/view-model.

Still out of scope:

- Sandbox runtime enforcement, provider/model/tool execution, process launch, networking, and
  filesystem/system actions.

## Phase 7.6: Runtime Architecture Checkpoint and Cleanup

Completed. Closed the Phase 7 metadata-only local runtime foundation before Phase 8.

Delivered:

- `docs/PHASE_7_CHECKPOINT.md` covering completed Phase 7 scope, known limitations, runtime
  guardrails, Phase 8 readiness criteria, and strict out-of-scope work.
- Architecture/status/roadmap/decision/context updates for completed Phase 7.3 through Phase 7.6
  work.
- Small QML read-only exposure consistency fix for the accepted page list.
- Focused desktop view-model test update for the runtime/navigation boundary.

Still out of scope:

- Real provider/model execution, networking/API keys, downloads, streaming, process/subprocess
  launch, filesystem/system actions, real tools, plugin loading, sandbox runtime enforcement,
  embeddings/vector DB/semantic search, autonomous workers, or execution/setup UI.

## Phase 8.0-8.2: Execution Lifecycle And Session Coordination

Completed. Added metadata-only execution lifecycle/session coordination for future runtime
integration while keeping execution disabled.

Delivered:

- Execution request/intent/priority and lifecycle state/status/result/trace metadata.
- Execution session id/status/ownership/coordination-mode metadata.
- `IExecutionLifecycle` with deterministic `StaticExecutionLifecycle`.
- `ExecutionCoordinator` and read-only `ExecutionCoordinationSnapshot`.
- Ordered lifecycle traces ending in blocked/non-executable state.
- Invalid transition rejection.
- Read-only controller/view-model Dashboard and Settings exposure.
- Tests for deterministic lifecycle behavior, blocked-by-default posture, transition validation,
  session ownership metadata, read-only coordination snapshots, and QML-safe exposure.

Still out of scope:

- Provider/model calls, Ollama launch, networking/API keys, downloads, streaming, process launch,
  filesystem/system actions, tools/plugins, autonomous workers, timers/background loops, execution
  controls, or setup UI.

## Phase 8.3-8.5: Local Runtime Adapter / Provider Bridge Readiness

Completed. Prepared the architecture for future Ollama/local runtime integration with metadata-only
adapter, bridge, and readiness boundaries.

Delivered:

- `ILocalRuntimeAdapter` and deterministic `StaticLocalRuntimeAdapter` placeholder metadata.
- Provider bridge boundary with not-connected and not-executable request/response metadata.
- Runtime integration readiness report with ordered checks for adapter contract, endpoint
  configuration, model discovery, provider bridge connection, and execution permission.
- Read-only controller/view-model Dashboard and Settings exposure.
- Tests for adapter metadata, bridge not-connected behavior, readiness ordering, controller
  exposure, and QML-safe view-model properties.

Still out of scope:

- Ollama/provider calls, endpoint setup fields, networking/API keys, downloads, streaming,
  process/subprocess launch, filesystem/system actions, model discovery, model execution,
  tools/plugins, embeddings/vector DB, autonomous workers, and model selection UI.

## Phase 9.0-9.2: Ollama Local Health And Discovery Boundary

Completed. Added the first controlled Ollama/local integration surface while keeping chat
inference and execution disabled.

Delivered:

- Ollama endpoint/config/status/model metadata.
- `IOllamaRuntimeClient`, deterministic `NullOllamaRuntimeClient`, and injectable
  `OllamaHttpRuntimeClient`.
- Safe default endpoint: `http://127.0.0.1:11434`.
- Loopback-HTTP-only endpoint normalization with fallback to the safe default.
- `AppSettings` persistence for normalized endpoint only; no API keys or cloud endpoint default.
- Health boundary limited to local `/api/version`.
- Optional installed model discovery limited to local `/api/tags` read-only metadata.
- Read-only controller/view-model exposure for endpoint, health, connection, model count, and model
  summaries.
- Settings/Dashboard local status visibility without setup or execution controls.

Still out of scope:

- Prompt execution, model generation, streaming, downloads/pulls/deletes/runs, subprocess/process
  launch, cloud calls, API keys, tool/plugin execution, filesystem/system scans/actions, background
  workers, model selection UI, and routing chat requests to Ollama.

## Phase 9.3-9.5: Controlled Local Inference Boundary

Completed. Adds the first explicit local-only inference path through `ILocalInferenceClient`.
Ollama generation is restricted to local loopback HTTP, non-streaming `/api/generate`, installed
model checks, and controller permission/safety gates. Streaming, model management, tools/plugins,
cloud endpoints, subprocess launch, filesystem/system actions, autonomous loops, and chat routing
automation remain out of scope.

## Phase 15.1-15.3: Voice Path Setup Refinement

Completed. Refined local Piper/Whisper path setup without enabling voice execution.

Delivered:

- Explicit Apply Paths action for Voice Configuration.
- Shorter path labels and user-facing help for Piper binary, Piper `.onnx` model, Whisper binary,
  and Whisper model folder or file.
- Exact configured-path validation rows for exists/readable/executable metadata.
- Ready/Blocked/Missing status for Piper controlled file-output TTS preparation.
- Ready/Blocked/Missing status for future Whisper STT preparation.
- Focused controller/view-model tests for persistence, fake valid files, missing paths,
  executable versus non-executable binaries, QML-safe exposure, and no execution side effects.

Still out of scope:

- Piper execution, Whisper execution, microphone access, playback, downloads, filesystem-wide
  scans, cloud/API keys, autonomous voice loops, and voice action controls.

## Phase 15.4-15.6: Controlled Piper File-Output Execution

Completed. Added explicit, policy-gated Piper TTS file generation to an app-controlled cache/temp
path.

Delivered:

- Persisted Piper file-output execution opt-in, disabled by default.
- Explicit Generate TTS File action gated by the opt-in, configured executable Piper binary,
  configured readable `.onnx` model, local-only request, and provider safety policy.
- Controlled output path generation inside the app cache/temp directory only.
- Execution status metadata for disabled, blocked/safety-blocked, missing binary, missing model,
  running, succeeded, failed, and timeout states.
- Generated audio path summary metadata after success without adding playback.
- Deterministic fake-client tests for success, failure, timeout, opt-in gating, invalid paths,
  controlled output metadata, QML-safe exposure, and persistence.

Still out of scope:

- Audio playback, microphone access, Whisper execution, arbitrary output paths, downloads,
  filesystem-wide scans, cloud/API keys, autonomous voice loops, and background voice actions.

## Phase 15.7: Ollama Reliability And Runtime Stabilization

Completed. Stabilized the controlled local Ollama chat path before continuing voice/STT work.

Delivered:

- Explicit timeout metadata for Ollama health checks, model discovery, non-streaming generation,
  and streaming generation.
- Clear local inference error categories for Ollama not running, endpoint unreachable, no selected
  model, selected model missing, request timeout, malformed response, stream interruption,
  permission/safety block, and duplicate busy request.
- Controller cleanup that resets busy state after success/error/refusal/timeout and clears stale
  streaming preview text after failures.
- Duplicate chat sends are rejected while local inference is active without appending another user
  message.
- Chat UI disables send/input while inference is busy and surfaces a concise inference summary.
- Fake-client tests cover unavailable Ollama, timeout, malformed response, selected model missing,
  duplicate-send rejection, streaming interruption cleanup, busy reset after error, and one final
  assistant message.

Still out of scope:

- Cloud providers/API keys, model downloads/pulls/deletes, Ollama process management, tools,
  plugins, filesystem/system actions, autonomous loops, microphone access, playback, Piper
  changes, and Whisper execution.

## Phase 15.10: Persistent Conversation UX And Chat History Management

Completed. Improved the single-transcript chat history UX without adding a transcript browser or
thread system.

Delivered:

- Conversation history summary metadata for persisted/runtime-only status, message counts, last
  saved status, and last restored status.
- Compact Chat and Settings rows showing persistence state and transcript summary without SQLite
  details or debug text.
- Clear Chat confirmation copy now makes runtime reset, persisted-history clearing, streaming text
  reset, and settings/memory preservation explicit.
- Focused coverage for persisted summary, runtime-only fallback, clear behavior, streaming/request
  reset after clear, and no duplicate initial/system messages.

Still out of scope:

- Multi-conversation/thread management, transcript browser, export/import, encryption, pruning,
  transcript search, cloud/API keys, model management actions, tools/plugins, filesystem/system
  actions, and voice execution changes.

## Phase 16.0-16.6: Controlled Semantic Memory Foundation And Review Flow

Completed. Prepared semantic memory architecture with reviewable metadata-only candidates and a
guarded review lifecycle.

Delivered:

- `MemoryCandidate` value types and `IMemoryCandidateStore`.
- `InMemoryMemoryCandidateStore` for deterministic non-persistent candidate storage.
- `MemoryCandidateReviewResult` plus reviewed timestamp, reviewer/source summary, and decision
  reason metadata.
- Controller/view-model candidate ids, states, counts, state-filtered summaries, last review
  result, and QML-safe action exposure.
- Compact Memory page candidate section with Approve, Reject, and Reset controls.
- Tests for pending defaults, approve/reject/reset/archive review metadata, invalid transition
  refusal, deterministic counts/summaries, no key-value memory mutation, QML-safe exposure, and
  clear-chat preserving approved candidate metadata.

Still out of scope:

- Embeddings, vector DB, semantic search, automatic memory capture, autonomous long-term memory
  writes, cloud sync, provider/model calls, filesystem/system actions, tools/plugins, and durable
  candidate persistence.
- Approved candidates are reviewed metadata only; committing them into long-term memory remains out
  of scope.

## Phase 16.7-16.9: Approved Memory Commit Planning

Completed. Added commit-planning metadata for reviewed memory candidates while keeping actual
memory mutation disabled.

Delivered:

- `MemoryCommitPlan`, `MemoryCommitTarget`, `MemoryCommitReadiness`, `MemoryCommitResult`, and
  `MemoryCommitPolicy`.
- Deterministic approved-candidate to key-value-memory plan summaries.
- Readiness reasons for pending, rejected, archived, missing, store-unavailable, and
  policy-disabled commit states.
- Controller/view-model exposure for commit readiness status, checks, plan count, target summary,
  per-candidate plan summaries, and last commit result.
- Compact Memory page readiness visibility that states Approved is not Committed and commit is
  future-gated.
- Tests proving default commit requests refuse safely and do not mutate key-value memory.

Still out of scope:

- Actual commit execution, automatic memory writes, semantic memory persistence, embeddings, vector
  DB, semantic search, provider/model calls, cloud sync, filesystem/system actions, tools/plugins,
  and durable candidate storage.

## Phase 16.10-16.12: Explicit Memory Commit Boundary

Completed. Enabled an explicit user-controlled Commit action from approved memory candidates into
the existing local key-value memory store.

Delivered:

- `MemoryCommitStatus`, `MemoryCommitConflictPolicy`, committed timestamp/key summaries, and
  accepted/refused `MemoryCommitResult` metadata.
- Sanitized deterministic commit keys derived from candidate category, title, and id.
- Commit execution only for Approved candidates, only through the explicit user action, and only
  when the key-value memory store is available.
- Commit values store reviewed candidate content only; source/review metadata remains in result
  and committed-candidate summaries because `IMemoryStore` has no metadata fields.
- Default duplicate-key conflict policy refuses existing keys. Overwrite remains unavailable.
- Approval remains review-only and never commits automatically.
- Memory page shows Commit only for approved candidates and keeps Approve/Commit labels distinct.
- Tests cover successful approved commit, non-approved refusal, duplicate refusal, no auto-commit,
  committed status/result exposure, clear-chat preserving committed memory, and view-model exposure.

Still out of scope:

- Embeddings, vector DB, semantic search, provider/model calls, cloud/API keys, tools/plugins,
  filesystem/system actions beyond the existing memory store, autonomous memory mutation, overwrite
  UI, and durable candidate storage.

## Phase 16.13-16.15: Memory Recall Metadata And Local Memory Surfacing

Completed. Added a local read-only recall surface for committed key-value memory.

Delivered:

- `MemoryRecallQuery`, `MemoryRecallResult`, `MemoryRecallSummary`, `MemoryRecallStatus`, and
  `MemoryRecallPolicy`.
- Literal key/value matching over existing `IMemoryStore` entries only.
- Controller/view-model exposure for recall policy, query text, status, summary, result count,
  memory entry count, and compact result summaries.
- Memory page “Local Memory Recall” field and compact result display.
- Tests for committed memory recall, empty query behavior, no recall mutation, recall after commit,
  clear-chat preserving committed memory recall, view-model exposure, and no prompt injection side
  effects.

Still out of scope:

- Embeddings, vector DB, semantic search, provider/model calls, automatic prompt injection,
  ranking, automatic context assembly, cloud/API keys, tools/plugins, filesystem/system actions,
  and autonomous memory mutation.

## Phase 16.16-16.18: Context Assembly Planning Foundation

Completed. Added planning-only metadata for future context assembly.

Delivered:

- `ContextAssemblyRequest`, `ContextAssemblySource`, `ContextAssemblyResult`,
  `ContextAssemblyStatus`, `ContextAssemblyPolicy`, and `ContextAssemblySummary`.
- Deterministic source participation estimates for conversation context, committed memory context,
  runtime metadata context, and orchestration context.
- Candidate block counts and simple character-size estimates.
- Controller/view-model exposure through QML-safe strings, counts, and lists.
- Compact Memory page “Context Assembly” readiness section.
- Tests for deterministic summaries, empty context, committed memory visibility, no mutation, no
  prompt injection side effects, and view-model exposure.

Still out of scope:

- Prompt assembly, automatic context attachment, provider/model calls, semantic ranking,
  embeddings/vector databases, cloud/API keys, tools/plugins, filesystem/system actions, and broad
  UI redesign.

## Phase 16.19-16.21: Safe Prompt Context Injection Foundation

Completed. Added controlled prompt context injection for local Ollama requests.

Delivered:

- `PromptContextBlock`, `PromptContextBundle`, `PromptContextInjectionPolicy`,
  `PromptContextInjectionStatus`, and `PromptContextInjectionResult`.
- Disabled-by-default persisted setting for “Use local memory/context in chat”.
- Deterministic, clearly delimited prompt context prepended to local Ollama prompts only after
  existing model, endpoint, permission, safety, and busy gates pass.
- Allowed sources limited to current conversation context, committed key-value memory, runtime
  metadata summaries, and orchestration metadata summaries.
- Fixed character budget, deterministic source order, and deterministic truncation.
- QML-safe status, summary, block count, source summary, size summary, and block summaries without
  exposing the raw private prompt.
- Settings toggle and compact Chat context status.
- Tests for disabled default, deterministic enabled injection, committed-only memory inclusion,
  pending/rejected candidate exclusion, truncation, no mutation, safety-gate ordering, and
  view-model exposure.

Still out of scope:

- Embeddings, vector DB, semantic ranking/search, automatic memory writes, cloud/API keys,
  provider expansion, tools/plugins, filesystem/system actions, voice/runtime scope changes, broad
  UI redesign, and raw prompt display.

## Phase 16.22-16.24: Conversation Window Management Foundation

Completed. Added deterministic bounded conversation-history windowing for local prompt context.

Delivered:

- `ConversationWindowPolicy`, `ConversationWindowSummary`, `ConversationWindowResult`,
  `ConversationWindowStatus`, and `ConversationWindowBudget`.
- Recent-message-first window selection with stable chronological output order.
- Simple character budget enforcement with deterministic truncation and omitted/truncated counts.
- Prompt context assembly now injects only the bounded recent conversation window, keeping it
  clearly delimited from committed memory, runtime metadata, orchestration metadata, and the
  current user prompt.
- Controller/view-model exposure for QML-safe status, budget summary, included count, omitted
  count, and truncated count.
- Compact Chat and Settings window status without broad redesign.
- Tests for recent-message prioritization, budget enforcement, committed-memory separation, long
  chat prompt stability, no mutation, and view-model exposure.

Still out of scope:

- Semantic ranking/search, embeddings, vector DB, transcript summarization, provider/model
  changes, cloud/API keys, tools/plugins, filesystem/system actions, broad UI redesign, and raw
  prompt display.

## Phase 16.25-16.27: Deterministic Conversation Summary Foundation

Completed. Added deterministic local summaries for older history omitted by the bounded recent
conversation window.

Delivered:

- `ConversationSummaryPolicy`, `ConversationSummaryStatus`, `ConversationSummaryResult`,
  `ConversationSummaryBlock`, `ConversationSummaryWindow`, and `ConversationSummaryBudget`.
- Local heuristic summary blocks that preserve original message indexes, chronology, and role
  visibility.
- Separate prompt context source for older conversation summaries, distinct from recent history,
  committed memory, runtime metadata, and orchestration metadata.
- Bounded deterministic character budgeting with included size, block count, summarized/omitted
  message count, and truncated block count exposure.
- Compact Chat and Settings summary status without broad redesign.
- Tests for deterministic generation, chronological behavior, budget truncation, recent-window
  interaction, memory separation, no mutation, and controller/view-model exposure.

Still out of scope:

- Semantic summarization, embeddings, vector DB, semantic ranking/search, provider/model calls,
  cloud/API keys, tools/plugins, filesystem/system actions, automatic memory writes, broad UI
  redesign, and raw prompt display.

## Phase 16.28-16.30: Deterministic Retrieval Planning Foundation

Completed. Added deterministic retrieval planning across existing local context sources.

Delivered:

- `RetrievalPlanningPolicy`, `RetrievalPlanningStatus`, `RetrievalPlanningResult`,
  `RetrievalCandidate`, `RetrievalSourcePriority`, `RetrievalBudget`, and
  `RetrievalSelectionSummary`.
- Fixed source priority: recent conversation window, deterministic older summaries, committed
  key-value memory, runtime metadata, then orchestration metadata.
- Character-budget allocation with deterministic truncation and selected/excluded source and
  candidate counts.
- Prompt context injection now consumes selected retrieval candidates while preserving source
  separation and chronology where conversation blocks provide it.
- QML-safe controller/view-model exposure for retrieval readiness, status, budget summaries,
  source summaries, and selected/excluded counts.
- Compact Chat, Memory, and Settings retrieval status.
- Tests for source selection, budget allocation, truncation, chronology preservation,
  summary/memory separation, no mutation during planning, and view-model exposure.

Still out of scope:

- Semantic retrieval, vector databases, embeddings, semantic ranking/search, provider/model calls,
  cloud/API keys, automatic memory writes, tools/plugins, filesystem/system actions, debug console
  UI, broad redesign, raw prompt display, and private assembled payload display.

## Phase 16.31-16.33: Embedding And Vector Abstraction Foundation

Completed. Prepared semantic retrieval architecture without enabling semantic behavior.

Delivered:

- `EmbeddingVector`, `EmbeddingDocument`, `EmbeddingRequest`, `EmbeddingResult`,
  `EmbeddingProviderStatus`, `EmbeddingProviderPolicy`, `VectorIndexStatus`,
  `VectorIndexPolicy`, `VectorSearchQuery`, `VectorSearchResult`, `VectorSearchCandidate`,
  `SemanticRetrievalStatus`, and `SemanticRetrievalPolicy`.
- `IEmbeddingProvider` and `IVectorIndex` boundaries for future local/provider embedding systems
  and future vector database/index compatibility.
- `FakeEmbeddingProvider` and `FakeVectorIndex` for deterministic tests only.
- Stable fake vectors derived from local text hashing/token counting and deterministic in-memory
  vector search with stable scoring/tie ordering.
- QML-safe readiness metadata for semantic retrieval disabled state, embedding provider readiness,
  vector index readiness, indexed item count, summaries, and checks.
- Compact Memory and Settings semantic/vector readiness sections.
- Tests for fake embedding generation, stable vectors, vector insert/search/remove, deterministic
  scoring order, retrieval-planning non-regression, prompt non-mutation, and QML-safe exposure.

Still out of scope:

- Real embeddings, transformer inference, semantic ranking/search, semantic prompt injection,
  vector database integration, cloud/API keys, provider/model calls, filesystem writes, downloads,
  plugins/tools, system execution, debug UI, broad redesign, and runtime authority expansion.

## Phase 16.34-16.36: Semantic Candidate Orchestration Foundation

Completed. Prepared hybrid deterministic and future semantic retrieval orchestration metadata
without activating semantic retrieval.

Delivered:

- Value-only semantic candidate orchestration types for candidate sources, selections, budgets,
  windows, arbitration, summaries, status, policy, and hybrid retrieval readiness.
- Candidate-source coverage for recent conversation windows, deterministic summaries, committed
  key-value memory, runtime metadata, orchestration metadata, and disabled future semantic/vector
  candidates.
- Deterministic arbitration ordering and participation budgeting with exclusion/truncation
  metadata, source isolation, chronology guarantees for conversation-derived sources, and compact
  participation summaries.
- Hybrid readiness metadata that keeps deterministic retrieval authoritative and reports the
  semantic path disabled.
- QML-safe Memory and Settings visibility for candidate counts, arbitration/budget summaries,
  participation summaries, and hybrid retrieval readiness.
- Tests for lifecycle metadata, arbitration order, deterministic budgeting, source isolation,
  chronology preservation, retrieval-planning non-regression, prompt non-mutation, QML-safe
  exposure, and fake-only/no-real-provider behavior.

Still out of scope:

- Real embeddings, semantic ranking/search, vector DB activation, transformer inference,
  provider/model calls, cloud/API keys, semantic prompt injection, raw semantic/vector payload UI,
  plugins/tools, filesystem/system actions, broad redesign, and runtime authority expansion.

## Phase 16.37-16.39: Semantic Retrieval Activation Readiness Checkpoint

Completed. Closed Phase 16 with an architecture checkpoint before any real semantic retrieval
activation.

Delivered:

- Added `docs/PHASE_16_MEMORY_CONTEXT_CHECKPOINT.md`.
- Audited the complete memory/context/retrieval path from memory candidates through hybrid
  readiness metadata.
- Confirmed deterministic retrieval remains authoritative and semantic retrieval remains disabled.
- Confirmed semantic candidates do not mutate prompts and prompt context injection remains opt-in.
- Confirmed QML exposes only safe summaries, statuses, counts, and readiness checks.
- Confirmed existing QA coverage already covers the checkpoint guarantees, so no redundant tests
  were added.

Still out of scope:

- Real embeddings, vector database integration, semantic ranking/search activation, semantic
  prompt injection, provider/model calls, cloud/API keys, plugins/tools, filesystem/system actions,
  raw vector/score/prompt UI, broad redesign, and runtime authority expansion.

## Later Phase 7: Packaging / Ecosystem / Extensions

Prepare packaging, update channels, plugin/extension lifecycle, platform-specific integration packages, and distribution workflows.
