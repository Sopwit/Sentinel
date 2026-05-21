# Phase Status

## Completed / Stable

### Phase 18.28-18.30: Controlled Audio-File Session Foundation

Completed. Adds deterministic metadata-only audio-file session modeling for future controlled
offline STT ingestion without enabling file loading, decoding, transcription, playback, or
execution.

Scope:

- Added value-only audio file session id/status/policy/result/readiness/safety/fallback/summary,
  descriptor, validation/status/restriction/budget, and trace metadata.
- Supported future extension metadata is deterministic and ordered for wav, mp3, flac, and ogg.
- Validation metadata reports local-only, supported/unsupported extension, empty-file,
  oversized-file, refused-path, sandbox-required, future-transcription-ready, and
  disabled-by-policy states.
- Unsafe or non-local path-style values are refused without exposing raw paths to QML.
- Empty, oversized, sandbox-required, and disabled states produce deterministic fallback metadata.
- Safety reports preserve `executionAttempted = false` and block file loading, waveform decoding,
  transcription, playback, microphone capture, subprocess execution, filesystem scanning,
  automatic ingestion, autonomous loops, and cloud/API calls.
- Controller, desktop view model, Settings, Agents, and the existing Chat status line expose only
  QML-safe status strings, summaries, counts, validation/refusal lists, supported-extension
  summaries, trace summaries, fallback summaries, and safety summaries.

Known limitation:

- This phase does not add upload controls, file pickers, real file reads, waveform decoding,
  Whisper execution, transcript creation, playback, microphone capture, filesystem scanning,
  subprocesses, cloud calls, or automatic ingestion. Future controlled transcription and live
  microphone phases must be scoped separately.

### Phase 18.25-18.27: Voice Pipeline Session Orchestration Foundation

Completed. Adds deterministic voice pipeline session orchestration metadata that composes Whisper
STT readiness, local chat inference readiness, and Piper TTS readiness without enabling audio or
runtime execution.

Scope:

- Added value-only voice pipeline session id/status/policy/result/step/trace/budget/readiness/
  safety/fallback/summary metadata.
- The session lifecycle is deterministic: prepare, await audio input, transcription readiness,
  chat inference readiness, synthesis readiness, then completion or refusal/fallback metadata.
- Missing Whisper readiness blocks the transcription stage. Missing or blocked local chat/model
  readiness blocks inference. Missing Piper readiness blocks synthesis.
- Unsafe or unavailable stages become refused/fallback metadata with no side effects.
- Safety reports preserve `executionAttempted = false` and block microphone capture, playback,
  Whisper execution, Piper execution, subprocess execution, voice chat auto-send, transcript
  injection, background workers, and autonomous loops.
- Controller, desktop view model, Settings, Agents, and the existing Chat status line expose only
  QML-safe strings, counts, and string lists for session status, stage readiness, traces,
  fallback, safety, and current session summary.

Known limitation:

- This phase does not add microphone capture, audio-file STT, chat auto-send from voice, Piper
  synthesis, audio generation, playback, subprocess execution, background workers, or voice
  activation controls. Future controlled audio-file STT, controlled synthesis, and live voice
  phases must be scoped separately.

### Phase 18.22-18.24: Piper TTS Local Runtime Foundation

Completed. Adds a readiness-first Piper synthesis boundary for future local text-to-speech
without enabling playback, live voice streaming, audio-file generation, or Piper subprocess
execution.

Historical scope:

- Added Piper synthesis policy/status/request/result/session/budget/readiness/safety/fallback/
  trace metadata plus `IPiperSynthesisClient`.
- Added `NullPiperSynthesisClient` and a bounded `LocalPiperSynthesisClient` skeleton that
  validates binary/model/text metadata and refuses before execution.
- Default behavior is disabled. Missing binary, missing model, unsafe/non-local path-style
  configuration, invalid timeout budget, empty text, and runtime privilege requests produce
  deterministic refusal/fallback metadata.
- Safety reports keep `executionAttempted = false` and block subprocess execution, playback,
  live voice streaming, microphone capture, cloud calls, downloads, filesystem scanning, and
  automatic chat/audio injection.
- The legacy Piper file-output surface is now compatibility/readiness metadata only; persisted
  file-output opt-in is reset to disabled and generation requests refuse without reaching a
  client or writing audio.
- Controller, desktop view model, Settings, and Agents expose QML-safe Piper synthesis status,
  readiness, last-result, fallback, safety, and trace summaries only.

Known limitation:

- The boundary does not synthesize or play audio yet. A future controlled synthesis phase must
  explicitly enable bounded execution gates, and any later playback/audio-device phase must
  separately define device permissions, lifecycle, UI controls, and tests.

### Phase 18.19-18.21: Whisper STT Local Runtime Foundation

Completed. Adds a controlled Whisper speech-to-text runtime boundary for future local audio-file
transcription without enabling microphone capture, live recording, playback, streaming STT, or
Whisper subprocess execution.

Scope:

- Added Whisper transcription policy/status/request/result/session/budget/readiness/safety/
  fallback/trace metadata plus `IWhisperTranscriptionClient`.
- Added `NullWhisperTranscriptionClient` and a bounded `LocalWhisperTranscriptionClient` skeleton
  that validates metadata and refuses before execution.
- Default behavior is disabled. Missing binary, missing model, missing audio, unsafe/non-local
  path-style input, invalid timeout budget, and runtime privilege requests produce deterministic
  refusal/fallback metadata.
- Safety reports keep `executionAttempted = false` and block subprocess execution, microphone
  capture, playback, streaming, cloud calls, downloads, filesystem scanning, prompt injection, and
  automatic chat send.
- Controller, desktop view model, Settings, and Agents expose QML-safe Whisper STT status,
  readiness, last-result, fallback, safety, and trace summaries only.

Known limitation:

- The boundary does not transcribe audio yet. A future controlled audio-file transcription phase
  must explicitly enable execution gates, and any later microphone/live STT phase must separately
  define capture permissions, lifecycle, UI controls, and tests.

### Phase 18.16-18.18: Voice Runtime Permission And Path Configuration Foundation

Completed. Adds deterministic local-only voice runtime readiness metadata for future Piper and
Whisper activation without enabling voice inference or audio I/O.

Scope:

- Added voice runtime policy/status/readiness/health/sandbox/restriction/budget/readiness and
  safety-report metadata beside the existing voice provider and environment boundaries.
- Added Piper and Whisper runtime descriptors, status/readiness records, configuration summaries,
  and path summaries that report configured/missing/refused counts without exposing raw paths in
  readiness summaries.
- Unsafe or non-local path-style configuration is refused before activation metadata is marked
  ready.
- Missing or partial Piper/Whisper configuration produces safe missing-configuration readiness.
- Permission summaries explicitly label local-only, disabled-by-default, sandbox-required, future
  microphone access, future audio playback, future transcription runtime, and future synthesis
  runtime as metadata only.
- Safety summaries preserve `executionAttempted = false` and state that no subprocess execution,
  inference, microphone capture, playback, streaming, filesystem scanning, downloads, cloud/API
  calls, or background workers are started.
- Controller, desktop view model, Settings, and Agents expose compact QML-safe readiness,
  configured/missing/refused counts, permission summaries, sandbox summaries, and runtime safety
  summaries.

Known limitation:

- The readiness layer is not a Piper/Whisper launcher, model loader, microphone permission flow,
  playback system, filesystem scanner, path picker, downloader, sandbox implementation, or voice
  activation control. Future STT/TTS activation still requires a separate explicit phase.

### Phase 18.13-18.15: Tool Contract And Permission Foundation

Completed. Adds deterministic metadata-only tool contract records beside the Phase 18 capability
registry without enabling any tool runtime or execution authority.

Scope:

- Added `ToolContract`, `ToolContractId`, `ToolContractType`, `ToolContractStatus`,
  `ToolContractScope`, `ToolContractPolicy`, `ToolContractPermission`, `ToolContractSandbox`,
  `ToolContractRestriction`, `ToolContractSafetyReport`, `ToolContractReadiness`,
  `ToolContractSummary`, `ToolContractRegistry`, `ToolContractRegistryStatus`, and
  `ToolContractRegistrySummary`.
- Static tool contracts are ordered deterministically and local-only where enabled.
- Enabled metadata contracts cover conversation summary, memory inspection, retrieval
  preparation, semantic supplement preparation, voice response preparation, and export
  preparation.
- Permission metadata exposes local-only, read-only, approval-required, sandbox-required,
  disabled, refused, future filesystem access, future subprocess execution, future plugin
  runtime, and future export action labels.
- Future filesystem access, subprocess execution, plugin runtime, and export action contracts
  remain disabled or refused and expose safe restriction/refusal summaries only.
- Sandbox readiness is summarized as metadata. Required or denied sandbox states do not start
  filesystem, subprocess, plugin, export, or tool runtime behavior.
- `executionAttempted` remains false. The registry does not start tools, plugins, filesystem
  actions, subprocesses, background workers, cloud/API calls, provider/model calls, approval
  workflows, or autonomous loops.
- Controller, desktop view model, and Agents page expose QML-safe contract counts, summaries,
  permission summaries, sandbox summaries, readiness summaries, and safety summaries with no
  execute buttons or approval UI.

Known limitation:

- Tool contracts are not permission grants, executors, plugin hosts, filesystem adapters,
  subprocess boundaries, approval workflows, or sandbox implementations. Future activation
  requires a separate explicit phase.

### Phase 18.10-18.12: Agent Capability Registry Foundation

Completed. Adds deterministic agent capability-registry metadata beside the Phase 18 task queue
and planning-session boundary without enabling runtime execution.

Scope:

- Added `AgentCapability`, `AgentCapabilityId`, `AgentCapabilityType`,
  `AgentCapabilityStatus`, `AgentCapabilityScope`, `AgentCapabilityPolicy`,
  `AgentCapabilitySummary`, `AgentCapabilityRequirement`, `AgentCapabilityRestriction`,
  `AgentCapabilityReadiness`, `AgentCapabilitySafetyReport`, `AgentCapabilityRegistry`,
  `AgentCapabilityRegistryStatus`, and `AgentCapabilityRegistrySummary`.
- Static capability metadata is ordered deterministically and local-only.
- Enabled metadata capabilities cover conversation summarization, memory inspection, retrieval
  preparation, semantic supplement preparation, export preparation, and voice response
  preparation.
- Future filesystem access, shell execution, and plugin runtime capabilities remain disabled or
  refused and expose safe restriction/refusal summaries only.
- `executionAttempted` remains false. The registry does not start tools, plugins, filesystem
  actions, shell/subprocess execution, background workers, cloud/API calls, provider/model calls,
  or autonomous loops.
- Controller, desktop view model, and Agents page expose QML-safe capability counts, summaries,
  readiness summaries, and safety summaries with no enable/execute buttons or approval workflow.

Known limitation:

- The registry is not a runtime permission grant, executor, plugin host, filesystem adapter, or
  shell boundary. Future activation requires a separate explicit phase with permission, safety,
  approval, sandbox, and UI scope.

### Phase 18.7-18.9: Agent Planning Session And Safety Arbitration Foundation

Completed. Adds bounded agent planning-session and safety arbitration metadata on top of the
Phase 18 task queue without enabling execution.

Scope:

- Added `AgentPlanningSession`, `AgentPlanningSessionId`, `AgentPlanningSessionStatus`,
  `AgentPlanningSessionSummary`, `AgentPlanningSessionPolicy`, `AgentPlanningResult`,
  `AgentPlanningCandidate`, `AgentPlanningBudget`, `AgentPlanningArbitration`,
  `AgentPlanningRefusal`, `AgentPlanningSafetyReport`, and `AgentPlanningFallback`.
- Planning sessions derive deterministic local metadata from the ordered task queue and stay
  bounded by candidate, step, and summary budgets.
- Arbitration selects candidates by priority, queue sequence, and task id. Budget overflow uses a
  deterministic fallback summary.
- Unsafe planning candidates are refused as safe metadata with exposed refusal summaries.
- `executionAttempted` remains false. Planning does not start tools, plugins, filesystem actions,
  shell/subprocess execution, background workers, cloud/API calls, provider/model calls, or
  autonomous loops.
- Controller, desktop view model, and Agents page expose QML-safe planning status, counts,
  arbitration summaries, refusal summaries, and fallback summary.

Known limitation:

- Planning sessions are not an executor, scheduler, approval workflow, or tool runtime. Future
  execution authority still requires a separate explicit phase.

### Phase 18.4-18.6: Agent Task Queue And Lifecycle Metadata

Completed. Adds deterministic queue and lifecycle metadata on top of the Phase 18 agent task
runtime foundation without enabling execution.

Scope:

- Added `AgentTaskQueue`, `AgentTaskQueueStatus`, `AgentTaskQueueSummary`,
  `AgentTaskLifecycle`, `AgentTaskLifecycleEvent`, `AgentTaskQueuePolicy`, and
  `AgentTaskQueueResult`.
- Static agent task runtime now keeps a deterministic in-memory metadata queue ordered by
  priority, queue sequence, and task id.
- Tasks can be queued, listed, marked planned, blocked, completed as metadata, or refused. Every
  lifecycle transition records ordered metadata only.
- Execution attempts continue to be refused safely and `executionAttempted` remains false.
- Controller, desktop view model, and Agents page expose queue count, planned/active/blocked/
  completed/refused counts, latest lifecycle summary, and QML-safe task summaries.

Known limitation:

- The queue has no real execution loop, worker, scheduler, tool/plugin authority, filesystem or
  shell action, provider/model call, approval flow, or autonomous behavior. Future execution
  requires a separate explicit phase.

### Phase 18.0-18.3: Agent Task Runtime Foundation

Completed. Adds the agent task runtime foundation as deterministic metadata/readiness
infrastructure without granting execution authority.

Scope:

- Added `AgentTask`, `AgentTaskId`, task type/status/priority/source enums, task plans, steps,
  results, traces, safety policy, and runtime status records.
- Added `IAgentTaskRuntime` and `StaticAgentTaskRuntime`.
- The static runtime seeds deterministic metadata tasks for summarizing conversation, inspecting
  memory status, planning response, preparing retrieval context, preparing voice response, and
  preparing export action.
- Runtime results always refuse execution and record ordered trace summaries at the execution
  boundary.
- Controller and desktop view model expose only QML-safe active status, task count, latest task
  summary, runtime summary, and latest trace summaries.
- Agents page shows compact read-only task runtime readiness/status with no execution, approval,
  tool, plugin, filesystem, shell, or cloud controls.

Known limitation:

- Phase 18 starts from the documented clang-tidy-clean Phase 17.31-17.33 checkpoint and preserves
  the no-execution posture. Future tool/task execution requires a separate explicit runtime phase
  with permission, safety, approval, and UI scope.

### Phase 17.31-17.33: Semantic Retrieval And Prompt Inclusion Checkpoint

Completed. Checkpoints the full Phase 17 semantic architecture after controlled prompt inclusion
without expanding runtime authority.

Scope:

- Added `docs/PHASE_17_SEMANTIC_CHECKPOINT.md`.
- Audited semantic provider planning, isolated embedding runtime metadata, vector persistence,
  bounded semantic search, hybrid bridge arbitration, semantic acceptance, supplement assembly,
  prompt authority policy, controlled inclusion, AppSettings persistence, controller prompt
  assembly, desktop view-model exposure, and Memory/Settings status surfaces.
- Confirmed deterministic retrieval remains the final prompt authority.
- Confirmed semantic inclusion is disabled by default and explicit opt-in.
- Confirmed semantic supplements are local-only, bounded, policy-gated, supplemental, clearly
  delimited, and appended only after deterministic context when every safety gate passes.
- Confirmed disabled/denied/unsafe/empty/stale/busy/timed-out/refused semantic states fall back to
  deterministic-only prompts.
- Confirmed QML exposure remains compact status/count/summary/check metadata only, with no raw
  prompts, raw supplement blocks, vectors, scores, provider handles, filesystem paths, or debug
  payloads.
- Confirmed no filesystem indexing, cloud/API/vector provider activation, tools/plugins,
  autonomous actions, or prompt-authority expansion was introduced.

Known limitation:

- Phase 17 closes with semantic systems still bounded and default-disabled. Any Phase 18 semantic
  expansion must preserve deterministic fallback behavior, local-only/provider boundaries, and QML
  non-exposure guarantees or explicitly scope and test a policy change.

### Phase 17.28-17.30: Controlled Semantic Prompt Inclusion

Completed. Adds disabled-by-default live inclusion metadata and a narrow prompt assembly step that
can append semantic supplements only after deterministic context when every local safety and
authority gate passes.

Scope:

- Added semantic prompt inclusion policy, status, result, budget, safety-report, fallback, and
  audit-summary records.
- Inclusion remains disabled by default and falls back to deterministic-only prompt assembly unless
  context injection is enabled, semantic prompt authority approves, supplement assembly is bounded
  and safe, local-only mode is active, and the inclusion safety report passes.
- When included, semantic supplements are appended as a separate clearly delimited
  supplemental/non-authoritative block after deterministic context and before the user prompt.
- Deterministic retrieval ordering, committed memory authority, conversation windows, summaries,
  and runtime metadata remain unchanged and cannot be replaced, reordered, or overridden by
  semantic supplements.
- Memory and Settings expose compact inclusion enabled/status, included count, budget, fallback,
  audit, and deterministic-authority-preserved summaries without raw prompts, raw supplement
  blocks, vectors, scores, provider handles, filesystem paths, or debug dumps.

Known limitation:

- Desktop semantic search, assembly, authority, and inclusion policies still default to disabled,
  so default local prompt assembly remains deterministic-only. No filesystem indexing,
  cloud/API/vector provider, autonomous action, tool/plugin, or raw prompt exposure is added.

### Phase 17.25-17.27: Semantic Prompt Authority Policy Foundation

Completed. Adds the policy gate that evaluates whether bounded semantic supplement metadata would
ever be eligible for prompt inclusion, while keeping live semantic prompt injection disabled by
default.

Scope:

- Added semantic prompt authority policy, status, result, readiness, decision, safety-report,
  fallback, and audit-summary records.
- Authority evaluation reads `SemanticSupplementAssemblyResult` only and defaults to
  Disabled/Denied with deterministic-only fallback metadata.
- Inclusion can only reach a test-only "would include metadata" decision when semantic search is
  local-only, supplements came through deterministic acceptance, the bundle is bounded, prompt
  injection is explicitly enabled, the policy explicitly allows authority, and safety passes.
- Even in the allowed-readiness path, semantic supplements remain supplemental,
  non-authoritative, clearly delimited metadata. Live prompt mutation remains blocked.
- Memory and Settings expose compact status, decision, readiness, fallback, safety, and audit
  summaries without raw prompts, supplement blocks, vectors, scores, provider handles, filesystem
  paths, or debug dumps.

Known limitation:

- Semantic prompt authority still does not mutate live prompts. Default prompt assembly remains
  unchanged and deterministic retrieval remains authoritative. Future activation requires a
  separate phase with explicit live-inclusion scope, privacy/safety review, deterministic fallback
  tests, and continued QML non-exposure guarantees.

### Phase 17.22-17.24: Semantic Supplement Prompt Assembly Readiness

Completed. Adds a disabled-by-default semantic supplement assembly layer that can prepare accepted
semantic supplements as bounded metadata for future prompt assembly tests without changing live
prompt behavior.

Scope:

- Added semantic supplement block, bundle, assembly policy/status/result, budget, readiness, and
  safety-report records.
- Assembly reads `SemanticAcceptanceResult` only and can produce a separate bounded supplement
  bundle when an explicit test-only policy gate is enabled.
- Default desktop policy is disabled. Live prompt inclusion remains blocked, and existing prompt
  context injection still consumes only deterministic retrieval-planning selections.
- Runtime protections cover count budgets, character budgets, deterministic ordering,
  deterministic truncation, disabled fallback, empty fallback, stale/busy/timed-out/refused
  fallback, and safety report checks.
- Memory and Settings expose compact readiness/status, supplement block count, budget summary,
  safety summary, disabled-by-default state, and non-authoritative checks without raw prompt
  blocks, vectors, scores, provider handles, filesystem paths, or debug dumps.

Known limitation:

- Semantic supplements have no live prompt authority. The layer does not mutate
  `PromptContextBlock`, `RetrievalPlanningResult`, deterministic context order, conversation
  windows, summaries, committed memory, runtime metadata, or live provider prompts. Future
  semantic prompt activation still requires a separate phase with explicit prompt-authority
  policy, privacy/safety review, deterministic fallback tests, QML non-exposure guarantees, and
  live prompt inclusion tests.

### Phase 17.19-17.21: Deterministic Semantic Acceptance Layer

Completed. Adds bounded hybrid acceptance metadata that can approve a small subset of semantic
advisory candidates as retrieval supplements while preserving deterministic retrieval as the final
authority.

Scope:

- Added semantic acceptance policy, status, result, accepted-candidate, budget, readiness,
  arbitration, fallback, and source-summary records.
- Acceptance reads deterministic `RetrievalPlanningResult`, `HybridRetrievalBridgeResult`, and
  `SemanticSearchResult` metadata, then approves only bounded semantic supplements that pass
  deterministic gates.
- Deterministic retrieval candidates remain primary and win all conflicts. Accepted semantic
  candidates are supplemental-only, explicitly marked semantic, ordered after deterministic
  candidates, and constrained by supplement count and character budgets.
- Disabled, stale, busy, timed-out, refused, empty, or capacity-exhausted semantic sources expose
  deterministic-only fallback summaries without mutating retrieval planning or prompt context.
- Memory and Settings expose compact acceptance status/readiness, approved supplement counts,
  deterministic-vs-semantic participation, fallback state, arbitration summaries, bounded budget,
  and local-only non-authoritative checks without raw vectors, prompt payloads, provider handles,
  filesystem paths, or debug dumps.

Known limitation:

- Acceptance does not grant semantic prompt authority. It does not mutate
  `RetrievalPlanningResult`, create or mutate `PromptContextBlock` values, inject prompts, replace
  deterministic candidates, alter source priority, index filesystems, call cloud/API/vector
  providers, or execute tools/plugins. Future semantic-authority activation still requires a
  separate phase with indexing policy, privacy/safety gates, prompt-authority review,
  deterministic fallback tests, and QML non-exposure guarantees.

### Phase 17.16-17.18: Hybrid Retrieval Bridge Foundation

Completed. Adds a bounded, non-authoritative bridge that can read deterministic retrieval planning
and semantic search candidate metadata while leaving deterministic retrieval as the final prompt
authority.

Scope:

- Added hybrid bridge policy, status, result, candidate, budget, readiness, arbitration, and source
  summary records.
- Bridge arbitration is deterministic-first. Selected deterministic retrieval candidates occupy
  bridge capacity first; semantic candidates may only fill unused bounded metadata capacity.
- Deterministic candidates win all bridge ties and conflicts. Semantic candidates remain advisory
  and cannot override deterministic source priority, ranking, or prompt-context selection.
- Disabled, empty, stale, busy, timeout, and refused semantic sources fall back to deterministic
  retrieval summaries without mutating retrieval planning or prompt context blocks.
- Memory and Settings expose compact bridge readiness/status, deterministic-vs-semantic
  participation counts, arbitration summaries, fallback summaries, and local-only
  non-authoritative checks without raw vectors, prompt payloads, provider handles, filesystem
  paths, or debug dumps.

Known limitation:

- The bridge is metadata-only. It does not grant semantic prompt authority, inject semantic
  content, index filesystems, call cloud/API/vector providers, execute tools/plugins, or mutate
  deterministic retrieval. Future semantic-authority activation still requires a separate phase
  with explicit indexing policy, privacy/safety gates, deterministic fallback tests, prompt
  authority review, and QML non-exposure guarantees.

### Phase 17.13-17.15: Controlled Local Semantic Search Activation

Completed. Adds bounded local semantic candidate search for readiness validation and hybrid
orchestration testing while keeping deterministic retrieval authoritative.

Scope:

- Added semantic search policy, status, readiness, session, budget, candidate, result, and hybrid
  arbitration summary records.
- Added a local-only deterministic search path over local vector persistence entries only.
- Search is bounded by candidate count, timeout metadata, request/session state, isolated
  embedding-output readiness, and normalized similarity scoring.
- Semantic candidates are metadata-only. They can expose bounded match summaries and arbitration
  summaries, but cannot mutate retrieval planning, prompt context blocks, prompt assembly, or
  deterministic ranking.
- Empty indexes, stale requests, busy sessions, timeout, disabled persistence, and non-local or
  authoritative policy attempts resolve to safe deterministic statuses.
- Memory and Settings expose compact readiness/runtime state, candidate counts, bounded search
  summaries, arbitration boundaries, and local-only/non-authoritative checks without raw vectors
  or debug payloads.

Known limitation:

- Semantic search is non-authoritative and readiness-oriented. It does not index files, ingest in
  the background, call cloud/API/vector providers, download providers, inject prompt content,
  override deterministic retrieval, or activate real semantic prompt authority.

### Phase 17.10-17.12: Local Vector Persistence Foundation

Completed. Adds disabled-by-default local vector persistence lifecycle metadata and a bounded
local index foundation without giving semantic retrieval prompt authority.

Scope:

- Added vector persistence policy, status, health, readiness, session, budget, result, lifecycle,
  and snapshot summary records.
- Added a local-only deterministic vector persistence lifecycle helper with explicit create,
  reset, clear, and isolated embedding-output acceptance paths.
- Persistence remains disabled by default in the desktop runtime. No automatic indexing,
  filesystem scanning, background ingestion, semantic retrieval authority, prompt mutation,
  automatic memory conversion, cloud/API provider, or external vector service is introduced.
- Lifecycle operations include empty-index handling, bounded item limits, stable summaries, stale
  session protection, and busy-state protection.
- Memory and Settings expose compact readiness, lifecycle status, bounded/local-only state,
  disabled-by-default state, and indexed item count without raw vectors, paths, or debug payloads.

Known limitation:

- Semantic retrieval remains disabled and deterministic retrieval remains authoritative. The
  vector persistence foundation stores only bounded local metadata in tests and is not wired into
  retrieval planning, ranking, prompt assembly, memory conversion, filesystem indexing, or
  background jobs.

### Phase 17.7-17.9: Isolated Local Embedding Runtime Activation Foundation

Completed. Adds isolated local embedding generation metadata and bounded test execution helpers for
readiness validation only, without enabling semantic retrieval authority.

Scope:

- Added embedding runtime status, health, session, generation result, generation policy,
  generation readiness, and isolation policy records.
- Isolated generation is allowed only when local-only mode, explicit semantic readiness, local/fake
  provider scope, no cloud providers, no filesystem indexing, no prompt integration, no ranking
  mutation, no automatic memory writes, no vector persistence, and no background indexing gates all
  pass.
- Fake/InMemory generation can run through the isolated helper for deterministic tests. Local
  Ollama embeddings remain a bounded local-only readiness path and are not wired into desktop
  semantic retrieval.
- Runtime metadata reports refused, succeeded, failed, timed out, stale, and busy states with
  deterministic timeout and request-id/session checks.
- Memory and Settings expose compact isolated runtime readiness, last-test status, health, and
  local-only bounded state without raw vectors or debug payloads.

Known limitation:

- Semantic retrieval remains disabled. Isolated generation does not mutate prompt assembly,
  retrieval planning, retrieval ranking, memory stores, vector databases, filesystem indexes, or
  background jobs. No cloud/API provider, API key, provider download, autonomous action, tool, or
  plugin behavior is introduced.

### Phase 17.4-17.6: Hybrid Arbitration Simulation And Embedding Runtime Planning

Completed. Adds deterministic semantic arbitration simulation and local embedding runtime
planning metadata without enabling semantic retrieval.

Scope:

- Added semantic arbitration policy, status, result, simulated score, budget, and selection
  summaries plus embedding runtime plan, budget, and readiness metadata.
- Simulated scoring is deterministic metadata only. It uses local candidate metadata, fixed source
  weights, bounded content-size buckets, stable source ordering, and candidate-id tie handling.
- Deterministic retrieval remains the final prompt authority. Simulated semantic rankings cannot
  create prompt blocks, change retrieval planning, or mutate prompt assembly.
- Embedding runtime planning exposes estimated jobs/items, rough memory/storage cost, local-only
  requirements, and disabled constraints for future activation.
- Memory and Settings show compact arbitration/runtime readiness without raw vectors, raw score
  payloads, provider handles, index handles, or activation controls.

Known limitation:

- Semantic retrieval is still disabled. There are no real embeddings, vector DB writes, filesystem
  indexing, Ollama embedding calls, provider/model inference, semantic prompt injection, cloud/API
  keys, tools/plugins, autonomous actions, or prompt mutation.

### Phase 17.0-17.3: Semantic Provider Planning And Local Selection

Completed. Adds real semantic retrieval activation planning metadata without activating semantic
ranking, embedding calls, vector writes, or prompt injection.

Scope:

- Added semantic provider planning records for descriptors, selection, readiness, health,
  capabilities, policy, activation readiness, and activation results.
- Supported planned provider modes are Disabled, Fake/InMemory test provider, Local Ollama
  embeddings provider, and Local file/vector index.
- The desktop default selection is Disabled. Activation readiness refuses by default and reports
  required later-phase steps.
- Local Ollama embeddings and local file/vector index modes are planned-only and inactive. No
  Ollama embedding request, vector database write, filesystem scan, download, cloud/API-key
  behavior, tool/plugin action, or system action is introduced.
- Deterministic retrieval remains authoritative, and prompt assembly still consumes only the
  existing deterministic retrieval-planning result.
- Controller and desktop view model expose QML-safe selected provider, mode, readiness, health,
  capability summaries, activation readiness, activation summary, and required steps.
- Memory and Settings show compact Semantic Provider readiness without activation controls.

Known limitation:

- Semantic retrieval remains disabled. Phase 17.0-17.3 prepares local-only activation gates only;
  no semantic ranking/search, semantic prompt injection, durable vector index, real embedding
  provider execution, or prompt mutation is enabled.

### Phase 16.43-16.45: Futuristic Runtime Surface Polish

Completed. Polishes the existing runtime presentation into a more cinematic local AI operating
layer while leaving backend behavior unchanged.

Scope:

- Added reusable QML presentation components for layered radial glow, lightweight status pulses,
  and compact runtime badges.
- Refined the Home runtime surface with layered glow depth, a smoother orbital core, reduced
  particle count, idle breathing motion, and compact runtime-state badges.
- Added subtle visual states for deterministic retrieval authority, context assembly availability,
  semantic retrieval disabled policy, local runtime ready/unavailable, and streaming active/inactive.
- Polished context/retrieval chips with stable sizing, hover glow, soft color transitions, and
  low-amplitude pulse indicators.
- Improved translucent panel hierarchy with soft edge lighting across chat, runtime, and memory
  surfaces.
- No controller, provider, model, retrieval, prompt assembly, semantic, vector, filesystem,
  plugin, tool, microphone, playback, or runtime authority behavior was changed.

Motion and performance constraints:

- Motion uses declarative opacity, scale, rotation, and color animations only.
- Particle count remains bounded and lower than the previous home surface.
- Animations are slow, subtle, and limited to presentation items; they do not allocate timers,
  trigger layout churn, or move large page layouts.

Known limitation:

- This is presentation polish only. Deterministic retrieval remains authoritative, semantic
  retrieval remains disabled, real provider/model/tool execution is unchanged, and QML continues
  to consume summary/count/status metadata only.

### Phase 16.40-16.42: Runtime UX And Context Visibility Polish

Completed. Polishes the existing Memory, Chat, and Settings presentation for the current
memory/context/retrieval runtime without expanding backend behavior.

Scope:

- Added compact context status chips and clearer status rows for prompt injection, deterministic
  retrieval, conversation windowing, deterministic summaries, and disabled semantic retrieval.
- Added a Memory context-pipeline surface that separates committed key-value memory, literal
  recall, context assembly, retrieval planning, semantic readiness, and memory-candidate review
  metadata.
- Updated Chat to show a compact context status near the live surface: context injection on/off,
  selected retrieval source count, conversation window in/out status, summary block status, and
  semantic disabled state.
- Reduced Settings noise by foregrounding concise status chips and keeping semantic/hybrid
  readiness checks behind an advanced-details toggle.
- No controller, provider, model, retrieval, prompt assembly, semantic, vector, filesystem,
  plugin, or tool behavior was changed.

Known limitation:

- This is UI visibility polish only. Semantic retrieval remains disabled, deterministic retrieval
  remains authoritative, prompt context injection remains opt-in, and QML still exposes only
  summary/count/status metadata.

### Phase 16.37-16.39: Semantic Retrieval Activation Readiness Checkpoint

Completed. Checkpoints the full Phase 16 memory/context/retrieval architecture before any real
semantic retrieval activation.

Scope:

- Added `docs/PHASE_16_MEMORY_CONTEXT_CHECKPOINT.md`.
- Audited the memory candidate lifecycle, explicit commit boundary, literal local recall, context
  assembly, opt-in prompt injection, conversation windows, deterministic summaries, retrieval
  planning, embedding/vector abstractions, semantic candidate orchestration, hybrid readiness, UI
  exposure, and documentation consistency.
- Confirmed deterministic retrieval planning remains authoritative for prompt context selection.
- Confirmed semantic retrieval remains disabled: no runtime embedding provider, no runtime vector
  index, no indexed semantic items, no semantic ranking/search, and no vector database activation.
- Confirmed semantic candidates do not mutate prompts and do not feed prompt context injection.
- Confirmed committed key-value memory is the only memory source that recall and prompt injection
  may read.
- Confirmed prompt context injection remains opt-in and runs only after existing local inference
  gates.
- Confirmed QML exposure remains summary/count/status/readiness-only, with no raw vectors, scores,
  prompt payloads, provider/index handles, or private semantic candidate payloads.
- No additional QA tests were required because existing focused tests already cover these
  checkpoint guarantees.

Known limitation:

- Real semantic retrieval activation remains future-gated. Phase 17 must separately scope and test
  a concrete embedding provider, vector index, indexing policy, deterministic fallback behavior,
  privacy/safety gates, and QML non-exposure guarantees before semantic candidates can influence
  prompts.

### Phase 16.34-16.36: Semantic Candidate Orchestration Foundation

Completed. Adds semantic candidate orchestration metadata without enabling semantic retrieval.

Scope:

- Added value-only semantic candidate orchestration records for candidates, sources, selection,
  budgeting, windows, arbitration, summaries, status, policy, and hybrid retrieval readiness.
- Candidate orchestration derives deterministic local metadata from recent conversation windows,
  deterministic summaries, committed memory, runtime metadata, orchestration metadata, and a
  disabled future semantic/vector source placeholder.
- Arbitration uses fixed source ordering, deterministic character budgeting, deterministic
  exclusion/truncation metadata, source isolation, and chronology preservation inside
  conversation-derived sources.
- Hybrid retrieval readiness reports that deterministic retrieval remains authoritative while the
  semantic path, semantic prompt injection, provider/model calls, and vector database activation
  remain disabled.
- Controller and desktop view model expose only QML-safe status, readiness, candidate counts,
  budget summaries, arbitration summaries, participation summaries, and disabled/enabled state.
- Memory and Settings show compact semantic orchestration and hybrid retrieval readiness copy that
  clearly states semantic retrieval is not active.
- Prompt assembly remains unchanged. Semantic candidates are not injected into prompts, do not
  alter retrieval planning, and do not expose raw semantic/vector payloads.
- Tests cover deterministic candidate lifecycle metadata, arbitration ordering, budgeting, source
  isolation, chronology preservation, prompt non-mutation, QML-safe exposure, and hybrid disabled
  readiness without real Ollama/provider/vector DB requirements.

Known limitation:

- Semantic retrieval remains disabled. There are no real embeddings, transformer inference,
  semantic ranking/search, vector database integrations, cloud/API keys, provider/model calls,
  filesystem/system actions, plugins/tools, semantic prompt injection, raw vector payloads, or
  runtime authority expansion.

### Phase 16.31-16.33: Embedding And Vector Abstraction Foundation

Completed. Adds semantic retrieval readiness abstractions without enabling semantic retrieval.

Scope:

- Added value/interface types for embedding providers, embedding requests/results/documents,
  vectors, vector index policy/status, vector search queries/results/candidates, and semantic
  retrieval policy/status.
- Added `IEmbeddingProvider` and `IVectorIndex` boundaries for future embedding providers and
  vector index backends.
- Added deterministic local fake implementations for tests only: `FakeEmbeddingProvider` and
  `FakeVectorIndex`.
- Fake embeddings are stable hash/token-count vectors. Fake vector search is in-memory only with
  deterministic cosine scoring and stable tie ordering.
- Controller and desktop view model expose only QML-safe readiness metadata: semantic retrieval
  disabled state, embedding provider readiness, vector index readiness, indexed item counts,
  summaries, and checks.
- Memory and Settings show compact semantic/vector readiness and clearly state that semantic
  retrieval is not active.
- Retrieval planning remains deterministic source selection. Semantic readiness metadata does not
  change ranking, selected sources, prompt assembly, or prompt injection.
- Tests cover deterministic fake embeddings, stable vectors, insert/search/remove, scoring order,
  retrieval-planning non-regression, prompt non-mutation, QML-safe exposure, and fake-only behavior
  with no real Ollama requirement.

Known limitation:

- Semantic retrieval remains disabled. There are no real embeddings, transformer inference,
  semantic ranking/search, vector database integrations, cloud/API keys, provider/model calls,
  filesystem writes, downloads, plugins/tools, system execution, semantic prompt injection, raw
  vector QML exposure, or runtime authority expansion.

### Phase 16.28-16.30: Deterministic Retrieval Planning Foundation

Completed. Adds deterministic retrieval-planning metadata between context source preparation and
prompt context injection.

Scope:

- Added value-only retrieval records: `RetrievalPlanningPolicy`, `RetrievalPlanningStatus`,
  `RetrievalPlanningResult`, `RetrievalCandidate`, `RetrievalSourcePriority`, `RetrievalBudget`,
  and `RetrievalSelectionSummary`.
- Retrieval planning deterministically selects participating context sources across recent
  conversation windows, deterministic conversation summaries, committed memory recall/runtime
  entries, runtime metadata, and orchestration metadata.
- Source priority is fixed and local: recent conversation, older conversation summaries, committed
  key-value memory, runtime metadata, then orchestration metadata.
- Planning applies a fixed character budget with deterministic allocation, truncation, selected
  source counts, excluded source counts, selected/excluded candidate counts, and source summaries.
- Prompt context injection now consumes selected retrieval candidates while keeping source blocks
  separate and preserving chronology inside conversation-derived blocks.
- Controller and desktop view model expose only QML-safe retrieval status, readiness, budget
  summaries, source summaries, and counts.
- Chat, Memory, and Settings show compact retrieval-planning status without raw prompt or private
  assembled payload display.
- Tests cover deterministic source priority, budget allocation, truncation, chronology
  preservation, summary/memory separation, no mutation during planning, QML-safe exposure, and fake
  local inference paths with no real Ollama requirement.

Known limitation:

- Retrieval planning is deterministic local source selection only. There is no semantic/vector
  search, embeddings, vector database, provider/model call, cloud/API-key behavior, automatic
  memory write, tools/plugins, filesystem/system action, debug console UI, broad redesign, or raw
  prompt exposure.

### Phase 16.25-16.27: Deterministic Conversation Summary Foundation

Completed. Adds bounded local conversation-summary metadata to the prompt context path.

Scope:

- Added value-only summary records: `ConversationSummaryPolicy`,
  `ConversationSummaryStatus`, `ConversationSummaryResult`, `ConversationSummaryBlock`,
  `ConversationSummaryWindow`, and `ConversationSummaryBudget`.
- Older messages omitted by the recent conversation window can produce compact deterministic
  summary blocks with original message indexes and role visibility.
- Summary generation is heuristic/local only: it groups and truncates transcript text
  deterministically and does not call providers/models, run semantic summarization, build
  embeddings, or use vector search.
- Prompt context keeps bounded recent conversation history, older conversation summaries,
  committed key-value memory, runtime metadata, and orchestration metadata as separate delimited
  blocks.
- Recent conversation window priority is preserved by assembling it before older summaries.
- Summary budgeting exposes deterministic character limits, included size, block counts,
  summarized-message counts, omitted-message counts, and truncated-block counts through
  QML-safe controller/view-model values.
- Chat and Settings show compact summary readiness/budget status without exposing raw private
  prompt payloads.
- Tests cover deterministic summary generation, chronological role-visible blocks, budget
  truncation, prompt separation from committed memory, no mutation during summary planning, and
  controller/view-model exposure.

Known limitation:

- Summaries are extractive/heuristic transcript compaction only. There is no semantic
  summarization, embeddings/vector DB, provider/model call, cloud/API-key behavior, tools/plugins,
  filesystem/system actions, automatic memory write, or broad UI redesign.

### Phase 16.22-16.24: Conversation Window Management Foundation

Completed. Adds deterministic bounded conversation-history windowing to the opt-in local prompt
context path.

Scope:

- Added value-only conversation window records: `ConversationWindowPolicy`,
  `ConversationWindowSummary`, `ConversationWindowResult`, `ConversationWindowStatus`, and
  `ConversationWindowBudget`.
- Conversation window assembly prioritizes recent messages, then emits included messages in
  stable chronological order.
- The window uses a simple configurable character budget and deterministic truncation only.
- Prompt injection now uses the bounded recent conversation window instead of the full active
  transcript, with explicit conversation-history delimiters kept separate from committed memory,
  runtime metadata, and orchestration metadata blocks.
- The current user prompt remains outside the conversation-history window and is kept in the
  existing `User prompt:` section.
- Controller and desktop view model expose QML-safe window status, budget summary, included
  message count, omitted message count, and truncated message count.
- Chat and Settings show compact conversation-window status without raw prompt or private context
  payload display.
- Tests cover deterministic recent-message prioritization, bounded budget enforcement,
  committed-memory separation, prompt stability under long chats, no extra mutation during
  assembly, and controller/view-model exposure.

Known limitation:

- Windowing is character-budgeted and deterministic only. There is no semantic ranking,
  embeddings, vector DB, transcript summarization, provider/model change, cloud/API-key behavior,
  tools/plugins, filesystem/system actions, or broad UI redesign.

### Phase 16.19-16.21: Safe Prompt Context Injection Foundation

Completed. Adds deterministic, opt-in prompt context injection for guarded local Ollama requests.

Scope:

- Added value-only prompt context records: `PromptContextBlock`, `PromptContextBundle`,
  `PromptContextInjectionPolicy`, `PromptContextInjectionStatus`, and
  `PromptContextInjectionResult`.
- Context injection is disabled by default and requires an explicit persisted setting:
  “Use local memory/context in chat”.
- When enabled, the controller builds a clearly delimited local context block and prepends it to
  the local Ollama prompt only after existing busy/model/endpoint/permission/safety gates pass.
- Allowed sources are current conversation context, committed local key-value memory, conversation
  runtime metadata summaries, and orchestration metadata summaries.
- Pending/rejected memory candidates are excluded because only committed `IMemoryStore` entries are
  read.
- Injection enforces a fixed character budget with deterministic source order and truncation.
- Controller and desktop view model expose QML-safe enabled/status/summary, injected block count,
  source summary, size summary, and block summaries without exposing the raw assembled prompt.
- Settings exposes a guarded toggle, and Chat shows compact context status.
- Tests cover disabled prompt preservation, enabled deterministic injection, committed-only memory
  inclusion, pending/rejected exclusion, budget truncation, no mutation, safety-gate ordering, and
  view-model exposure.

Known limitation:

- Injection is local prompt assembly only. There is no semantic ranking, embeddings, vector DB,
  automatic memory write, cloud/provider/API-key behavior, tools/plugins, filesystem/system
  actions, voice/runtime scope change, or UI exposure of the private raw prompt.

### Phase 16.16-16.18: Context Assembly Planning Foundation

Completed. Adds deterministic context assembly planning metadata while keeping actual prompt
assembly and automatic context attachment disabled.

Scope:

- Added value-only context assembly records: `ContextAssemblyRequest`,
  `ContextAssemblySource`, `ContextAssemblyResult`, `ContextAssemblyStatus`,
  `ContextAssemblyPolicy`, and `ContextAssemblySummary`.
- Planning estimates participation for conversation context, committed key-value memory context,
  runtime metadata context, and orchestration metadata context.
- Planning summarizes candidate source blocks and simple character-size estimates only.
- Controller and desktop view model expose QML-safe status, source counts, block counts,
  estimated size, availability strings, source summaries, and readiness checks.
- Memory page shows a compact “Context Assembly” readiness section.
- Tests cover deterministic summaries, empty-context behavior, committed memory source visibility,
  no prompt injection side effects, no mutation during planning, and controller/view-model
  exposure.

Known limitation:

- Context assembly remains planning-only. It does not assemble LLM prompts, attach context to
  chat, call providers/models, run semantic ranking, build embeddings/vector indexes, execute
  tools/plugins, access cloud/API keys, or perform filesystem/system actions.

### Phase 16.13-16.15: Memory Recall Metadata And Local Memory Surfacing

Completed. Adds deterministic local recall over committed key-value memory entries while keeping
recall read-only and separate from chat prompt construction.

Scope:

- Added value-only recall metadata: `MemoryRecallQuery`, `MemoryRecallResult`,
  `MemoryRecallSummary`, `MemoryRecallStatus`, and `MemoryRecallPolicy`.
- Recall reads only the existing `IMemoryStore` key-value entries and uses literal key/value
  matching.
- Empty queries return an empty-query summary and do not mutate memory or chat state.
- Recall results expose QML-safe status, summary, result count, memory entry count, and compact
  result strings.
- Memory page shows a compact “Local Memory Recall” search field and matching committed entries.
- Recall does not call providers/models, does not build embeddings, does not use a vector DB or
  semantic search, and does not inject results into chat prompts.
- Tests cover committed key/value recall, empty query behavior, read-only recall, recall after
  commit, committed memory surviving clear chat, controller/view-model exposure, and no prompt
  injection side effects.

Known limitation:

- Recall is literal/local/read-only only. Semantic recall, embeddings, vector indexing, prompt
  injection, ranking, durable recall history, and automatic context assembly remain future work.

### Phase 16.10-16.12: Explicit Memory Commit Boundary

Completed. Adds explicit user-controlled commitment from reviewed memory candidates into the
existing key-value memory store while keeping approval, planning, and commitment as separate
states.

Scope:

- Approved candidates can be committed only through an explicit user Commit action.
- Approval remains review metadata only and does not write to `IMemoryStore`.
- Pending, rejected, archived, missing, store-unavailable, already-committed, and duplicate-key
  cases refuse safely.
- Commit keys are deterministic and sanitized from candidate category, title, and id.
- Commit values store only the reviewed candidate content. Source/review metadata remains in the
  commit result and candidate committed summary, not in the key-value memory value.
- The default conflict policy refuses existing keys; overwrite is not enabled.
- Committed candidates expose committed status, committed key, and committed timestamp summaries
  through controller/view-model strings and counts.
- Memory UI shows Commit only for approved candidates and labels Approve as review while Commit
  stores to local memory.
- Tests cover approved commit, pending/rejected/archived refusal, duplicate-key refusal, no
  automatic commit on approval, committed status exposure, clear-chat preserving committed memory,
  and controller/view-model exposure.

Known limitation:

- Commit target is only the existing local key-value memory store. There is no embeddings/vector
  DB, semantic search, provider/model call, cloud/API-key behavior, tools/plugins, filesystem or
  system action expansion, autonomous capture, or overwrite UI.

### Phase 16.7-16.9: Approved Memory Commit Planning

Completed. Adds explicit commit-planning metadata for approved memory candidates while keeping
actual key-value memory mutation disabled by default and future-gated.

Scope:

- Added value-only commit-planning records: `MemoryCommitPlan`, `MemoryCommitTarget`,
  `MemoryCommitReadiness`, `MemoryCommitResult`, and `MemoryCommitPolicy`.
- Approved candidates now produce deterministic candidate-to-key-value-memory commit plan
  summaries. Pending, rejected, archived, or missing candidates report blocked readiness.
- The default commit policy disables actual commit. Commit requests refuse safely and do not call
  `IMemoryStore::put()`.
- Approval remains review metadata only. Approved candidate does not mean committed memory.
- `DesktopShellViewModel` exposes QML-safe commit readiness status, checks, plan count,
  target summary, per-candidate commit summaries, and last commit request result strings.
- Memory shows a compact “Commit Readiness” status inside the existing candidate section and makes
  the future-gated state visible without adding an enabled store/commit action.
- Tests cover approved-candidate plan generation, pending/rejected refusal, disabled default
  policy, refused no-mutation commit requests, deterministic summaries/counts, and view-model
  exposure.

Known limitation:

- Candidate storage remains in-memory only and commit execution is non-operational. There is no
  automatic memory write, semantic store, embeddings, vector DB, semantic search, provider/model
  call, filesystem authority, tool/plugin authority, or autonomous memory mutation.

### Phase 16.0-16.6: Controlled Semantic Memory Foundation And Review Flow

Completed. Adds a review-only semantic memory candidate foundation and explicit candidate review
flow without embeddings, vector storage, automatic capture, or long-term memory mutation.

Scope:

- Added value-only memory candidate abstractions: candidate id, source, category, confidence,
  review state, retention policy, capture policy, candidate summary, `IMemoryCandidateStore`, and
  `InMemoryMemoryCandidateStore`.
- `ApplicationController` can create candidates from supplied conversation text metadata only.
  Candidates default to `Pending Review`.
- Approval and rejection update review metadata only. They do not write to `IMemoryStore`, memory
  taxonomy, chat history, conversation storage, files, providers, models, tools, plugins, or
  runtime services.
- Phase 16.4 through Phase 16.6 add explicit approve, reject, reset-to-pending, and archive review
  actions with `MemoryCandidateReviewResult`, reviewed timestamp, reviewer/source summary, and
  decision reason metadata.
- Review transitions are guarded: approve/reject require Pending Review, reset requires Approved or
  Rejected, and archive is terminal metadata after review.
- Approved candidate metadata means reviewed, not committed. It is not automatically stored as
  long-term key-value memory or semantic memory.
- `DesktopShellViewModel` exposes QML-safe candidate ids, states, counts, state-filtered summaries,
  and last review result strings.
- Memory shows a compact “Memory Candidates” section with review counts, last review result, and
  Approve/Reject/Reset controls. Approved is labeled as reviewed metadata, not committed memory.
- Tests cover creation, default pending review, approve/reject/reset/archive transitions, invalid
  transition refusal, no key-value memory mutation, deterministic counts/summaries, QML-safe
  exposure, and clear-chat preserving approved candidate metadata.

Known limitation:

- Candidate storage is in-memory only and review metadata is not durable. There is no semantic
  search, embeddings, vector DB, autonomous memory writes, cloud sync, provider/model call,
  filesystem authority, tool/plugin authority, or automatic capture toggle.

### Phase 15.33-15.35: Conversation Browser Runtime QA And Checkpoint

Completed. Checkpoints the multi-conversation browser/runtime path after the delete-readiness
polish and confirms it remains local, non-destructive, and QML-safe.

Scope:

- Reviewed active-conversation ownership, store/browser actions, session switching, archive
  behavior, stale async response handling, and delete-readiness refusal behavior.
- Added `docs/PHASE_15_CONVERSATION_CHECKPOINT.md`.
- Added focused QA coverage for SQLite soft-delete metadata retaining message rows and controller
  permanent-delete refusal leaving a SQLite conversation store unchanged.
- Confirmed legacy single-transcript history remains a compatibility startup source when the
  conversation store is empty.
- Confirmed archived conversations remain visible/loadable and block sending clearly.

Known limitation:

- Permanent delete remains disabled and non-mutating. No semantic memory, embeddings/vector DB,
  cloud sync, import/export changes, permanent delete execution, broad UI redesign,
  model/voice/tool/plugin changes, or runtime authority expansion is added.

### Phase 15.30-15.32: Conversation Browser Polish And Delete Readiness

Completed. Polishes the compact Conversation Browser and adds safe permanent-delete readiness
metadata while keeping archive as the supported removal flow.

Scope:

- Chat browser now marks the current conversation more clearly, mutes archived rows, shows active
  versus inactive row summaries, and shows an empty-state hint when no user-created conversations
  exist.
- Rename actions now provide compact success/refusal feedback without broad redesign.
- Archived active conversations remain loadable, but Chat shows a clear hint and keeps sending
  disabled until the conversation is unarchived.
- Added `ConversationDeletePolicy`, `ConversationDeleteReadiness`, and
  `ConversationDeleteResult` metadata.
- Permanent delete is disabled by default. Delete readiness exposes why it is disabled, and the
  guarded delete request path refuses safely without mutating storage.
- Settings shows active/archived conversation counts and archive-first delete policy/readiness.
- Tests cover archived active send blocking, archive/unarchive browser summaries, disabled delete
  readiness, refused no-mutation delete requests, valid active conversation state after
  archive/unarchive, and QML-safe view-model exposure.

Known limitation:

- Permanent delete remains non-operational. There is no destructive delete UI, cloud sync, import,
  multi-conversation export, arbitrary filesystem writes, model/voice/tool/plugin change, or
  runtime safety-policy change.

### Phase 15.26-15.29: Conversation Browser UI Foundation And Safe Session Switching

Completed. Exposes the multi-conversation store through a compact Chat browser and makes the
active transcript load from `IConversationStore` while keeping legacy single-transcript history as
a compatibility source when needed.

Scope:

- Desktop wiring now uses `SQLiteConversationStore` at the app data `conversations.sqlite3` path.
- `ApplicationController` maintains one valid active conversation id, creates an initial
  conversation when the store is empty, and copies existing single-transcript startup messages
  into that conversation when possible.
- Chat shows a compact conversation browser with title, last-updated summary, message count, and
  archived state, plus create, switch, rename, archive, and unarchive actions.
- Switching conversations invalidates active async request metadata, clears live streaming
  preview, resets runtime/search state, loads the selected transcript, and emits QML-safe browser
  metadata updates.
- Stale async completions from an earlier active conversation are ignored through the existing
  request-id guard, so they do not append duplicate assistant messages or corrupt the newly active
  transcript.
- Tests cover create/list/load, switching, rename/archive/unarchive, SQLite persistence across
  controller recreation, stale async result protection, duplicate-insertion avoidance, busy reset,
  active-conversation validity, and view-model exposure.

Known limitation:

- The browser is intentionally compact and local-only. There is no permanent delete UI, import,
  cloud sync, multi-conversation export, semantic/vector search, embeddings, tools/plugins, or
  changes to Ollama/runtime safety policy.

### Phase 15.23-15.25: Multi-Conversation Storage Foundation

Completed. Adds the first real multi-conversation store boundary while keeping the active desktop
chat path on the existing single-transcript `IChatHistoryStore`.

Scope:

- Added `IConversationStore`, `ConversationRecord`, `ConversationMessageRecord`,
  `ConversationStoreStatus`, and `ConversationStoreError`.
- Added `InMemoryConversationStore` and `SQLiteConversationStore` with create/list/load,
  append-message, rename, archive, and soft-delete behavior.
- `ApplicationController` owns a conversation-store dependency separately from `IChatHistoryStore`
  and exposes QML-safe status, count, active summary, and summary list values.
- Settings shows read-only conversation-store readiness and active-conversation summary.
- Tests cover in-memory and SQLite create/list/load, append/load, deterministic ordering,
  persistence across SQLite instances, no destructive migration of the single transcript store, and
  controller/view-model exposure.

Known limitation:

- Existing chat still uses the current single local transcript. There is no UI switch to
  multi-conversation workflows, no import, no cloud sync, and no automatic migration from
  `chat_history.sqlite3`.

### Phase 15.20-15.22: Multi-Conversation Planning Skeleton

Completed. Adds metadata-only multi-conversation planning readiness while preserving current
single-transcript storage behavior.

Scope:

- Added planning abstractions: `ConversationId`, `ConversationDescriptor`,
  `ConversationLifecycleStatus`, `ConversationStorageMode`, `ConversationMigrationReadiness`, and
  `ConversationSchemaPlan`.
- `ApplicationController` now exposes read-only planning/readiness metadata:
  current storage mode, future storage mode, migration readiness, migration status summary, and
  schema status summary.
- `DesktopShellViewModel` forwards these values as QML-safe strings only.
- Settings now includes a compact read-only “Multi-conversation readiness” status and schema-plan row.
- Tests cover deterministic planning metadata, single-transcript mode staying active, planned/not
  started readiness reporting, no storage mutation from metadata reads, and QML-safe exposure.

Known limitation:

- No SQLite schema migration, no multi-conversation persistence, no import, no cloud sync, and no
  transcript browser/thread controls are added in this phase.

### Phase 15.17-15.19: Conversation Browser Metadata Foundation

Completed. Adds single-transcript conversation-browser metadata without changing storage shape.

Scope:

- Added value-only metadata records for a future browser surface:
  `ConversationDisplayTitle`, `ConversationListEntry`, `ConversationListSummary`, and
  `ConversationBrowserStatus`.
- `ApplicationController` now exposes QML-safe current-transcript browser metadata: browser status,
  single-entry count, current title, message count, persistence status, last updated/saved summary,
  and search/export availability summaries.
- Metadata remains deterministic and single-entry: one current local transcript is always exposed.
- `DesktopShellViewModel` forwards these values as strings/booleans/counts only.
- Settings now shows a compact “Current Transcript” readiness section using these values.
- Tests cover deterministic single entry, empty transcript summary, message count summary, clear-chat
  update behavior, search/export availability reflection, and QML-safe view-model exposure.

Known limitation:

- This is foundation only. No multi-conversation storage, thread list, transcript browser UI,
  import, export-path picker, cloud sync, or schema migration is added.

### Phase 15.14-15.16: Local Transcript Export Implementation

Completed. Adds safe local Markdown/JSON export for the current single transcript.

Scope:

- Added controlled current-transcript export for Markdown and JSON only.
- Export writes only to the app-owned export directory below Qt `AppDataLocation`; QML does not
  receive raw filesystem paths.
- Export filenames are sanitized, timestamped, and made unique to avoid silent overwrite.
- Empty transcripts with only the initial system message are refused with a safe summary.
- Export result metadata now reports status, safe output filename, exported message count, last
  export timestamp, and concise error/refusal summaries.
- `ApplicationController` exposes `exportTranscript(format)` and keeps the existing export request
  wrapper forwarding to the same implementation.
- `DesktopShellViewModel` exposes QML-safe export status/summary/filename/count/timestamp values.
- Chat and Settings show small Export Markdown and Export JSON actions plus last export status.
- Tests cover Markdown content, JSON structure, empty-transcript refusal, timestamped filename
  behavior, view-model exposure, and no arbitrary path writes through the UI/controller action.

Known limitation:

- Export is current-transcript only. There is no import, file picker, custom output path, transcript
  browser, multi-conversation export, encryption, pruning, cloud sync, or external process.

Still out of scope:

- Cloud/API keys/providers, model downloads/pulls/deletes, Ollama process management, tools,
  plugins, arbitrary filesystem actions, autonomous loops, microphone access, playback, Piper
  changes, Whisper execution, vector search, embeddings, semantic search, database FTS, import,
  and multi-conversation storage.

### Phase 15.11-15.13: Conversation Search, Export Readiness, And Transcript QA

Completed. Adds lightweight transcript search metadata and disabled export readiness for the
current single transcript.

Scope:

- Added value-only conversation search query/result/summary metadata over the current in-memory
  `ChatSession` transcript.
- Search is literal, case-insensitive, and in-memory only. It does not use vector search, semantic
  search, embeddings, SQLite full-text indexes, or database queries.
- Empty search queries produce an empty-query summary and do not mutate chat history.
- Clear Chat resets search metadata along with runtime state, persistence, streaming/live text,
  active request metadata, and the transcript reseed.
- Added value-only export format/request/readiness/result metadata. Export remains disabled and
  metadata-only; no file picker is shown and no filesystem export/write action is performed.
- Chat and Settings show compact search/export readiness. Chat also provides a small current
  transcript search field.
- Tests cover user/assistant search matches, empty query behavior, no history mutation during
  search, search reset on clear, disabled export/no side effects, and QML-safe view-model exposure.

Known limitation:

- Chat history remains one local transcript. Search is not persisted, not indexed, not semantic,
  and not multi-conversation aware. Export/import, encryption, pruning, file writing, and transcript
  browser workflows remain unimplemented.

Still out of scope:

- Cloud/API keys/providers, model downloads/pulls/deletes, Ollama process management, tools,
  plugins, filesystem/system actions, autonomous loops, microphone access, playback, Piper
  changes, Whisper execution, vector search, embeddings, semantic search, database FTS, file
  picker/export writes, and multi-conversation storage.

### Phase 15.10: Persistent Conversation UX And Chat History Management

Completed. Adds compact persistent conversation UX metadata on top of the existing single
transcript chat-history boundary.

Scope:

- Added value-only conversation history metadata for persistence status, message counts, last
  save/restore status, and clear results.
- `ApplicationController` now derives a QML-safe conversation history summary from the current
  `ChatSession` and `IChatHistoryStore` availability without exposing SQLite paths, schemas, or
  store internals.
- Chat and Settings show whether the transcript is persisted or runtime-only, the current message
  count summary, and concise saved/restored status.
- Clear Chat copy now states that runtime transcript, persisted local chat history when available,
  active request metadata, and live streaming text are reset while settings and memory are kept.
- Clear Chat continues to clear/reseed the single transcript deterministically with one initial
  system message and no duplicate startup/system messages.
- Tests cover persisted summary, runtime-only fallback summary, runtime plus persisted clear,
  streaming/request metadata reset after clear, and single initial system-message behavior.

Known limitation:

- Chat history remains one local transcript. There is still no multi-conversation/thread browser,
  export/import, encryption, pruning, transcript search, or advanced history management.

Still out of scope:

- Cloud/API keys/providers, model downloads/pulls/deletes, Ollama process management, tools,
  plugins, filesystem/system actions, autonomous loops, microphone access, playback, Piper
  changes, Whisper execution, transcript export/import, and multi-conversation storage.

### Phase 15.9: Conversation Runtime State And Session Continuity

Completed. Adds a concise conversation-runtime read model over the existing async local inference
and chat-history paths.

Scope:

- `ApplicationController` now tracks QML-safe conversation runtime state: current conversation
  state, current request id, active model, active route, active streaming state, last successful
  response summary, last error/refusal summary, and last latency summary.
- `DesktopShellViewModel` forwards the runtime state as strings, string lists, and booleans only.
- Chat shows the current session state, route, and active request id without exposing traces,
  worker objects, provider internals, or SQLite details.
- Chat history remains one stable local transcript. Successful inference appends one assistant
  message, failed inference appends one safe error assistant message, and stale async completions
  after cancellation do not update visible runtime state or history.
- App restart still loads persisted chat rows as-is and does not synthesize duplicate system or
  assistant messages when history exists.
- Clear Chat now resets transient runtime state, cancels the active request id metadata when
  needed, resets the conversation graph to Idle, and clears/reseeds persistent chat history through
  the existing `IChatHistoryStore` boundary.

Still out of scope:

- Cloud/API keys/providers, model downloads/pulls/deletes, Ollama process management, tools,
  plugins, filesystem/system actions, autonomous loops, microphone access, playback, Piper
  changes, and Whisper execution.

### Phase 15.8: Async Local Runtime Worker Foundation

Completed. Moves local chat inference execution behind an async worker boundary so real Ollama
generation and streaming calls no longer run on the UI/controller thread.

Scope:

- Added an injectable local inference worker abstraction above the existing non-streaming and
  streaming local inference clients.
- The desktop app uses the worker to run real loopback-only Ollama generate/stream calls off the
  controller thread while posting chunks and final results back through request-id guarded
  callbacks.
- Existing local chat gates remain unchanged: local chat opt-in, selected/effective model
  validation, loopback endpoint policy, runtime permission policy, runtime safety policy, and busy
  duplicate-send rejection.
- Chat finalization remains single-write: user messages are appended once, successful assistant
  responses are appended once, failures do not persist partial assistant output, and streaming
  preview text is cleared on error and completion.
- Cancellation is metadata-only: the active request id can be invalidated so stale async results
  are ignored. The current worker does not interrupt a running Ollama HTTP request mid-flight.
- Tests cover async fake success, async timeout/error, duplicate-send rejection while busy, stale
  result ignoring after cancellation, busy reset, and no duplicate assistant messages. No real
  Ollama service is required.

Known limitation:

- Ollama itself must still already be installed, running, and serving an installed local model
  outside Sentinel. Sentinel still does not launch Ollama, load models proactively, pull/delete
  models, manage model downloads, or provide hard network-request interruption beyond existing
  timeouts and stale-result rejection.

Still out of scope:

- Cloud/API keys/providers, model downloads/pulls/deletes, Ollama process management, tools,
  plugins, filesystem/system actions, autonomous loops, microphone access, playback, Piper
  changes, and Whisper execution.

### Phase 15.7: Ollama Reliability And Runtime Stabilization

Completed. Tightens local Ollama chat failure handling, timeout metadata, busy-state cleanup, and
stream finalization without expanding runtime authority.

Scope:

- Health checks, model discovery, non-streaming generation, and streaming generation now carry
  explicit timeout metadata in request/result traces.
- Local inference failures are categorized as Ollama not running, endpoint unreachable, no model
  selected, selected model missing, request timeout, malformed response, stream interrupted,
  permission/safety blocked, and busy request already in progress.
- Chat rejects duplicate sends while local inference is active before appending another user
  message.
- Failed streaming clears live preview text and does not persist partial assistant output.
- Successful streaming and non-streaming paths still append exactly one final assistant message.
- Chat UI disables input/send while inference is active and shows a concise inference summary near
  runtime status.
- Focused tests cover unavailable Ollama metadata, timeout, malformed response, selected model
  missing, duplicate-send rejection, stream interruption cleanup, busy reset after errors, and
  single final assistant persistence.

Known limitation:

- The current real Ollama clients still use synchronous Qt network waits behind explicit
  controller calls. Timeouts are bounded and shorter for health/model discovery, but a later phase
  should move real inference to an asynchronous worker path before broader runtime UX expansion.

Still out of scope:

- Cloud/API keys/providers, model downloads/pulls/deletes, Ollama process management, tools,
  plugins, filesystem/system actions, autonomous loops, microphone access, playback, Piper changes,
  and Whisper execution.

### Phase 15.4-15.6: Controlled Piper File-Output Execution

Completed. Enables explicit, policy-gated Piper TTS file generation to an app-controlled
cache/temp path while keeping execution disabled by default and keeping playback/microphone
behavior out of scope.

Current status: Superseded by Phase 18.22-18.24. The active desktop Piper path is now
readiness/synthesis metadata only; the legacy file-output opt-in/generation path is disabled and
refuses without subprocess execution, file output, or playback.

Scope:

- Added a persisted opt-in setting for controlled Piper file-output execution. The default remains
  disabled.
- Piper can run only from an explicit user action after the opt-in is enabled, the configured Piper
  binary is executable, the configured `.onnx` model is readable, local-only/process/file-output
  gates pass, and the output path is generated inside the controlled app cache/temp directory.
- A process-backed Piper client was reachable only after the controller applied the explicit
  execution policy; current Phase 18.22-18.24 behavior has removed that active subprocess path.
- Status metadata now reports disabled, blocked/safety-blocked, missing binary, missing model,
  running, succeeded, failed, and timeout states plus the generated audio path summary when
  available.
- Settings previously showed a Piper execution opt-in and Generate TTS File action; current
  Settings exposes Piper synthesis readiness/status/fallback/safety summaries only.
- Tests cover disabled defaults, opt-in gating, blocked/invalid paths, fake success, failure,
  timeout, controlled output path metadata, QML-safe exposure, persistence, and no real Piper
  requirement.

Still out of scope:

- Audio playback, microphone access, Whisper execution, arbitrary output paths, downloads,
  filesystem-wide scans, cloud/API keys, autonomous voice loops, and background voice actions.

### Phase 15.1-15.3: Voice Path Setup Refinement And Controlled Piper Readiness

Completed. Refines the local Piper/Whisper configuration surface and prepares clearer controlled
file-output TTS readiness metadata without enabling voice execution.

Scope:

- Voice Configuration now uses shorter labels, explicit help text, an Apply Paths action, compact
  Ready/Blocked/Missing badges, and concise validation rows.
- User-provided Piper and Whisper paths continue to persist through settings and immediately update
  controller/view-model readiness metadata.
- Exact configured-path validation reports Piper binary exists/executable, Piper `.onnx` model
  exists/readable, Whisper binary exists/executable, and Whisper model folder or file
  exists/readable.
- Piper file-output TTS preparation now reports Ready, Blocked, or Missing separately from the
  still-disabled Piper execution provider path, with blocked reasons listing the exact failed
  path checks.
- Whisper remains configuration/readiness only and reports whether STT can be prepared later.
- Tests cover persisted path updates, fake valid files, missing paths, executable versus
  non-executable binary metadata, QML-safe exposure, Apply Paths forwarding, and no execution side
  effects.

Still out of scope:

- Running Piper, running Whisper, microphone access, playback, downloads, filesystem-wide scans,
  cloud/API keys, autonomous voice loops, and voice action controls.

### Phase 14.7-15.0: Controlled Local Ollama Runtime Activation

Completed. Enables controlled local-only Ollama chat inference for explicitly selected local
models while preserving the no-cloud, no-agent, no-tool, and no-voice-execution posture.

Scope:

- The desktop app now wires the real loopback-only Ollama runtime client, non-streaming inference
  client, and streaming-ready inference client from the persisted Ollama endpoint setting.
- Runtime health checks use only the configured local loopback HTTP endpoint and `/api/version`.
- Local model discovery uses only the configured local loopback HTTP endpoint and `/api/tags`.
- A narrow local-only runtime permission policy allows only explicit `LocalInference` execution;
  provider invocation, tool invocation, external process, filesystem, broader network, and plugin
  permissions remain denied.
- Chat invokes Ollama only when local chat inference is explicitly enabled, the endpoint is
  loopback HTTP, a selected/effective local model is valid, and runtime permission/safety gates
  pass.
- Selected local model persistence remains in settings and is validated against discovered local
  model metadata when available.
- Runtime state exposure now uses the controlled activation vocabulary: unavailable, idle,
  inferencing, streaming, and failed.
- Streaming remains opt-in and architecture-ready; completed streams are finalized into one chat
  message and live preview text is cleared.
- Tests remain deterministic through fake Ollama runtime and inference clients.

Still out of scope:

- Cloud/API keys/providers, autonomous agents, tool execution, shell execution, filesystem-wide
  actions, model downloads/pulls/deletes, Ollama process management, microphone access, playback,
  Whisper execution, Piper playback, and autonomous voice loops.

### Phase 14.4-14.6: Voice Configuration UX Polish And Safe Auto-Detection Hints

Completed. Polishes the local Piper/Whisper Settings UX and adds read-only, non-invasive path
hints without enabling voice execution.

Scope:

- Settings now shows configured Piper/Whisper paths with concise help text, compact status badges,
  and short read-only hint rows instead of long noisy readiness summaries.
- `ApplicationController` exposes voice configuration status badges and hint summaries as
  QML-safe strings/string lists.
- Binary hints check only fixed known locations: `/opt/homebrew/bin/piper`,
  `/usr/local/bin/piper`, `/opt/homebrew/bin/whisper`, and `/usr/local/bin/whisper`.
- Model hints validate only configured paths for readability; no model directories are scanned.
- Hints are suggestions only and never write settings automatically.
- Tests cover configured readable/executable paths, missing paths, QML-safe exposure, empty-safe
  behavior, and the no-execution posture.

Still out of scope:

- Running Piper, running Whisper, microphone access, playback, downloads, filesystem-wide scans,
  cloud/API keys, autonomous voice loops, path pickers, and automatic settings changes from hints.

### Phase 14.0-14.3: Local Voice Configuration UX For Piper And Whisper

Completed. Adds safe persisted local voice path configuration and readiness metadata for Piper and
Whisper without enabling voice execution.

Scope:

- Added persisted settings for Piper binary path, Piper model path, Whisper binary path, and
  Whisper model directory/path.
- Settings now shows a compact Voice Configuration section with editable text fields, current path
  values, readiness summary, existing voice/Piper readiness, and exact-path validation metadata.
- `ApplicationController` validates only the configured paths as metadata: configured/missing,
  exists/missing, readable/unreadable, and executable/non-executable for binaries.
- `DesktopShellViewModel` exposes QML-safe strings, string lists, and booleans for voice
  configuration and Piper readiness.
- Piper readiness updates from configured metadata while execution remains blocked by existing
  safety posture.
- Tests cover empty defaults, fake configured files/directories, missing paths,
  executable/non-executable binary metadata, persistence reload, view-model exposure, and no
  execution side effects.

Still out of scope:

- Running Piper, running Whisper, microphone access, playback, model downloads, filesystem-wide
  scans, cloud/API keys, autonomous loops, voice action buttons, and path picker integration.

### Phase 13.9: Voice/Piper Checkpoint And Readiness Review

Completed. Reviews the current voice/Piper architecture after controlled file-output work and
records the Phase 14 readiness boundary without adding runtime behavior.

Scope:

- Added `docs/PHASE_13_CHECKPOINT.md` with completed Phase 13 scope, current Piper/voice
  architecture, current TTS path, safety findings, remaining limitations, future next steps, and
  Phase 14 readiness criteria.
- Confirmed the current TTS path is `text -> Piper provider -> gated file-output metadata`.
- Confirmed Piper remains disabled by default and file output is reachable only behind explicit
  configuration, request, binary/model, controlled-output-path, permission, and safety gates.
- Confirmed there is no playback, microphone access, Whisper execution, downloads, cloud/API-key
  behavior, or autonomous voice loop.
- Confirmed existing tests already cover the obvious Piper/voice safety gaps with null/static/fake
  providers and clients.

Still out of scope:

- New runtime behavior, playback, microphone access, Whisper/STT execution, downloads,
  cloud/API-key behavior, autonomous voice loops, voice setup UX, path/model pickers, and broad UI
  redesign.

### Phase 13.6-13.8: Controlled Piper TTS File-Output Boundary

Completed. Adds a gated local-only Piper text-to-audio file-output path while keeping Piper
execution disabled by default and keeping playback/microphone behavior out of scope.

Scope:

- Extended Piper request/result/configuration metadata with controlled output directory, output
  path summary, timeout, exit/error metadata, and file-output readiness.
- Piper file output can run only when the adapter is explicitly enabled, the Piper binary exists
  and is executable, the voice model exists and is readable, the output path is inside the
  app-controlled output directory, process execution is allowed by request/config/safety policy,
  playback and microphone remain blocked, and the request is local-only.
- Historical implementation note: this phase added a process-backed Piper client for controlled
  file output. Phase 18.22-18.24 removed that active subprocess client from current desktop
  behavior.
- The default Piper adapter remains disabled/not configured and refuses before any client boundary.
- `ApplicationController` and `DesktopShellViewModel` expose QML-safe Piper file-output status and
  summary metadata.
- Settings shows Piper file-output readiness only, with no speak/play button or setup redesign.
- Tests use a deterministic fake Piper client and do not require real Piper, models, playback, or
  microphones.

Still out of scope:

- Automatic playback, speak/play buttons, microphone access, Whisper/STT, model downloads,
  filesystem-wide scans, cloud/API-key behavior, autonomous voice loops, path/model pickers, and
  broad UI redesign.

### Phase 13.3-13.5: Piper TTS Adapter Skeleton

Completed. Adds a safe Piper text-to-speech provider boundary without enabling audio playback,
Piper execution, downloads, cloud calls, API keys, model loading, or filesystem-wide scans.

Scope:

- Added `PiperTtsConfig`, `PiperVoiceModelDescriptor`, `PiperTtsRequest`,
  `PiperTtsResult`, `PiperTtsStatus`, `IPiperTtsClient`, `NullPiperTtsClient`, and
  `PiperTextToSpeechProvider`.
- The default Piper adapter is disabled/not configured and reports deterministic readiness
  metadata only.
- Missing Piper binary or voice model metadata produces deterministic refusal before reaching any
  client boundary.
- `ApplicationController` exposes Piper TTS status, summary, readiness checks, and readiness bool.
- `DesktopShellViewModel` forwards Piper TTS metadata as QML-safe strings, string lists, and
  booleans.
- Settings shows read-only Piper readiness metadata with no speak button, path picker, model
  picker, setup action, playback control, or broad UI redesign.
- Tests cover null-client refusal, missing binary/model refusal, provider summaries,
  controller/view-model exposure, and no-side-effect posture.

Still out of scope:

- Audio playback, Piper subprocess execution, file-output synthesis, microphone access,
  Whisper/STT work, downloads, cloud/API-key behavior, filesystem-wide scans, automatic runtime
  probing, model loading, path/model pickers, speak controls, and uncontrolled process execution.

Next:

- A later explicit phase may add controlled file-output TTS only after enabling policy,
  user-controlled paths, lifecycle, cancellation, cleanup, and safety checks are defined.

### Phase 13.0-13.2: Local Voice Runtime Safety, Binary Ownership, And Model Ownership Skeleton

Completed. Adds metadata-only ownership boundaries for future local Piper/Whisper runtime
integration without enabling audio I/O, subprocesses, downloads, model loading, or execution.

Scope:

- Added voice binary, model, runtime permission, and runtime safety metadata for future Piper and
  Whisper ownership.
- Added `IVoiceRuntimeEnvironment` with deterministic null/static environment implementations.
- The default environment reports missing/not-configured Piper and Whisper binary/model metadata,
  denied voice runtime permissions, and blocked execution safety posture.
- `ApplicationController` exposes voice binary, model, permission, environment, and safety
  summaries.
- `DesktopShellViewModel` forwards only QML-safe strings, string lists, and booleans.
- Settings extends read-only Voice Readiness metadata with environment, binary, model, permission,
  and safety summaries, with no activation controls.
- Tests cover deterministic null/static environment behavior, missing binary/model metadata,
  safety blocking, controller/view-model exposure, and no-execution posture.

Still out of scope:

- Microphone access, audio playback, Piper execution, Whisper execution, subprocess/process
  launch, filesystem-wide scans, model loading, downloads, cloud calls, API keys, voice controls,
  path pickers, and broad UI redesign.

### Phase 12.7-12.9: Voice Checkpoint And Local Voice Integration Planning

Completed. Checkpoints the completed Phase 12 voice boundary/runtime architecture and documents
future local Piper/Whisper integration prerequisites without enabling audio I/O or execution.

Scope:

- Reviewed `ITextToSpeechProvider`, `ISpeechToTextProvider`, `VoiceSession`, voice pipeline
  metadata, `StaticVoiceRuntimeCoordinator`, `ApplicationController` ownership, and
  `DesktopShellViewModel` QML-safe exposure.
- Added `docs/PHASE_12_CHECKPOINT.md` with completed scope, current architecture, limitations,
  safety guardrails, future Piper plan, future Whisper plan, and Phase 13 readiness criteria.
- Confirmed existing tests cover deterministic voice pipeline behavior, null TTS/STT refusal,
  controller/view-model exposure, disabled runtime posture, and blocked/error metadata paths.

Still out of scope:

- Microphone access, audio playback, Piper execution, Whisper execution, subprocess/process
  launch, filesystem/system actions, downloads, cloud calls, API keys, voice controls, activation
  flows, autonomous voice loops, and broad UI redesign.

### Phase 12.3-12.6: Voice Runtime Planning And Session Orchestration Skeleton

Completed. Adds deterministic voice session/runtime orchestration metadata without enabling audio
capture, playback, local voice binaries, or runtime execution.

Scope:

- Added `VoiceSession`, `VoiceSessionId`, `VoiceSessionState`, `VoicePipelineStage`,
  `VoicePipelineStatus`, `VoicePipelineTrace`, `IVoiceRuntimeCoordinator`, and
  `StaticVoiceRuntimeCoordinator`.
- Added a deterministic metadata-only voice pipeline: idle, preparing, awaiting-input,
  transcribing-placeholder, inference-placeholder, synthesis-placeholder, and completed, with
  blocked and error metadata paths.
- Added voice runtime summaries for runtime, TTS, STT, microphone, playback, local-only policy,
  and process-execution posture.
- `ApplicationController` exposes voice session status, pipeline summary, runtime summary, trace
  summaries, and readiness details.
- `DesktopShellViewModel` forwards only QML-safe strings, string lists, and booleans.
- Settings extends the existing read-only Voice Readiness section with session/runtime/pipeline
  metadata and no voice controls.
- Tests cover deterministic pipeline transitions, blocked/error metadata behavior, controller
  exposure, view-model QML-safe exposure, and disabled runtime posture.

Still out of scope:

- Microphone access, audio playback, Piper execution, Whisper execution, subprocess/process
  launch, filesystem/system actions, downloads, cloud calls, API keys, voice buttons, activation
  controls, and autonomous voice loops.

### Phase 12.0-12.2: Voice Boundary And TTS/STT Planning Skeleton

Completed. Adds a disabled-by-default voice architecture boundary and readiness surface without
enabling real audio input, audio output, or voice runtime execution.

Scope:

- Added value-only voice capability, provider descriptor/status, runtime mode, request, response,
  and readiness report metadata.
- Added `ITextToSpeechProvider`, `ISpeechToTextProvider`, `NullTextToSpeechProvider`, and
  `NullSpeechToTextProvider`.
- Null TTS and STT providers return deterministic safe refusals and do not record, play,
  synthesize, transcribe, launch processes, touch filesystems, download assets, call cloud
  providers, or use API keys.
- `ApplicationController` exposes disabled voice runtime mode, readiness summary/checks,
  capability summaries, and TTS/STT status summaries.
- `DesktopShellViewModel` forwards only QML-safe strings, string lists, and booleans.
- Settings shows a read-only Voice Readiness section with no record, speak, playback, setup, or
  model-management controls.
- Tests cover null TTS/STT behavior, readiness summaries, controller exposure, and view-model
  QML-safe exposure.

Still out of scope:

- Microphone access, audio playback, Whisper/Piper execution, subprocess/process launch,
  filesystem/system actions, downloads, cloud calls, API keys, voice buttons, record/speak buttons,
  and broad UI redesign.

### Phase 11.7-11.9: Local AI Usability Checkpoint And Runtime QA

Completed. Checkpoints the current local AI user flow and runtime QA surface without adding new
product features or runtime authority.

Scope:

- Reviewed the full local AI flow for unavailable Ollama, missing/invalid models, disabled local
  chat inference, disabled/enabled streaming, permission/safety blocking, fake non-streaming
  success, and fake streaming success.
- Added focused coverage for safety-policy blocking, stream-error preview cleanup, stable JSON
  persistence of selected model/local chat/streaming settings, and expanded QML-safe exposure
  checks for local AI/runtime properties.
- Documented the completed local AI scope, current user flow, known limitations, safety
  guardrails, and Phase 12 readiness criteria in `docs/PHASE_11_CHECKPOINT.md`.

Still out of scope:

- New product features, model pulls/deletes/installs, cloud providers, API keys, filesystem or
  system actions, tools/plugins, subprocess launch, UI redesign, and autonomous behavior.

### Phase 11.3-11.6: Local AI Experience Stabilization And Runtime UX Polish

Completed. Stabilizes the explicit local AI chat path and polishes runtime/model-selection UX
without expanding execution scope.

Scope:

- Local chat inference remains opt-in and guarded by selected/effective model validation,
  loopback-only Ollama endpoint policy, runtime permission metadata, and runtime safety metadata.
- Local inference refusals and errors now surface safer user-facing summaries for missing models,
  invalid discovered-model selections, blocked endpoints, permission/safety denials, unavailable
  local clients, request failures, timeouts, and invalid responses.
- Streaming preview text is exposed only while a stream is active; completed stream text is cleared
  from the live preview and persisted once through normal chat history as the final assistant
  message.
- Runtime state reports busy, idle, or error/refused/blocked outcomes through QML-safe strings.
- Settings shows selected-model status/summary, discovered Ollama model metadata, recommendation
  summaries near selection, read-only requirement metadata, and unavailable model-management action
  summaries.
- Tests cover disabled local inference UX, missing/invalid model summaries, streaming
  reset/finalization, safe refusal/error responses, and view-model exposure without requiring a
  real Ollama service.

Still out of scope:

- Cloud providers, API keys, model downloads, pulls, deletes, installs, endpoint expansion,
  filesystem/system actions, tools/plugins, subprocess launch, real cancellation controls,
  autonomous loops, and broad model-management workflows.

### Phase 11.0-11.2: Lightweight Local Model Management Readiness

Completed. Adds a safe metadata/readiness boundary for future local model management without
performing model management operations.

Scope:

- Added `ModelManagementAction`, `ModelManagementStatus`, `ModelManagementRequest`,
  `ModelManagementResult`, `ModelRecommendation`, `ModelRequirementSummary`,
  `IModelManagementService`, and `StaticModelManagementService`.
- The static service returns deterministic local model recommendations and approximate descriptive
  RAM/disk requirement metadata.
- Pull, delete, and install requests are evaluated as unavailable/not implemented metadata only.
- `ApplicationController` exposes management status, summary, action availability,
  recommendations, and requirement summaries.
- `DesktopShellViewModel` exposes QML-safe strings and string lists only.
- Settings shows a read-only Model Management readiness section with installed count through the
  existing Ollama model count, selected/effective model summary, recommendations, requirements, and
  unavailable action metadata.
- Tests cover deterministic recommendations, action-unavailable behavior, controller exposure, and
  view-model exposure.

Still out of scope:

- Model downloads, pulls, deletes, installs, filesystem/system scans or actions, subprocess/process
  launch, cloud calls, API keys, tools/plugins, autonomous loops, and broad model-management
  workflows.

### Phase 10.6-10.8: Model Selection And Local Runtime Management UX

Completed. Adds lightweight model selection and local runtime status UX on top of existing
loopback-only Ollama health/discovery and explicit local inference boundaries.

Scope:

- Settings shows discovered local Ollama models when `/api/tags` metadata is available.
- Users can select a discovered local model from Settings; the selected model persists through
  `AppSettings`.
- The controller validates selected/effective models against discovered metadata when available
  and exposes Missing, Fallback, Available, Unverified, and Invalid states.
- Model summaries include QML-safe name, size when available, modified date when available, and a
  Local Only status.
- Settings shows Ollama endpoint, health, model count, selected model status, selected metadata,
  local chat inference enablement, streaming enablement, runtime badge, inference state, and
  streaming status.
- Tests cover selected-model persistence, discovered-model validation, invalid selected-model
  fallback/refusal behavior, view-model exposure, QML-safe model summaries, and fake/no-Ollama
  runtime paths.

Still out of scope:

- Model downloads, pulls, deletes, installs, endpoint expansion, cloud providers, API keys,
  filesystem/system actions, tools/plugins, subprocess launch, autonomous loops, and broad model
  management actions.

### Phase 10.3-10.5: Streaming Chat Boundary And Live Response UX

Completed. Adds guarded local-only streaming for chat responses without changing the default safe
chat path.

Scope:

- Added a persisted local inference streaming opt-in; default is disabled.
- Added chunk/status/error/cancellation/malformed-chunk metadata for streaming results.
- Added an Ollama local stream client for loopback-only `/api/generate` streaming with manual
  redirect policy, no cloud endpoints, no API keys, no downloads/pulls/deletes, and no subprocess
  launch.
- Chat uses streaming only when local chat inference is enabled, streaming is enabled and
  available, an effective local model is valid, the endpoint is loopback HTTP, and
  permission/safety checks pass. Otherwise the non-streaming/local-safe fallback path remains.
- Streaming chunks accumulate into one assistant response, expose live text through the desktop
  view model, and persist the final assistant message once.
- ChatPanel shows live streaming text while active, and Settings exposes streaming opt-in/status
  without model management UI.

Still out of scope:

- Cloud provider calls, API keys, model downloads/pulls/deletes, broad model management UI,
  tools/plugins, filesystem/system actions, subprocess launch, autonomous loops, and UI redesign.

### Phase 10.0-10.2: Explicit Chat-To-Ollama Routing

Completed. Adds explicit opt-in chat routing to the controlled local inference boundary while
preserving the existing local-safe chat path by default.

Scope:

- Added a persisted local chat inference setting; default is disabled.
- Desktop view model exposes QML-safe local chat routing status and summary.
- Chat may call `runLocalInference` only when local chat inference is enabled, an effective local
  model is available and valid against discovered metadata when present, the Ollama endpoint is
  local loopback HTTP, and runtime permission/safety gates pass.
- User chat messages are still appended before routing, and exactly one assistant response is
  appended from either the inference result or a safe local refusal/error.
- Settings exposes a guarded local chat inference toggle, and ChatPanel shows the active routing
  status/model/runtime badge without model-management or provider setup controls.

Still out of scope:

- Streaming chat, model downloads, pulls, deletes, broad model management UI, cloud calls, API
  keys, subprocess launch, filesystem/system actions, tools/plugins, and autonomous loops.

### Phase 9.6-9.8: Model Selection, Runtime UX State, And Streaming Skeleton

Completed. Adds selected local model metadata, runtime inference UX state, and a disabled
streaming skeleton while keeping chat routing and local execution explicitly guarded.

Scope:

- Added a persisted selected local model setting.
- Controller resolves an effective local model from an explicit request, selected model, or safe
  discovered-model fallback when metadata is available.
- Selected model validation uses discovered local model metadata when available and rejects known
  invalid selections before invoking inference.
- Runtime UX metadata now includes idle/busy/error state, active local runtime/model badge,
  last-response latency summary, and existing trace summaries.
- Added `LocalInferenceStreamChunk`, `LocalInferenceStreamStatus`, `LocalInferenceStreamResult`,
  `ILocalInferenceStreamClient`, and deterministic disabled stream client behavior.
- Desktop view model exposes QML-safe strings, lists, and booleans only.
- Settings and chat surfaces show model/runtime readiness metadata without adding model-management
  actions or routing chat through Ollama.

Still out of scope:

- Model downloads, pulls, deletes, broad model management UI, automatic chat-to-Ollama routing,
  real token streaming UI, cloud calls, API keys, subprocess launch, filesystem/system actions,
  tools/plugins, and autonomous loops.

### Phase 9.3-9.5: Controlled Local Inference Boundary

Completed. Adds the first explicit local-only inference path behind the Ollama/local inference
boundary while keeping default behavior permission-blocked and non-autonomous.

Scope:

- Added `LocalInferenceRequest`, `LocalInferenceResponse`, `LocalInferenceStatus`,
  `LocalInferenceOptions`, `LocalInferenceError`, and `LocalInferenceTrace`.
- Added `ILocalInferenceClient`, deterministic `NullLocalInferenceClient`, and
  `OllamaLocalInferenceClient`.
- Restricted Ollama inference to local loopback HTTP and non-streaming `/api/generate`.
- Controller local inference calls evaluate runtime permission and safety policy before invoking
  the client.
- Desktop view model exposes QML-safe status, summary, last-response summary, and trace strings
  only.
- Tests cover null refusal, blank prompt rejection, missing/unavailable model rejection, injected
  fake success, QML-safe exposure, and default permission blocking.

Still out of scope:

- Cloud inference, API keys, provider routing automation, streaming, model pull/download/delete,
  subprocess launch, filesystem/system actions, tools/plugins, autonomous loops, and UI redesign.

### Phase 1: Desktop Alpha Foundation

Created the native Qt/QML desktop shell, C++ core interfaces, local provider boundary, runtime memory store, mode handling, and initial documentation.

### Phase 2: Provider And Application Core Foundation

Established provider, controller, chat session, settings, plugin, integration, and context interfaces without adding real network providers.

### Phase 3: SQLite Memory Foundation

Added SQLite-backed memory persistence through `SQLiteMemoryStore` while keeping the `IMemoryStore` contract storage-backend independent.

### Phase 3.1: Persistence Stabilization

Added `JsonSettingsStore`, kept settings and memory persistence separate, expanded SQLite persistence tests, and documented storage boundaries.

### Phase 3.1.5: AI Context & Agent Instruction Layer

Adds lightweight repository-local AI instructions and context documents for Codex, Claude, ChatGPT, and similar coding agents.

Scope:

- Prompt compression.
- Stable architecture guardrails.
- Phase status memory.
- Decision history.
- Agent working instructions.

This phase must not change application logic, SQLite logic, QML architecture, build behavior, or dependencies.

### Phase 3.2: Persistent Chat History Preparation

Added a dedicated chat history persistence boundary and SQLite-backed implementation.

Scope:

- `IChatHistoryStore`.
- `SQLiteChatHistoryStore`.
- Separate chat history database.
- Optional controller persistence loading and appending.
- Runtime chat remains available if persistence is unavailable.
- Tests for chat history storage and controller integration.

Chat history persistence is separate from key-value memory storage.

### Phase 3.3: Chat History UX and Lifecycle Controls

Completed. Adds minimal chat history lifecycle UX on top of the Phase 3.2 persistence boundary.

Scope:

- Generic chat history availability/status in the desktop view model.
- Status bar and chat panel visibility for chat history state.
- Clear-chat confirmation before clearing runtime and persistent history.
- Startup-loaded messages exposed through `ChatMessageListModel`.
- Tests for desktop view-model chat history status and restored messages.

Current limitation:

- Chat history is one local transcript.
- No multi-conversation/thread support.
- No encryption, export, or pruning.

## Current Alignment Work

### Phase 3.4: Cross-platform Architecture Readiness

Completed. Added architecture boundaries and storage maintenance surfaces without platform-specific runtime integrations.

Scope:

- Added platform boundary interfaces:
  - `IPathProvider`
  - `IPlatformService`
  - `INotificationService` (lightweight placeholder)
  - `ISystemIntegrationService` (lightweight placeholder)
- Added default `StandardPathProvider` with Qt `QStandardPaths` ownership for:
  - settings path
  - memory database path
  - chat history database path
- Added storage maintenance controls:
  - clear memory store
  - clear chat history store with runtime-safe fallback
  - settings persistence remains separate and unaffected
- Added controller/view-model generic maintenance status surfaces for QML.
- Added settings page controls for local memory/chat clear actions with confirmation dialogs.
- Added tests for path behavior, clear behavior, unavailable stores, and settings isolation.

Candidate future interfaces:

- `IPlatformService`
- `IPathProvider`
- `INotificationService`
- `ISystemIntegrationService`

### Phase 3.5: Pre-agent Architecture Audit and Release Checkpoint

Completed. Stabilization checkpoint before Phase 4.

Scope:

- Full architecture consistency review across:
  - core boundaries
  - desktop/view-model boundaries
  - persistence separation
  - platform boundary separation
  - QML exposure safety
  - test coverage gaps
  - documentation accuracy
- Small safe fixes only:
  - stale wording and status consistency
  - documentation updates and phase references
  - minor focused tests where obvious behavior needed coverage
- Explicitly no Phase 4 runtime implementation.

Known limitations (unchanged):

- No networking or API key handling.
- No real provider integrations.
- No plugin loading.
- No privileged automation.
- No multi-conversation/thread chat model.
- No encryption, export, or pruning.

Phase 4 readiness criteria:

- Core boundaries remain interface-driven and portable.
- QML receives only generic status/action surfaces.
- Settings, memory, and chat history remain separate persistence categories.
- Path ownership remains behind `IPathProvider`.
- Full test suite and formatting checks pass.

Must remain out of scope until explicitly approved:

- Agent core/tool runtime behavior.
- Real external provider integration.
- Platform-specific automation/integration implementation.

## Completed Agent Foundation

### Phase 4: Agent Core & Tool System

Completed through Phase 4.11 and remains local-safe.

### Phase 4.0: Agent Core Planning and Minimal Runtime Skeleton

Completed. Introduced minimal local-safe agent runtime boundaries.

Scope:

- Added `IAgentRuntime`, `AgentRequest`, `AgentResponse`, `AgentStatus`, and capability descriptors.
- Added deterministic `NullAgentRuntime`:
  - local-only
  - no networking
  - no tool execution
  - no filesystem/system-modifying actions
- Kept provider and agent concepts separate:
  - `IChatProvider` remains chat-response oriented
  - `IAgentRuntime` is reserved for future orchestration boundaries
- Minimal controller/view-model surfaces:
  - generic agent status
  - deterministic placeholder agent request/response
  - no raw runtime object exposure to QML

Out of scope (unchanged):

- Real tool execution.
- OS/platform automation.
- Networking/API keys.
- Real provider integrations.
- Plugin loading.
- Sandbox/permission enforcement runtime.

### Phase 4.1: Tool Descriptor and Registry Skeleton

Completed. Added metadata-only tool modeling and registry boundaries.

Scope:

- Added tool descriptor abstractions:
  - `ToolDescriptor`
  - `ToolParameterDescriptor`
  - `ToolRiskLevel`
  - `ToolExecutionMode`
- Added registry boundary:
  - `IToolRegistry`
  - `InMemoryToolRegistry`
  - deterministic `register/list/find` behavior
  - duplicate-id rejection
- Connected runtime metadata only:
  - `NullAgentRuntime` publishes `availableTools()` metadata
  - no tool execution path
- Minimal controller/view-model metadata exposure:
  - available tool count
  - available tool ids
  - no mutable registry exposure in QML

Out of scope (unchanged):

- Any real tool execution.
- Filesystem/system mutation.
- Shell/process execution.
- Platform automation.
- Networking/API keys.
- Provider integrations.
- Plugin loading.
- Sandbox/permissions runtime.

### Phase 4.2: Tool Invocation Planning Boundary

Completed. Added metadata-only tool invocation planning without execution.

Scope:

- Added value-only structures for proposed tool invocation intent:
  - `ToolInvocationPlan`
  - `PlannedToolInvocation`
  - `ToolInvocationArgument`
  - `ToolInvocationPlanStatus`
- `IAgentRuntime` can produce metadata-only invocation plans through `plan()`.
- Invocation plans remain separate from `ToolDescriptor` registry metadata.
- `NullAgentRuntime` produces deterministic fake plans from registered tool metadata.
- Controller/view-model exposure is limited to latest plan status and summary strings.
- Tests cover deterministic output, empty-tool state, unknown requested tool ids, metadata
  preservation, and controller/view-model status exposure.

Still out of scope:

- Tool execution runtime.
- Shell/process execution.
- Filesystem/system mutation.
- Platform automation.
- Networking/API keys.
- Provider integrations.
- Plugin loading.
- Permission prompts that approve real execution.
- Sandbox runtime.

Reference plan:

- `docs/PHASE_4_2_PLAN.md`

### Phase 4.3: Approval and Permission Metadata Skeleton

Completed. Added approval/permission metadata evaluation for planned tool invocations without
execution or sandboxing.

Scope:

- Added approval metadata abstractions:
  - `ApprovalStatus`
  - `ApprovalDecision`
  - `PermissionDescriptor`
  - `ToolApprovalRequest`
- Added approval policy boundary:
  - `IApprovalPolicy`
  - `StaticApprovalPolicy`
- Approval policy evaluates `ToolInvocationPlan` data only.
- Low-risk metadata-only plans can be marked as not requiring approval.
- Medium/high-risk planned invocations are represented as requiring approval.
- Explicit approved/denied policy states are representable as metadata.
- Controller/view-model exposure is limited to latest approval status and summary strings.
- Tests cover safe plan behavior, risky plan approval requirements, approved/denied state
  representation, empty plan behavior, deterministic ordering, and controller/view-model status
  exposure.

Still out of scope:

- Tool execution runtime.
- Shell/process execution.
- Filesystem/system mutation.
- Platform automation.
- Networking/API keys.
- Provider integrations.
- Plugin loading.
- Sandbox runtime.
- Permission prompts that approve real execution.

### Phase 4.4: Sandbox and Capability Boundary Skeleton

Completed. Added sandbox/capability metadata evaluation for planned and approved tool invocations
without execution or real sandbox enforcement.

Scope:

- Added sandbox/capability metadata abstractions:
  - `CapabilityDescriptor`
  - `SandboxStatus`
  - `SandboxCapabilityDecision`
  - `SandboxEvaluationResult`
- Added sandbox policy boundary:
  - `ISandboxPolicy`
  - `StaticSandboxPolicy`
- Sandbox policy evaluates `ToolInvocationPlan` and `ApprovalDecision` data only.
- Low-risk metadata-only plans can be marked as allowed by metadata capability policy.
- Unknown or high-risk capabilities are represented as denied unless future explicit support is
  added.
- Approval is an input to sandbox evaluation but does not grant runtime capability.
- Controller/view-model exposure is limited to latest sandbox status and summary strings.
- Tests cover metadata-only capability allowance, unknown capability denial, empty plan behavior,
  approved-but-not-capable blocking, deterministic ordering, and controller/view-model status
  exposure.

Still out of scope:

- Tool execution runtime.
- Shell/process execution.
- Filesystem/system mutation.
- Platform automation.
- Networking/API keys.
- Provider integrations.
- Plugin loading.
- Real sandbox runtime or OS permission enforcement.
- Privileged automation.

### Phase 4.5: Execution Boundary Skeleton

Completed. Added a deterministic placeholder execution boundary for planned, approved, and
sandbox-allowed tool invocation metadata without real execution.

Scope:

- Added execution boundary abstractions:
  - `ToolExecutionRequest`
  - `ToolExecutionResult`
  - `ToolExecutionStatus`
  - `IToolExecutor`
  - `NullToolExecutor`
- `NullToolExecutor` returns placeholder-only deterministic results.
- Controller flow now routes metadata through:
  - planning
  - approval
  - sandbox capability evaluation
  - placeholder execution boundary
- Approved and sandbox-allowed plans can produce placeholder success.
- Denied, unapproved, sandbox-blocked, empty, or unknown-tool plans remain blocked or safely
  rejected at the placeholder boundary.
- Controller/view-model exposure is limited to latest execution status and summary strings.
- Tests cover placeholder success, blocked execution, empty plans, unknown tools, deterministic
  results, and controller/view-model status exposure.

Still out of scope:

- Real tool execution.
- Shell/process execution.
- Subprocess execution.
- Filesystem/system mutation.
- Platform automation.
- Networking/API keys.
- Provider integrations.
- Plugin loading.
- Real sandbox runtime or OS permission enforcement.
- Privileged automation.

### Phase 4.6: Agent Runtime Pipeline Stabilization

Completed. Consolidated the metadata-only agent runtime pipeline result without expanding tool
capabilities.

Scope:

- Added a value-based aggregate pipeline result for:
  - planning status
  - approval status
  - sandbox status
  - placeholder execution status
  - deterministic summary text
- Centralized safe stage and pipeline summaries for controller and view-model exposure.
- Clarified `ApplicationController::runAgentRequest` routing through the full Phase 4 pipeline:
  - registry
  - planning
  - approval
  - sandbox capability metadata evaluation
  - placeholder execution boundary
- Added generic pipeline status/summary exposure to the desktop view model.
- Added tests for successful placeholder routing, approval-blocked routing, sandbox-blocked routing,
  empty-plan routing, unknown-tool routing, deterministic summary/status output, and view-model
  status exposure.

Still out of scope:

- Real tool execution.
- Shell/process execution.
- Subprocess execution.
- Filesystem/system mutation.
- Platform automation.
- Networking/API keys.
- Provider integrations.
- Plugin loading.
- Real sandbox runtime or OS permission enforcement.
- Privileged automation.

### Phase 4.7: Runtime Context and Tool Session Skeleton

Completed. Added deterministic local runtime context/session ownership without adding execution or
OS interaction.

Scope:

- Added runtime context/session abstractions:
  - `AgentRuntimeContext`
  - `RuntimeSession`
  - `RuntimeSessionId`
  - `RuntimeContextStatus`
- Runtime context safely aggregates value metadata from the latest pipeline result:
  - active planned tool ids
  - approval metadata
  - sandbox metadata
  - placeholder execution metadata
  - deterministic session id and revision
- `ApplicationController` owns the runtime session and attaches each completed pipeline result.
- Desktop view model exposes generic read-only runtime context status, summary, session id, and
  active planned tool ids.
- Tests cover deterministic context creation, pipeline result attachment, planned tool ordering,
  reset behavior, controller exposure, and view-model exposure.

Boundary clarification:

- Runtime context is not execution.
- Runtime context is not persistence.
- Runtime context is not planning.
- Runtime context is not approval policy.
- Runtime context is not sandbox enforcement.
- Runtime context is not plugin loading.

Still out of scope:

- Real tool execution.
- Shell/process execution.
- Subprocess execution.
- Filesystem/system mutation.
- Platform automation.
- Networking/API keys.
- Provider integrations.
- Plugin loading.
- Runtime context persistence.
- Real sandbox runtime or OS permission enforcement.
- Privileged automation.

### Phase 4.8: Agent Activity Log and Audit Trail Skeleton

Completed. Added in-memory metadata-only activity logging for agent pipeline observability.

Scope:

- Added activity/audit metadata abstractions:
  - `AgentActivityEntry`
  - `AgentActivityType`
  - `AgentActivityStatus`
  - `AgentActivityLog`
- Activity entries are value-based and deterministic.
- Activity log is in-memory only and has deterministic sequence ordering.
- `ApplicationController` appends metadata events for:
  - request received
  - plan created
  - approval evaluated
  - sandbox evaluated
  - placeholder execution evaluated
  - pipeline completed or blocked
- Desktop view model exposes generic read-only activity count and latest summary.
- Tests cover deterministic ordering, clear behavior, successful pipeline logging, blocked pipeline
  logging, and controller/view-model exposure.

Boundary clarification:

- Activity logging is metadata only.
- Activity logging is not persistence.
- Activity logging is not execution.
- Activity logging is not sandbox enforcement.
- Activity logging is not provider integration.
- Activity logging is not plugin loading.
- Future durable audit, export, redaction, pruning, and security review are later-phase work.

Still out of scope:

- Real tool execution.
- Shell/process execution.
- Subprocess execution.
- Filesystem/system mutation.
- Platform automation.
- Networking/API keys.
- Provider integrations.
- Plugin loading.
- Activity log persistence/export.
- Real sandbox runtime or OS permission enforcement.
- Privileged automation.

### Phase 4.9: Agent Pipeline UI Visibility

Completed. Added read-only QML visibility for the metadata-only agent pipeline.

Scope:

- Dashboard shows latest pipeline status and summary.
- Dashboard shows runtime context status and summary.
- Dashboard shows active planned tool ids when present.
- Dashboard shows activity count and latest activity summary.
- Desktop view-model remains the QML boundary and exposes simple read-only values only.
- Tests cover visible pipeline values, activity values, successful request updates, blocked
  pipeline updates, and QML-facing property safety.

Boundary clarification:

- UI visibility is read-only.
- UI visibility is not execution.
- UI visibility is not approval.
- UI visibility is not persistence.
- UI visibility is not provider integration.
- UI visibility is not plugin loading.

Still out of scope:

- Real tool execution.
- Shell/process execution.
- Subprocess execution.
- Filesystem mutation.
- Networking/API keys.
- Provider integrations.
- Plugin loading.
- Execution buttons or approval controls.
- Activity log persistence/export.
- Real sandbox runtime or OS permission enforcement.
- Privileged automation.

### Phase 4.10: Architecture Checkpoint and Cleanup

Completed. Stabilized Phase 4 architecture documentation and small consistency gaps before Phase 5.

Scope:

- Reviewed provider vs agent separation.
- Reviewed registry, planning, approval, sandbox metadata, and placeholder execution separation.
- Reviewed runtime context/session ownership.
- Reviewed activity log scope.
- Reviewed QML read-only exposure.
- Added Phase 4 checkpoint documentation for completed scope, limitations, Phase 5 readiness, and
  explicit out-of-scope work.
- Added focused tests for summary fallback and reset behavior.
- Clarified minor Dashboard labels without adding controls.

Boundary clarification:

- This checkpoint adds no product features.
- This checkpoint does not start Phase 5 implementation.
- Real execution, approval UX, sandbox runtime, provider integrations, plugin loading,
  networking/API keys, filesystem/system actions, shell/process launch, subprocess launch, and
  privileged automation remain out of scope.

### Phase 4.11: AI Orchestration Planning Checkpoint

Completed. Documented future model/provider routing direction before Phase 5 UI work.

Scope:

- Added future `ModelRouter` and `RoutingPolicy` planning.
- Documented provider capability profiles and task classification.
- Documented local/cloud fallback and device-aware model selection.
- Documented user routing modes: Auto, Fast, Balanced, Quality, Local Only, Cloud Allowed, and
  Battery Saver.
- Documented future model-management UI metadata: installed/downloadable/recommended models,
  RAM/disk requirements, and local/cloud badges.
- Documented privacy-aware routing: sensitive data should prefer local-only, and cloud use must
  require explicit user permission.
- Clarified that this planning remains separate from current Phase 4 runtime, `IChatProvider`,
  `IAgentRuntime`, tool execution, and future UI model-management screens.

Still out of scope:

- Real provider integrations.
- Networking/API keys.
- Ollama/OpenAI/Anthropic integration.
- Model downloads or model execution.
- Plugin loading.
- Filesystem/system actions.
- Real tool execution.
- Runtime behavior changes.

## Active UI Foundation

### Phase 5: Advanced UI/UX & Motion System

Started with Phase 5.0 design-system foundation and remains lightweight/local-safe.

### Phase 5.0: UI/UX Planning and Design System Foundation

Completed. Added UI planning and a minimal QML design-token layer without changing runtime
behavior.

Scope:

- Added `docs/UI_UX_PLAN.md`.
- Documented Linux/Fedora KDE-friendly, cross-platform Qt/QML design direction.
- Documented future motion and assistant visual guidelines without implementation.
- Added `ui/qml/theme/SentinelTheme.qml` for palette, spacing, radius, and typography tokens.
- Registered the theme singleton in the existing QML module.
- Refactored existing QML styling to consume shared tokens where safe.

Still out of scope:

- Real tool execution.
- Approval UX that triggers actions.
- Sandbox runtime.
- Networking/API keys.
- Provider integrations.
- Model downloads or model execution.
- Plugin loading.
- Filesystem/system actions.
- Heavy animation or particle assistant visual implementation.
- Broad QML rewrite.

### Phase 5.1: Motion and Interaction Foundation

Completed. Added lightweight motion tokens and subtle interaction polish without changing runtime
behavior.

Scope:

- Added duration/easing motion tokens to `SentinelTheme.qml`.
- Added lightweight hover/focus behavior for shell navigation.
- Added `SentinelButton.qml` for tokenized command button hover/focus states.
- Added focus-ring transitions for text fields.
- Added low-cost page opacity transition hooks.
- Extended UI/UX docs with motion philosophy and future assistant visual boundaries.

Still out of scope:

- Heavy animations.
- Particle systems.
- Assistant-face rendering.
- Custom OpenGL/Vulkan rendering systems.
- Provider/model execution.
- Networking/API keys.
- Plugin loading.
- Filesystem/system actions.
- Broad QML rewrite.

### Phase 5.2: Adaptive Layout and Responsive Shell Foundation

Completed. Added compact/normal/wide layout behavior while preserving the current QML/view-model
boundaries.

Scope:

- Added layout breakpoint and responsive spacing tokens to `SentinelTheme.qml`.
- Lowered the desktop shell minimum width for narrower resizable windows.
- Added compact shell navigation behavior with stable eliding labels.
- Updated header/status surfaces to avoid crowding at compact widths.
- Updated dashboard, chat, memory, and settings layouts to wrap with lightweight `GridLayout`
  column changes.
- Extended UI/UX docs with adaptive layout guidance for Linux/Fedora KDE and macOS.

Still out of scope:

- Heavy animations.
- Particle systems.
- Assistant-face rendering.
- Custom OpenGL/Vulkan rendering systems.
- Provider/model execution.
- Networking/API keys.
- Plugin loading.
- Filesystem/system actions.
- Execution-related UI or approval actions.
- Broad QML rewrite.

### Phase 5.3: Component Consistency and Visual QA Pass

Completed. Normalized small visual patterns and documented manual QA expectations without changing
runtime behavior.

Scope:

- Added `SentinelTextField.qml` for shared text-field styling and focus behavior.
- Added `InfoRow.qml` for read-only label/value status rows.
- Normalized command button and input heights through shared theme tokens.
- Normalized section heading wrapping, metric card padding, dashboard status rows, settings status
  rows, and compact input usage.
- Added `docs/UI_QA_CHECKLIST.md` for compact/normal/wide manual screen checks and platform notes.

Still out of scope:

- Heavy animations.
- Particle systems.
- Assistant-face rendering.
- Custom OpenGL/Vulkan rendering systems.
- Provider/model execution.
- Networking/API keys.
- Plugin loading.
- Filesystem/system actions.
- Execution or approval controls.
- Automated visual driving or screenshot QA.

### Phase 5.4: Workspace UX Integration

Completed. Translated the useful direction from the former `lovable-tasarim` UI/UX reference into
native Qt/QML while keeping React, Vite, Tailwind, Node tooling, and WebView integration out of the
production app.

Scope:

- Added mode-aware graphite/glass/cyan visual tokens in `SentinelTheme.qml`.
- Added ambient Qt/QML shell background and central `WorkspacePresence` component.
- Refined the shell into a left status/navigation rail, central workspace/presence area, and right
  chat/interaction panel.
- Added lightweight QML breathing/orbit/opacity motion for workspace state cues.
- Added mode-aware visual behavior for Companion, Focus, Mission, System, Minimal, and Tactical
  modes.
- Preserved existing view models, controllers, providers, memory stores, and execution boundaries.

Follow-up visual migration:

- Move the completed QML workspace closer to the translated Sentinel visual identity.
- Emphasize cinematic presence-first composition, softer translucent surfaces, reduced dashboard
  density, larger central AI presence, thinner visual hierarchy, and more negative space.
- Keep the implementation native Qt/QML and preserve all no-execution boundaries.

Still out of scope:

- React architecture or web runtime integration.
- Real tool execution.
- Approval actions or sandbox runtime.
- Filesystem/system mutation or automation.
- Provider/model execution, networking/API keys, voice pipeline, hardware integration, plugin
  loading, advanced particle systems, assistant-face rendering, or Qt Quick 3D.

### Phase 5.4.5: Architecture and UI Risk Audit

Completed. This phase audited architecture and UI risk before Phase 5.5 without adding product
features or runtime behavior.

Scope:

- Re-audit C++ core boundaries, desktop view-model exposure, persistence separation,
  provider/agent separation, tool planning/approval/sandbox/execution boundaries, runtime context,
  activity log scope, QML component structure, design-token usage, and the Phase 5.4 workspace UX
  translation.
- Fix only small safe issues such as stale docs, naming inconsistencies, duplicated simple QML
  styling, fragile bindings, minor label clarity, and checklist gaps.
- Keep any external design references outside the production app.

Known risks to monitor before Phase 5.5:

- `SentinelTheme.qml` now owns more visual helpers; avoid turning mode-aware visual helpers into
  product logic.
- `WorkspacePresence` and `Atmosphere` add continuous lightweight animation; keep them cheap and
  avoid advanced particle/assistant-face systems.
- Compact/normal/wide manual QA is still important because automated screenshot driving is not in
  place.
- `qmllint` may be unavailable in some developer environments; build-time QML cache compilation
  and startup smoke checks should remain fallback verification.

Phase 5.5 readiness criteria:

- Full tests, formatting, and available QML verification pass.
- QML remains presentation-only behind `DesktopShellViewModel`.
- No UI control implies real execution, approval, networking, provider/model execution, plugin
  loading, filesystem/system actions, voice, hardware integration, WebView, React, Node, Tailwind,
  Vite, assistant-face rendering, advanced particles, or Qt Quick 3D.

### Phase 5.5: Visual Identity Reconstruction

Completed. Reconstructed the Dashboard/Core visual shell around the translated Sentinel visual
identity while preserving Sentinel's native Qt/QML and C++ architecture boundaries.

Scope:

- Moved the main workspace from a dashboard-panel composition toward a cinematic presence-first
  scene.
- Added bottom floating dock navigation and ambient shell framing.
- Expanded QML visual primitives for a central orb, floating telemetry readouts, and dock-led
  navigation.
- De-emphasized rigid metric/status panels on the Dashboard/Core page.
- Refined the right AI bridge surface to feel more translucent, spacious, and atmospheric.
- Preserved `DesktopShellViewModel`, provider/agent separation, persistence boundaries, and the
  Phase 4 no-execution architecture.

Still out of scope:

- React/Vite/Tailwind/Node/WebView integration.
- Provider/model execution, networking/API keys, real tool execution, approval actions, sandbox
  runtime, filesystem/system actions, plugin loading, voice, hardware integration, assistant-face
  rendering, advanced particles, heavy custom rendering, or Qt Quick 3D.

## Current Functional Architecture Work

### Phase 6.0: Functional Workspace and Model-Orchestration Skeleton

Completed. Added a metadata-only model/provider routing skeleton without adding provider
integration or model execution.

Scope:

- Added value descriptors for provider capability profiles, providers, models, routing modes, task
  classification, and route results.
- Added `IModelRouter` as a separate routing boundary.
- Added `StaticModelRouter` with deterministic local-only placeholder routing.
- Added minimal `ApplicationController` and `DesktopShellViewModel` read-only exposure for routing
  mode, routing status, and selected placeholder model/provider summary.
- Added focused tests for descriptor preservation, local-only routing, unknown task fallback,
  deterministic static routing, and controller/view-model exposure.

Still out of scope:

- Real provider integration.
- Ollama/OpenAI/Anthropic calls.
- API keys, networking, model downloads, or model execution.
- Real tool execution, approval actions, sandbox runtime, plugin loading, filesystem/system
  actions, broad UI redesign, or model-management UI.

### Phase 6.1: Routing Mode Settings and Persistence

Completed. Added a persisted routing mode preference while keeping model routing metadata-only.

Scope:

- Added routing mode preference to `AppSettings`.
- Persisted routing mode through the existing `JsonSettingsStore`.
- Kept the default privacy-safe: `Local Only`.
- Normalized invalid/unknown routing mode values back to `Local Only`.
- Added mutable metadata routing mode support to `IModelRouter`/`StaticModelRouter`.
- Added `ApplicationController` and `DesktopShellViewModel` route metadata updates when routing
  mode changes.
- Added a minimal Settings page routing mode selector plus read-only route status/summary.
- Added tests for defaults, persistence, invalid fallback, controller/view-model updates, and route
  summary changes.

Still out of scope:

- Real provider integration.
- Ollama/OpenAI/Anthropic calls.
- API keys, networking, model downloads, or model execution.
- Real tool execution, approval actions, sandbox runtime, plugin loading, filesystem/system
  actions, broad UI redesign, provider setup UI, or model-management UI.

### Phase 6.2: Provider Catalog Metadata Skeleton

Completed. Added a deterministic metadata-only catalog for current and future providers/models.

Scope:

- Added `IProviderCatalog` and value catalog entries for providers and models.
- Added `StaticProviderCatalog` with Local Metadata Provider / Sentinel Local Placeholder as
  available local metadata.
- Added Ollama Local, OpenAI Cloud, and Anthropic Cloud placeholders as not configured.
- Captured availability, local/cloud classification, supported task metadata, privacy labels, and
  rough RAM/disk hints.
- Seeded `StaticModelRouter` defaults from available catalog metadata only.
- Exposed read-only provider catalog count and summaries through `ApplicationController` and
  `DesktopShellViewModel`.
- Added a minimal Settings page provider catalog section with text-only local/cloud/status
  summaries.
- Added tests for deterministic catalog entries, local/cloud classification, unavailable cloud
  placeholders, local-only/cloud-placeholder route exclusion, and QML-safe exposure.

Still out of scope:

- Real provider integration.
- Ollama/OpenAI/Anthropic calls.
- API keys, endpoints, networking, model downloads, or model execution.
- Provider setup UI, API key fields, download buttons, model-management actions, real tool
  execution, approval actions, sandbox runtime, plugin loading, filesystem/system actions, or broad
  UI redesign.

### Phase 6.3: Capability Graph and Task Planner Skeleton

Completed. Added deterministic high-level task planning metadata without adding provider/model/tool
execution.

Scope:

- Added value-only task planning metadata:
  - `CapabilityNode`
  - `CapabilityGraph`
  - `TaskPlan`
  - `PlannedTaskStep`
  - `TaskPlanStatus`
- Added `ITaskPlanner` and deterministic `StaticTaskPlanner`.
- Planner consumes task classification, routing mode, provider/model catalog availability,
  local/cloud metadata, privacy sensitivity, and resource hints.
- Sensitive/private tasks require local metadata.
- Unknown tasks use a safe local metadata fallback.
- Cloud-allowed planning falls back to local metadata while cloud catalog entries remain not
  configured.
- Unavailable providers/models are not selected for executable routes.
- `ApplicationController` and `DesktopShellViewModel` expose read-only latest task plan status,
  summary, and planned step count.
- Settings page shows minimal read-only task planning status.
- Tests cover deterministic planning, sensitive local planning, cloud-unavailable fallback, unknown
  fallback, blocked unavailable cloud metadata, step ordering, and controller/view-model exposure.

Still out of scope:

- Real provider integration.
- Ollama/OpenAI/Anthropic calls.
- API keys, endpoints, networking, model downloads, or model execution.
- Real tool execution, approval actions, sandbox runtime, plugin loading, filesystem/system
  actions, provider setup UI, executable model-management actions, or broad UI redesign.

### Phase 6.4: Agent System Skeleton

Completed. Added deterministic static agent metadata without autonomous behavior or runtime
execution.

Scope:

- Added value-only agent metadata:
  - `AgentDescriptor`
  - `AgentRole`
  - `AgentState`
  - `AgentPriority`
  - `AgentCapabilityProfile`
  - `AgentTaskAffinity`
  - `AgentRuntimeSnapshot`
- Added `IAgentRegistry` and deterministic `StaticAgentRegistry`.
- Registry exposes Atlas, Orin, Vela, Kaze, Nyx, and Sol as static metadata only.
- Agent metadata includes id, display name, role, capability summary, preferred task types,
  local/cloud affinity, privacy affinity, state, and priority.
- `StaticTaskPlanner` may annotate a plan with preferred agent metadata based on task affinity.
- `IModelRouter`, `ITaskPlanner`, `IAgentRuntime`, and `IChatProvider` remain separate boundaries.
- `ApplicationController` and `DesktopShellViewModel` expose read-only registered agent count,
  active agent summaries, and current preferred agent summary.
- Dashboard and Settings show text-only agent metadata without execution controls.
- Tests cover registry determinism, unique ids, task affinities, planner metadata interaction, and
  controller/view-model exposure.

Still out of scope:

- Real agent execution, autonomous loops, threads/background workers, provider integration,
  Ollama/OpenAI/Anthropic calls, API keys, networking, downloads, model execution, tool execution,
  memory writes, plugin loading, filesystem/system actions, or broad UI redesign.

### Phase 6.5: Memory Taxonomy and Semantic Metadata Skeleton

Completed. Added deterministic static memory taxonomy metadata without semantic memory execution or
changes to key-value memory persistence.

Scope:

- Added value-only memory taxonomy metadata:
  - `MemoryType`
  - `MemoryShardDescriptor`
  - `MemoryShardStatus`
  - `MemoryAffinity`
  - `MemoryRetentionPolicy`
  - `MemoryPrivacyLevel`
  - `MemoryRecallHint`
  - `MemoryAssociationDescriptor`
- Added `IMemoryCatalog` and deterministic `StaticMemoryCatalog`.
- Catalog exposes Episodic, Semantic, Procedural, Reflective, and Ambient categories as static
  metadata only.
- Memory metadata includes retention, privacy, recall hint, tags, task affinities, and simple
  association labels.
- `IMemoryStore` and `SQLiteMemoryStore` remain the explicit key-value memory persistence boundary.
- `StaticTaskPlanner` may annotate a plan with preferred memory affinity metadata.
- `ApplicationController` and `DesktopShellViewModel` expose read-only memory category count,
  memory taxonomy summaries, and current memory affinity summary.
- Memory and Settings pages show text-only memory taxonomy metadata without semantic search or graph
  execution controls.
- Tests cover catalog determinism, unique ids/types, retention/privacy preservation, planner
  affinity metadata, and controller/view-model exposure.

Still out of scope:

- Vector databases, embeddings, semantic search, autonomous memory writes, semantic recall
  execution, provider integration, Ollama/OpenAI/Anthropic calls, API keys, networking, downloads,
  model execution, tool execution, plugin loading, filesystem/system actions, replacing
  `IMemoryStore`/`SQLiteMemoryStore`, or broad UI redesign.

### Phase 6.6: Orchestration Snapshot and Workspace State Skeleton

Completed. Added deterministic orchestration/workspace state aggregation without provider,
model, memory, tool, or autonomous execution.

Scope:

- Added value-only orchestration snapshot metadata:
  - `OrchestrationSnapshot`
  - `WorkspaceStateSummary`
  - `OrchestrationHealthStatus`
  - `OrchestrationSignal`
- Snapshot aggregates read-only metadata from routing mode/status, selected provider/model summary,
  latest task plan status/summary, preferred agent summary, memory affinity summary, provider
  catalog count, agent count, memory taxonomy count, runtime context status, and activity metadata.
- `ApplicationController` builds the current snapshot deterministically on demand and emits snapshot
  change notifications alongside relevant metadata-only changes.
- `DesktopShellViewModel` exposes only QML-safe snapshot status, summary, and compact signal
  strings.
- Dashboard shows a minimal read-only orchestration snapshot panel using existing presentation
  patterns.
- Tests cover deterministic snapshot value behavior, controller aggregation, routing-mode updates,
  provider/agent/memory counts, preferred agent/memory/task summaries, and view-model QML-safe
  exposure.

Still out of scope:

- Real provider integration, Ollama/OpenAI/Anthropic calls, API keys, networking, model downloads,
  model execution, real tool execution, filesystem/system actions, plugin loading, vector
  databases, embeddings, semantic search, autonomous background workers, timers, threads, or broad
  UI redesign.

### Phase 6.7: Orchestration Diagnostics and Readiness Checklist

Completed. Added deterministic diagnostics/readiness metadata over the existing orchestration
snapshot and catalogs without probing providers, models, filesystems, networks, or system services.

Scope:

- Added value-only orchestration diagnostics metadata:
  - `OrchestrationDiagnostic`
  - `OrchestrationDiagnosticLevel`
  - `OrchestrationReadinessCheck`
  - `OrchestrationReadinessReport`
- Added `StaticOrchestrationDiagnostics` for ordered, deterministic readiness report generation.
- Diagnostics inspect metadata-only readiness for routing mode, selected provider/model route,
  provider catalog, agent registry, memory taxonomy, task planner, snapshot health, local-only
  privacy posture, cloud provider unavailability/not-configured posture, and disabled execution
  capability.
- `ApplicationController` and `DesktopShellViewModel` expose only QML-safe readiness status,
  summary, and diagnostic `QStringList` values.
- Dashboard and Settings show minimal read-only readiness visibility without actions or setup UI.
- Tests cover deterministic ordering, local-only healthy metadata, cloud not-configured behavior,
  execution disabled checking, and controller/view-model exposure.

Still out of scope:

- Real provider integration, Ollama/OpenAI/Anthropic calls, API keys, networking, provider/model
  probing, model downloads, model execution, real tool execution, filesystem/system scans/actions,
  plugin loading, vector databases, embeddings, semantic search, autonomous background workers,
  timers, threads, external process calls, or broad UI redesign.

### Phase 6.8: Runtime Context Session Layer

Completed. Added deterministic conversation/session context metadata without adding provider,
model, memory, tool, streaming, or autonomous execution.

Scope:

- Added value-only runtime context session metadata:
  - `ConversationSession`
  - `ConversationSessionId`
  - `ConversationSessionStatus`
  - `RuntimeContextWindow`
  - `InteractionMode`
  - `AttentionState`
  - `ContextScope`
- Added `ConversationSessionStore` and `ConversationSessionContextBuilder` for deterministic
  session/context-window ownership and summary generation.
- `ApplicationController` owns the conversation session separately from `ChatSession` message
  history and Phase 4 `RuntimeSession` agent runtime metadata.
- Context windows summarize current routing mode, preferred agent, memory affinity, and latest
  orchestration snapshot summary.
- `DesktopShellViewModel` exposes only QML-safe read-only strings for conversation session id,
  status, interaction mode, attention state, and context-window summary.
- Dashboard and Settings show minimal read-only session/context metadata.
- Tests cover deterministic session creation, status defaults, context-window metadata,
  controller/view-model exposure, routing update reflection, and separation from `ChatSession` and
  `RuntimeSession`.

Still out of scope:

- Real provider integration, Ollama/OpenAI/Anthropic calls, API keys, networking, model downloads,
  model execution, streaming, real tool execution, filesystem/system actions, plugin loading,
  vector databases, embeddings, semantic search, autonomous background workers, timers, threads,
  external process calls, multi-conversation persistence, or broad UI redesign.

### Phase 6.9: Conversation State Graph Skeleton

Completed. Added deterministic conversation state-machine metadata without adding provider, model,
tool, approval, streaming, or autonomous execution.

Scope:

- Added value-only conversation state graph metadata:
  - `ConversationState`
  - `ConversationTransition`
  - `ConversationTransitionResult`
  - `ConversationTransitionStatus`
  - `IConversationStateGraph`
  - `StaticConversationStateGraph`
- Defined deterministic high-level states: Idle, Listening, Planning, Routing,
  Waiting For Approval, Ready To Respond, Responding, Completed, and Error.
- Added safe transition rules for the current metadata flow and deterministic rejection summaries
  for invalid transitions.
- `ApplicationController` owns the state graph separately from `ConversationSession`,
  `ChatSession`, and Phase 4 `RuntimeSession`.
- `DesktopShellViewModel` exposes only QML-safe read-only strings for current state, transition
  status, and transition summary.
- Dashboard and Settings show minimal read-only state metadata.
- Tests cover valid transitions, invalid transition rejection, error transitions,
  controller/view-model exposure, QML-safe properties, and separation from chat/runtime metadata.

Still out of scope:

- Real provider integration, Ollama/OpenAI/Anthropic calls, API keys, networking, model downloads,
  model execution, streaming, real tool execution, approval controls/actions, filesystem/system
  actions, plugin loading, vector databases, embeddings, semantic search, autonomous background
  workers, timers, threads, external process calls, multi-conversation persistence, or broad UI
  redesign.

### Phase 6.10: Pre-runtime Architecture Checkpoint and Stabilization

Completed. Checkpointed the completed Phase 6 metadata orchestration foundation before Phase 7.

Scope:

- Reviewed the Phase 6 architecture across provider catalog, model router, task planner, agent
  registry, memory taxonomy, orchestration snapshot, diagnostics/readiness, conversation session,
  conversation state graph, `ApplicationController` ownership, and `DesktopShellViewModel` QML
  exposure.
- Confirmed Phase 6 surfaces remain deterministic, value-based, and metadata-only.
- Added `docs/PHASE_6_CHECKPOINT.md` with completed scope, architecture findings, known
  limitations, Phase 7 readiness criteria, strict out-of-scope list, and recommended Phase 7
  breakdown.
- Updated roadmap/status/context/orchestration docs to mark Phase 6 as checkpointed.
- Defined Phase 7.0 as local runtime boundary planning/implementation, not full model execution
  unless explicitly scoped later.
- No product features, runtime execution, provider integrations, or UI redesign were added.

Architecture findings:

- Provider catalog, model routing, task planning, agent registry, memory taxonomy, snapshot,
  diagnostics, conversation session, and conversation state graph remain separate metadata
  responsibilities.
- `ChatSession`, `ConversationSession`, Phase 4 `RuntimeSession`, and `ConversationStateGraph`
  remain separate.
- QML exposure remains QML-safe through `DesktopShellViewModel`.

Still out of scope:

- Real provider integration, Ollama/OpenAI/Anthropic calls, API keys, networking, model downloads,
  model execution, streaming, real tool execution, approval controls/actions, filesystem/system
  actions, plugin loading, vector databases, embeddings, semantic search, autonomous background
  workers, timers, threads, external process calls, multi-conversation persistence, or broad UI
  redesign.

### Phase 7.0: Local Runtime Boundary Skeleton

Completed. Added the future local inference/runtime boundary without enabling execution.

Scope:

- Added value-only local runtime metadata:
  - `LocalRuntimeDescriptor`
  - `LocalRuntimeStatus`
  - `LocalRuntimeHealth`
  - `LocalRuntimeCapability`
  - `LocalRuntimeRequest`
  - `LocalRuntimeResponse`
- Added `ILocalRuntime` as a separate future local runtime boundary.
- Added `NullLocalRuntime` with deterministic metadata and safe placeholder refusal for requests.
- `ApplicationController` owns the local runtime boundary and exposes status, health, summary,
  capability summaries, and refusal summary strings.
- `DesktopShellViewModel` forwards only QML-safe read-only strings and string lists.
- Settings shows minimal read-only local runtime metadata without actions.
- Tests cover deterministic metadata, safe refusal, controller exposure, and view-model QML-safe
  exposure.

Separation:

- `ILocalRuntime` is not `IChatProvider`.
- `ILocalRuntime` is not `IModelRouter`.
- `ILocalRuntime` is not `IAgentRuntime`.
- `ILocalRuntime` is not `IToolExecutor`.

Still out of scope:

- Real provider integration, Ollama/OpenAI/Anthropic calls, API keys, networking, model downloads,
  model execution, streaming, process/subprocess launch, real tool execution, approval
  controls/actions, filesystem/system scans/actions, plugin loading, vector databases, embeddings,
  semantic search, autonomous background workers, timers, threads, external process calls,
  multi-conversation persistence, or broad UI redesign.

### Phase 7.1: Local Runtime Session Ownership Skeleton

Completed. Added deterministic local runtime session ownership/lifecycle metadata without enabling
runtime execution.

Scope:

- Added value-only local runtime session metadata:
  - `LocalRuntimeSession`
  - `LocalRuntimeSessionId`
  - `LocalRuntimeSessionStatus`
  - `LocalRuntimeSessionHealth`
  - `LocalRuntimeAllocation`
  - `LocalRuntimeReservation`
- Added `ILocalRuntimeSessionManager` as the future local runtime session ownership boundary.
- Added `NullLocalRuntimeSessionManager` with one deterministic placeholder reserved session.
- Session lifecycle states are representable as metadata: Not Started, Reserved, Active,
  Suspended, and Released.
- `ApplicationController` exposes local runtime session count, status, health, summary,
  allocation summary, reservation summary, and session summary strings.
- `DesktopShellViewModel` exposes only QML-safe read-only strings, counts, and string lists.
- Settings shows minimal read-only local runtime session metadata without actions.
- Tests cover deterministic session metadata, lifecycle naming, summary ordering,
  controller/view-model exposure, and QML-safe property boundaries.

Separation:

- Local runtime sessions are not chat sessions.
- Local runtime sessions are not Phase 4 agent runtime execution/context.
- Local runtime sessions are not provider/model execution.
- Local runtime sessions are not tool execution or plugin ownership.

Still out of scope:

- Real provider integration, Ollama/OpenAI/Anthropic calls, API keys, networking, model downloads,
  model execution, streaming, process/subprocess launch, real tool execution, approval
  controls/actions, filesystem/system scans/actions, plugin loading, vector databases, embeddings,
  semantic search, autonomous background workers, timers, threads, external process calls,
  multi-conversation persistence, or broad UI redesign.

### Phase 7.2: Runtime Capability Negotiation Layer

Completed. Added deterministic runtime capability negotiation metadata without enabling runtime
execution or capability activation.

Scope:

- Added value-only runtime capability negotiation metadata:
  - `RuntimeCapabilityDescriptor`
  - `RuntimeCapabilityState`
  - `RuntimeCapabilityGroup`
  - `RuntimeNegotiationProfile`
  - `RuntimeNegotiationResult`
- Added `IRuntimeCapabilityRegistry` as the future runtime capability metadata boundary.
- Added `StaticRuntimeCapabilityRegistry` with deterministic descriptors for local inference,
  streaming, multimodal, embeddings, semantic memory, tool bridge, plugin bridge, memory binding,
  filesystem access, external process execution, cloud relay support, local-only enforcement, and
  privacy-safe mode.
- Enabled capabilities are safety metadata only: local-only enforcement and privacy-safe mode.
- Disabled or unavailable capabilities remain non-executable and are not activated automatically.
- `ApplicationController` exposes capability count, enabled/disabled summaries, negotiation
  profile summary, negotiation result summary, and local-only enforcement summary.
- `DesktopShellViewModel` exposes only QML-safe read-only strings, counts, and string lists.
- Settings shows minimal read-only runtime negotiation metadata without controls.
- Tests cover deterministic registry metadata, disabled/unavailable capability handling,
  local-only enforcement metadata, controller exposure, and QML-safe view-model properties.

Separation:

- Runtime capability negotiation is not runtime execution.
- Runtime capability negotiation is not provider routing.
- Runtime capability negotiation is not agent execution.
- Runtime capability negotiation is not tool execution.
- Runtime capability negotiation is not permission approval.
- Runtime capability negotiation is not plugin management.

Still out of scope:

- Capability activation, real provider integration, Ollama/OpenAI/Anthropic calls, API keys,
  networking, model downloads, model execution, streaming, process/subprocess launch, real tool
  execution, approval controls/actions, filesystem/system scans/actions, plugin loading, vector
  databases, embeddings, semantic search, autonomous background workers, timers, threads, external
  process calls, multi-conversation persistence, or broad UI redesign.

### Phase 7.3: Runtime Permission Metadata Skeleton

Completed. Added deterministic runtime permission metadata boundaries without enabling runtime
execution.

Scope:

- Added value-only runtime permission metadata:
  - `RuntimePermission`
  - `RuntimePermissionLevel`
  - `RuntimePermissionDecision`
  - `RuntimePermissionRequest`
- Added `IRuntimePermissionPolicy` and `StaticRuntimePermissionPolicy`.
- Default policy denies runtime execution permission requests in metadata-only mode.
- `ApplicationController` and `DesktopShellViewModel` expose QML-safe read-only permission
  decision/summary strings.
- Tests cover deterministic naming and default-deny behavior.

Separation:

- Runtime permission metadata is not runtime execution.
- Runtime permission metadata is not provider/model invocation.
- Runtime permission metadata is not tool/plugin execution.

### Phase 7.4: Runtime Request Pipeline Skeleton

Completed. Added a deterministic metadata-only runtime request pipeline with trace visibility and
blocked execution status.

Scope:

- Added value-only runtime pipeline metadata:
  - `RuntimePipelineRequest`
  - `RuntimePipelineStage`
  - `RuntimePipelineResult`
  - `RuntimePipelineStatus`
  - `RuntimePipelineTrace`
- Added `IRuntimePipeline` and `StaticRuntimePipeline`.
- Pipeline evaluates request metadata, permission decision metadata, and safety report metadata in
  ordered trace stages.
- Execution remains blocked and non-operational; pipeline returns status/summary/trace metadata
  only.
- `ApplicationController` and `DesktopShellViewModel` expose QML-safe read-only pipeline
  status/summary/trace string lists.
- Tests cover deterministic stage naming, ordered traces, and blocked execution metadata.

Separation:

- Runtime request pipeline is not provider runtime.
- Runtime request pipeline is not model execution.
- Runtime request pipeline is not process/tool/plugin execution.

### Phase 7.5: Runtime Safety Policy Skeleton

Completed. Added deterministic runtime safety posture metadata and rule reporting without sandbox
runtime execution.

Scope:

- Added value-only runtime safety metadata:
  - `RuntimeSafetyPolicy`
  - `RuntimeSafetyRule`
  - `RuntimeSafetyDecision`
  - `RuntimeSafetyReport`
- Added `IRuntimeSafetyPolicy` and `StaticRuntimeSafetyPolicy`.
- Safety report enforces local-only and no-execution posture metadata with deterministic rules.
- Settings page shows read-only runtime permission/safety/pipeline status and trace metadata without
  controls.
- Tests cover deterministic safety decision/report output and QML-safe controller/view-model
  exposure.

Separation:

- Runtime safety policy metadata is not sandbox runtime enforcement.
- Runtime safety policy metadata is not provider/model/tool execution.
- Runtime safety policy metadata is not filesystem/network/process behavior.

Still out of scope:

- Real provider integration, Ollama/OpenAI/Anthropic calls, API keys, networking, downloads, model
  execution, streaming, process/subprocess launch, filesystem/system scans/actions, real tools,
  plugin loading, embeddings/vector DB/semantic search, autonomous workers, and approval/setup UI.

### Phase 7.6: Runtime Architecture Checkpoint and Cleanup

Completed. Checkpointed the Phase 7 local runtime boundary foundation before Phase 8.

Scope:

- Reviewed `ILocalRuntime`, local runtime session manager, capability registry, permission policy,
  safety policy, request pipeline, `ApplicationController` ownership, and
  `DesktopShellViewModel` QML exposure.
- Added `docs/PHASE_7_CHECKPOINT.md` with completed scope, architecture findings, known
  limitations, runtime guardrails, Phase 8 readiness criteria, and strict out-of-scope work.
- Updated roadmap/status/architecture/decision/context docs to mark Phase 7.3 through Phase 7.6 as
  completed metadata-only work.
- Tightened QML read-only navigation exposure so the view-model page list includes every accepted
  page name.
- Added focused view-model test coverage for the updated read-only page list.

Architecture findings:

- Phase 7 runtime surfaces are deterministic metadata only.
- Local runtime, session ownership, capability negotiation, permission policy, safety policy, and
  request pipeline remain separate responsibilities.
- `ApplicationController` owns runtime boundaries through interfaces and exposes only summary/status
  values.
- `DesktopShellViewModel` exposes QML-safe strings, counts, and string lists; raw runtime objects
  remain hidden.

Still out of scope:

- Real provider/model execution, provider calls, API keys, networking, downloads, streaming,
  process/subprocess launch, filesystem/system actions, real tools, plugin loading, sandbox
  runtime enforcement, embeddings/vector DB/semantic search, autonomous workers, and execution or
  setup UI.

### Phase 8.0-8.2: Execution Lifecycle And Session Coordination

Completed. Added a metadata-only execution lifecycle/session coordination layer without enabling
execution.

Scope:

- Added execution value metadata:
  - `ExecutionRequest`
  - `ExecutionIntent`
  - `ExecutionPriority`
  - `ExecutionLifecycleState`
  - `ExecutionLifecycleStatus`
  - `ExecutionLifecycleResult`
  - `ExecutionLifecycleTrace`
  - `ExecutionTraceLevel`
- Added execution session metadata:
  - `ExecutionSession`
  - `ExecutionSessionId`
  - `ExecutionSessionStatus`
  - `ExecutionOwnership`
  - `ExecutionCoordinationMode`
- Added `IExecutionLifecycle`, `StaticExecutionLifecycle`, `ExecutionCoordinator`, and
  `ExecutionCoordinationSnapshot`.
- Lifecycle evaluation is deterministic and ordered:
  requested -> validating -> permission-check -> safety-check -> coordination ->
  ready-placeholder -> blocked.
- Execution remains blocked and non-executable even when the lifecycle reaches
  ready-placeholder metadata.
- Invalid transitions are rejected safely.
- `ApplicationController` owns lifecycle/coordinator interfaces.
- `DesktopShellViewModel` exposes QML-safe strings and string lists only.
- Dashboard and Settings show read-only lifecycle/session/snapshot metadata with no controls.

Separation:

- Execution lifecycle is not local runtime, chat provider, agent runtime, tool executor, provider
  adapter, or plugin/runtime worker.
- Coordination snapshot is a read-only metadata view, not a scheduler.

Still out of scope:

- Real provider/model execution, Ollama launch, API keys, networking, downloads, streaming,
  subprocess/process launch, filesystem/system actions, real tools, plugin loading, autonomous
  workers, timers/background loops, and execution/setup UI.

### Phase 8.3-8.5: Local Runtime Adapter, Provider Bridge, And Pre-integration Readiness

Completed. Added metadata-only local runtime adapter, provider bridge, and runtime integration
readiness boundaries to prepare for future Ollama/local runtime integration without calling,
launching, discovering, or executing anything.

Scope:

- Added local runtime adapter metadata:
  - `ILocalRuntimeAdapter`
  - `LocalRuntimeAdapterDescriptor`
  - `LocalRuntimeAdapterStatus`
  - `LocalRuntimeAdapterHealth`
  - `LocalRuntimeAdapterCapabilitySummary`
  - `StaticLocalRuntimeAdapter`
- Added provider bridge metadata:
  - `IProviderRuntimeBridge`
  - `ProviderRuntimeBridgeStatus`
  - `ProviderRuntimeBridgeSummary`
  - `ProviderRuntimeBridgeRequest`
  - `ProviderRuntimeBridgeResponse`
  - `StaticProviderRuntimeBridge`
- Added pre-integration readiness metadata:
  - `RuntimeIntegrationReadiness`
  - `RuntimeIntegrationCheck`
  - `RuntimeIntegrationReport`
  - `StaticRuntimeIntegrationReadiness`
- Adapter metadata is descriptive placeholder data only.
- Provider bridge reports not connected and not executable.
- Readiness report deterministically explains missing endpoint configuration, model discovery,
  provider bridge connection, and execution permission before real Ollama/local runtime integration.
- `ApplicationController` owns adapter/bridge/readiness boundaries.
- `DesktopShellViewModel` exposes QML-safe strings and string lists only.
- Dashboard and Settings show read-only integration readiness metadata with no controls.

Separation:

- Adapter contract is not execution.
- Provider bridge is not an `IChatProvider` implementation.
- Readiness report is not probing.
- Execution lifecycle remains blocked.
- Local runtime remains placeholder-only.

Still out of scope:

- Ollama/OpenAI/Anthropic calls, API keys, networking, downloads, streaming, process/subprocess
  launch, filesystem/system actions, model discovery, model execution, real tools/plugins,
  embeddings/vector DB, autonomous workers, endpoint fields, setup controls, and model selection UI.

### Phase 9.0-9.2: Ollama Local Health And Discovery Boundary

Completed. Added the first controlled local-provider integration boundary for Ollama health and
installed-model metadata only.

Scope:

- Added Ollama value/config metadata:
  - `OllamaEndpoint`
  - `OllamaConfig`
  - `OllamaConnectionStatus`
  - `OllamaHealthStatus`
  - `OllamaModelSummary`
- Added runtime client boundary:
  - `IOllamaRuntimeClient`
  - `NullOllamaRuntimeClient`
  - `OllamaHttpRuntimeClient`
- Default endpoint is `http://127.0.0.1:11434`.
- Endpoint normalization accepts loopback HTTP only and safely falls back to the default for cloud,
  non-HTTP, malformed, query, fragment, or non-loopback endpoints.
- `AppSettings` can persist the normalized Ollama endpoint; no API key or credential setting
  exists.
- HTTP health checks are limited to the loopback Ollama `/api/version` endpoint.
- Optional model discovery is limited to the loopback Ollama `/api/tags` endpoint and returns
  installed model metadata only.
- `ApplicationController` exposes endpoint, connection status, health status, health summary, model
  count, and model summary strings.
- `DesktopShellViewModel` exposes QML-safe strings/counts/lists only.
- Dashboard and Settings show read-only Ollama local status.

Separation:

- Ollama client is not an `IChatProvider`.
- Ollama client is not execution lifecycle, model router execution, agent runtime, tool executor, or
  plugin runtime.
- Chat requests are not routed to Ollama.
- Execution lifecycle remains blocked and non-executable.

Still out of scope:

- Prompt/model generation, streaming, model downloads/pulls/deletes/runs, subprocess/process
  launch, cloud calls, API keys, tool/plugin execution, filesystem/system scans/actions, background
  workers, setup UI, and model selection UI.
