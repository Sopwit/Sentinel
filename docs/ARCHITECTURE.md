# Sentinel Architecture

Sentinel Desktop Alpha is a modular monolith. The application is split into a native Qt/QML shell and a small C++ core library.

## Layers

- `apps/sentinel-desktop`: desktop application entry point and QML engine setup.
- `core`: provider, memory, mode, context, plugin, and application coordination interfaces.
- `ui/qml`: desktop shell and presentation layer.
- `integrations`: placeholder for future local and provider integrations.
- `plugins`: placeholder for future plugin contracts and loading.
- `tests`: Qt-based automated tests for core behavior and desktop view-model boundaries.

## Current Runtime Flow

`main.cpp` creates `ApplicationController`, `LocalEchoProvider`, `NullAgentRuntime`,
`SQLiteMemoryStore`, `SQLiteChatHistoryStore`, the loopback-only Ollama runtime/inference clients,
the async local inference worker, the local-only runtime permission policy, `ModeManager`,
`AppSettings`, and
`DesktopShellViewModel`, then exposes only the view model to QML.

QML handles layout and user input. C++ owns chat handling, provider calls, mode state, memory state, chat history persistence, and settings defaults.

Phase 19.4 through Phase 19.6 are presentation-only UI cleanup phases. Home is the primary local
Ollama chatbot surface. Runtime/Memory is a local memory, recall, conversation-context, and local
data visibility area, not the main chat screen. Agents is a metadata-only registry/task/capability
visibility area, not an execution console. Developer Mode changes only UI density and diagnostic
visibility; it does not grant provider, model, tool, voice, filesystem, subprocess, cloud, or
agent authority.

## Desktop View Model

`DesktopShellViewModel` is the QML boundary for the desktop app. It forwards safe operations to the core controller, mode manager, and settings object:

- provider name
- current mode and available modes
- chat messages and send action
- memory entries and maintenance actions
- theme/configuration placeholder settings

Raw core objects are not exposed directly to QML.

The bottom status strip is presentation-only and reads from `DesktopShellViewModel` properties
backed by `ApplicationController`, `ModeManager`, and `AppSettings` state. It must not introduce
mock values, local probes, permission changes, or direct runtime access.

## QML Structure

The desktop shell is split into small QML components:

- `Main.qml`: application window and high-level layout.
- `components/HeaderBar.qml`: current mode and mode switcher.
- `components/Atmosphere.qml`: low-cost ambient shell background.
- `components/WorkspacePresence.qml`: central mode-aware workspace presence visualization.
- `components/InfoRow.qml`: read-only label/value status row presentation.
- `components/StatusBar.qml`: local alpha status footer.
- `pages/DashboardPage.qml`: overview and chat panel host.
- `pages/MemoryPage.qml`: runtime memory UI.
- `pages/SettingsPage.qml`: settings placeholder UI.
- `theme/SentinelTheme.qml`: presentation tokens for palette, spacing, radius, and typography.
- `components/SentinelButton.qml`: lightweight command button styling with tokenized hover/focus
  states.
- `components/SentinelTextField.qml`: lightweight text-field styling with tokenized focus states.

These files bind to `shellViewModel`. They should not own business rules, provider logic, persistence logic, or platform automation.

Current product styling keeps `ShellPanel` brackets disabled by default. Pages may use subtle glass
panels and soft borders; accent color should be reserved for focus, selected segmented controls,
active status chips, and warnings.

## i18n Direction

Sentinel uses a Qt-native localization foundation for the core desktop shell:

- Preferred app languages are English and Turkish.
- The persisted `AppSettings::appLanguage` value is `system`, `en`, or `tr`.
- System Default resolves to Turkish only when the system locale is Turkish; otherwise English is
  used as the fallback.
- QML receives only QML-safe language state through `DesktopShellViewModel`: `appLanguage`,
  `availableLanguages`, and `languageDisplayName`.
- User-facing QML strings should use `qsTr()`. Internal ids, enum values, provider names, model
  names, filesystem paths, and diagnostic machine tokens must not be translated.
- Startup translator loading is presentation-only and must not alter prompt assembly, provider
  routing, runtime permissions, memory behavior, context behavior, or tool/provider execution
  authority.
- `.ts` catalogs exist for English and Turkish. Compiled `.qm` release packaging and live
  retranslation remain follow-up work; language changes may require restart.

A JSON/string-catalog structure like Helium-style web apps is not recommended for Sentinel's core
UI because Qt already provides extraction, context, plural handling, QML/C++ integration, and
release packaging through its native translation workflow.

External API/cloud providers remain future opt-in integrations only. No cloud provider or API-key
configuration is active in the current desktop runtime.

## Runtime Provider Registry

Phase 36 introduces a value-only runtime provider abstraction above the existing local Ollama
health/model metadata and inference boundary.

The current registry contains:

- `ollama`: Local Ollama, the only enabled provider/runtime path.
- `openai-compatible`: disabled placeholder metadata only.

Runtime provider descriptors expose provider id, display name, runtime state, deterministic
readiness state, validation state, endpoint/model summaries, installed/configured/enabled flags,
model names, and capability metadata. Capability metadata includes local-only, API-key
requirement, offline support, structured output, reasoning, function calling, images, audio,
streaming, tools, vision, and embeddings.

The registry can report selected provider, active provider, installed providers, configured
providers, available local runtimes, capability summaries, and validation traces. Disabled
provider selections fall back to the enabled local runtime as active provider metadata; they do
not enable cloud execution or routing.

This layer does not probe providers in the background, auto-discover runtimes, scan filesystems,
download models, install packages, store API keys, route to cloud providers, execute tools, or
switch providers automatically. Ollama health/model metadata remains the existing explicit
loopback-only path, and local inference still requires the existing user opt-in, selected model,
permission, safety, and busy-state gates.

## Local Ollama Chat Reliability

The current active inference path is local Ollama only. `ApplicationController` owns the local chat
readiness contract and `DesktopShellViewModel` exposes only QML-safe booleans and summaries to the
UI.

Send readiness requires:

- local chat inference enabled
- local loopback HTTP Ollama endpoint
- reachable Ollama health status
- installed-model discovery available
- explicitly selected model present in discovered Ollama metadata
- active conversation not archived
- no active local inference request

Normal UI consumes `localChatSendAvailable` and `localChatSendAvailabilitySummary` for composer
state and next-action copy. Developer Mode may show `localInferenceStatus`, traces, latency,
stream status, and conversation runtime summaries. Failed readiness refuses before transcript
mutation; started requests still use request-id guards so stale completions after cancellation,
clear, or conversation switch are ignored.

`ApplicationController` also owns a QML-safe send lifecycle:
idle, validating, sending, streaming, completed, refused, failed, and cancelled. Refused means the
prompt was not accepted and no transcript mutation occurred. Failed means a request was accepted
and then produced a runtime/provider failure, so the user prompt remains in the transcript with one
safe assistant error message.

Streaming output is a transient preview only. It is read from local inference stream metadata while
the active request is streaming, clears after completion/failure/cancel/switch, and is never copied
separately into the transcript. Final assistant text is appended once from the completed response
for the request id and active conversation id that started the request.

## Deterministic Context Assembly

Prompt context injection remains explicit opt-in local behavior. When enabled, `ApplicationController`
builds local value-only candidates from recent conversation history, deterministic older
conversation summaries, committed key-value memory, runtime metadata, orchestration metadata, and
selected conversation metadata.

Committed key-value memory first passes through deterministic memory relevance scoring. The value
records are `MemoryRelevancePolicy`, `MemoryRelevanceCandidate`, `MemoryRelevanceScore`,
`MemoryRelevanceReason`, `MemoryRelevanceBudget`, `MemoryRelevanceSelection`,
`MemoryRelevanceTrace`, and `MemoryRelevanceSummary`. Scoring uses only literal key overlap,
literal value overlap, active conversation title overlap, recent conversation terms, and
explicit pinned/committed priority metadata. Selected memories are emitted as separate committed
memory context candidates so duplicate suppression, character budgeting, included/excluded counts,
and exclusion reasons remain explainable.

Conversation salience then scores the deterministic candidates through value-only records:
`ConversationSaliencePolicy`, `ConversationSalienceCandidate`, `ConversationSalienceScore`,
`ConversationSalienceReason`, `ConversationSalienceBudget`, `ConversationSalienceSelection`,
`ConversationSalienceTrace`, and `ConversationSalienceSummary`. Scoring uses only literal active
conversation title overlap, recent user message overlap, recent assistant message overlap, pinned
conversation metadata, committed memory overlap, explicit user query terms, and deterministic
recency weighting.

Adaptive budgeting splits prompt-time context capacity across active conversation context, selected
committed memories, and runtime/orchestration metadata. The split is deterministic and exposes
included, excluded, duplicate, truncated, and allocation summaries through QML-safe strings and
counts.

Selection is deterministic: fixed source priority, stable tie-breaking, bounded character budget,
bounded candidate/source counts, duplicate suppression, and explicit exclusion reasons. Committed
key-value memory is considered for prompt injection only when deterministic local relevance
metadata finds literal overlap or explicit pinned priority. The current user prompt remains outside
conversation history and is placed after the compact local context block.

QML receives only safe summaries through `DesktopShellViewModel`: context used, budget usage,
included/excluded counts, memory relevance summaries, salience budget/trace/exclusion summaries,
source summaries, and trace summaries. The UI does not receive raw prompt dumps, vector scores,
provider handles, filesystem paths, or semantic payloads.

The context assembly path does not run semantic/vector search, embeddings, filesystem indexing,
background summarization, autonomous memory writes, tool/plugin execution, subprocesses, cloud/API
calls, or provider discovery.

## Conversation Compression Readiness

Conversation compression is currently a deterministic metadata-readiness layer only.
`ConversationCompressionPolicy`, `ConversationCompressionStatus`,
`ConversationCompressionCandidate`, `ConversationCompressionBudget`,
`ConversationCompressionReadiness`, `ConversationCompressionSelection`,
`ConversationCompressionTrace`, `ConversationCompressionFallback`, and
`ConversationCompressionSummary` describe pressure and candidate plans without generating a
summary.

Readiness uses local message count, estimated character/token pressure, active conversation
length, prompt context injection enablement, existing deterministic summary availability, and
conversation salience budget pressure. Candidate planning can mark recent conversation window,
older conversation segment, high-salience user facts, low-salience repeated turns, and
system/runtime metadata exclusion. Candidates are ordered deterministically and bounded by a local
metadata budget.

The controller and desktop view model expose only QML-safe summaries and counts. Compression does
not mutate transcripts, replace user/assistant messages, write committed memory, alter prompts,
expose raw prompt/debug payloads, call providers/models, activate semantic/vector systems, index
filesystems, call cloud APIs, or start background workers.

## Controlled Local Summary Generation

Conversation summary generation is an explicit manual foreground pipeline through the existing
local inference boundary. `ConversationSummaryRequest`, `ConversationSummaryReadiness`,
`ConversationSummarySegment`, `ConversationSummaryTrace`, `ConversationSummaryFallback`, and
`ConversationSummaryPreview` extend the existing value-only summary records.

Planning is deterministic over the active visible conversation only. It separates the retained
recent window, older-window summary preparation, important user facts, repeated-turn exclusions,
and system/runtime metadata exclusion. The controller refuses non-manual, background, inactive
conversation, archived conversation, busy generation, transcript-mutating, memory-writing,
hidden-prompt, tool, filesystem-authority, missing-runtime, and missing-model requests before
execution.

Execution uses only the local inference worker already used by local Ollama chat. Request ids and
active conversation ids guard completions, so stale results after conversation switches or
cancellation are suppressed. Generated text is sanitized, bounded by the summary budget, and
refused when it contains hidden prompt/runtime/provider/tool/filesystem leakage terms.

Persistence stores only safe local summary records beside the conversation store: sanitized
summary text, timestamp, source conversation id, covered message range, estimated reduction, and
readiness state. Raw prompts, hidden prompt templates, transcript replacements, committed
memories, provider payloads, runtime internals, and traces are not persisted.

When prompt context injection is explicitly enabled, the generated summary can participate as the
existing Conversation Summary context candidate in deterministic source order. It does not replace
transcript history and does not write memory.

## Summary-Aware Conversation Continuity

Phase 32 extends the deterministic context path so persisted manual local summaries can preserve
long-conversation continuity after restarts and context pressure. `ApplicationController` validates
summary ownership, readiness state, timestamp, covered message range, active archive state,
transcript compatibility, and freshness before exposing the summary as a context candidate.

Validated summaries participate only in the existing explicit prompt-context path. Ordering remains
stable: active conversation recency, summary continuity value, committed memory relevance, runtime
metadata, orchestration/selected-conversation metadata, then budget constraints. Recent transcript
turns remain in the conversation window, and the summary compresses older covered history without
deleting, replacing, or mutating transcript messages.

Stale, invalid, incompatible, archived, unavailable, and budget-excluded summaries produce
QML-safe fallback and allocation summaries. The fallback is transcript-only context selection; it
does not write committed memory, run generation, start background work, activate semantic/vector
retrieval, index filesystems, call cloud/API providers, or expose raw prompt/debug payloads.

## Context Observability And Explainability

Phase 33 adds a read-only explainability layer over deterministic context assembly. The core value
records are `ContextDecisionReason`, `ContextDecisionTrace`, `ContextDecisionBudget`,
`ContextDecisionContribution`, `ContextDecisionFallback`, `ContextDecisionSummary`, and
`ContextDecisionVisibility`.

The controller derives these records from already-bounded prompt context injection, conversation
salience, memory relevance, and summary continuity metadata. QML receives only safe summaries:
context reasoning, ordering stages, contribution breakdowns, inclusion/exclusion hints, fallback
reasoning, and Developer Mode traces. The UI does not receive hidden prompts, raw system prompts,
provider payloads, raw prompt text, semantic/vector internals, filesystem paths, or unsafe debug
objects.

Ordering visibility is deterministic: recent transcript, continuity summary, committed memory, and
runtime metadata. Budget visibility reports allocated characters, approximate tokens, remaining
budget, compression gain, and transcript/summary/memory/runtime metadata contribution counts.
Explainability is visibility only; it does not change prompt construction, mutate transcripts,
write memory, activate semantic/vector systems, index filesystems, call providers/cloud APIs,
execute tools/plugins, or start background workers.

Phase 34 adds the persisted `contextExplainabilityVisible` setting as a UI-only control. It
defaults on and is exposed through `AppSettings` and `DesktopShellViewModel` for QML. Disabling it
hides normal Home/Chat context reasoning surfaces and normal Settings visibility copy, but the
controller still derives safe context decision metadata internally. Developer Mode remains a
separate visibility gate for bounded diagnostics and does not alter runtime permission, safety,
prompt assembly, retrieval selection, provider, tool, or execution authority.

## Intentional Boundaries

- Chat provider behavior is hidden behind `IChatProvider`.
- Agent orchestration behavior is hidden behind `IAgentRuntime`.
- Memory behavior is hidden behind `IMemoryStore`.
- Chat history behavior is hidden behind `IChatHistoryStore`.
- Future plugin behavior starts at `IPlugin`.
- Context construction starts at `IContextEngine`.
- Agent task orchestration starts at `IAgentTaskRuntime` and remains metadata-only.
- UI code does not call network APIs or own business rules.

## Memory Storage Contract

`IMemoryStore` is intentionally storage-backend independent. It stores exact key/value pairs and returns entries through a small value-based contract. Application-level validation, such as rejecting blank keys, belongs in controllers or services rather than storage implementations.

`InMemoryStore` remains the default lightweight test backend. `SQLiteMemoryStore` implements the same `IMemoryStore` contract for desktop persistence without requiring changes to `ApplicationController`.

The desktop app stores memory below Qt's `AppDataLocation` as `memory.sqlite3`. Settings remain separate in `JsonSettingsStore` below Qt's `AppConfigLocation`.

SQLite memory storage stores only explicit key-value memory entries. Chat history uses a separate store and database.

`SQLiteMemoryStore` exposes generic availability and last-error diagnostics through `IMemoryStore`. The desktop shell only shows an Available/Unavailable memory status and does not expose SQLite-specific details to QML.

The SQLite database includes a `memory_schema_metadata` table with `schema_version = 1`. This is migration preparation only; no migration framework exists yet.

Phase 6.5 adds memory taxonomy metadata separately from this storage contract:

- `MemoryMetadata.h` defines value-only memory taxonomy descriptors.
- `IMemoryCatalog` is the read-only catalog boundary for memory category metadata.
- `StaticMemoryCatalog` describes Episodic, Semantic, Procedural, Reflective, and Ambient memory
  categories with retention, privacy, recall hint, association, and task-affinity labels.
- `StaticTaskPlanner` may annotate task plans with preferred memory affinity metadata.
- The taxonomy catalog does not read or write `IMemoryStore`, perform recall, build embeddings, run
  semantic search, create a vector database, or write autonomous memories.

Phase 6.7 readiness diagnostics inspect existing orchestration metadata only. They do not read
storage paths, scan files, query memory stores, build embeddings, run semantic search, mutate
memory, or perform provider/model/tool execution.

Phase 18.0 through Phase 18.15 add an agent task runtime, queue, lifecycle, planning-session,
capability-registry, and tool-contract metadata boundary beside the
earlier agent and runtime metadata layers:

- `AgentTaskRuntime.h` defines value-only task ids, types, statuses, priorities, sources, plans,
  steps, results, traces, lifecycle events, queue policy, queue summaries, planning sessions,
  planning candidates, arbitration/refusal/fallback metadata, capability ids/types/status/scope/
  policy/requirements/restrictions/readiness/safety reports, tool contract ids/types/status/scope/
  policy/permissions/sandbox/restrictions/readiness/safety reports, registry status/summaries,
  and runtime status records.
- `IAgentTaskRuntime` is the task orchestration boundary.
- `StaticAgentTaskRuntime` creates deterministic local metadata tasks, keeps an in-memory queue
  ordered by priority, queue sequence, and task id, records lifecycle transitions, derives bounded
  planning-session metadata, and refuses execution by design.
- Task examples are metadata only: summarize conversation, inspect memory status, plan response,
  prepare retrieval context, prepare voice response, and prepare export action.
- Tasks may be queued, listed, marked planned, blocked, completed as metadata, or refused. These
  transitions update metadata only and never start workers.
- Planning sessions select candidates deterministically from the queue, apply candidate/step/
  summary budgets, refuse unsafe planning candidates as metadata, and report deterministic
  fallback summaries when budget or safety gates block a candidate.
- The capability registry exposes deterministic local metadata capabilities for conversation
  summarization, memory inspection, retrieval preparation, semantic supplement preparation, export
  preparation, and voice response preparation.
- Future filesystem access, shell execution, and plugin runtime capabilities remain disabled or
  refused and expose safe readiness/safety summaries only.
- The tool contract registry exposes deterministic local metadata contracts for future tool
  categories. Permission labels include local-only, approval-required, sandbox-required, read-only,
  disabled, refused, future filesystem access, future subprocess execution, future plugin runtime,
  and future export action.
- Future filesystem, subprocess, plugin, and export action contracts remain disabled or refused.
  Sandbox requirements are summarized as readiness metadata only and do not create sandbox,
  filesystem, subprocess, plugin, export, or tool runtime behavior.
- The boundary does not execute tools, plugins, filesystem actions, shell/subprocess commands,
  provider/model calls, cloud/API calls, background workers, or autonomous loops.
- `ApplicationController` and `DesktopShellViewModel` expose only strings, counts, and string
  lists for runtime status, queue counts, latest task/lifecycle summaries, task summaries, trace
  summaries, planning status/counts, arbitration summaries, refusal summaries, and fallback
  summaries, plus capability and tool-contract counts, summaries, permission summaries, sandbox
  summaries, readiness summaries, and safety summaries.

Phase 18.16 through Phase 18.18 add a voice runtime permission and path-configuration readiness
layer on top of the existing voice boundaries:

- `Voice.h` defines value-only voice runtime policy/status/readiness/health/sandbox/restriction/
  budget/readiness-report and safety-report metadata, plus Piper and Whisper runtime descriptors,
  configuration records, and path summaries.
- Piper and Whisper readiness uses configured/missing/refused counts and local-only path-style
  checks only. Unsafe or non-local path-style values are refused as metadata. Raw configured paths
  are not exposed by the new readiness summaries.
- Piper and Whisper remain disabled by default and not actively running. Readiness may say
  `Ready Metadata` only when binary and model path values are configured and local-only; this is
  not an execution grant.
- Permission and sandbox summaries label local-only, disabled, sandbox-required, future
  microphone access, future audio playback, future transcription runtime, and future synthesis
  runtime as metadata only.
- Safety reports preserve `executionAttempted = false` and block subprocess execution, inference,
  microphone capture, playback, streaming, filesystem scanning, downloads, cloud/API calls, and
  background workers.
- `ApplicationController` and `DesktopShellViewModel` expose QML-safe readiness summaries, runtime
  health, configured/missing/refused counts, permission summaries, sandbox summaries, and safety
  summaries. QML receives no raw runtime objects and the UI adds no start/stop, microphone,
  playback, or activation controls.

Phase 18.19 through Phase 18.21 add a Whisper STT local runtime foundation behind a new
transcription client boundary:

- `WhisperTranscription.h` defines value-only transcription policy/status/request/result/session/
  budget/readiness/safety/fallback/trace metadata and the `IWhisperTranscriptionClient` boundary.
- `NullWhisperTranscriptionClient` is the disabled default refusal client. The optional local
  client skeleton validates configured binary/model/audio metadata and refuses before execution.
- The boundary is for future local audio-file transcription only. It does not capture microphones,
  perform live recording, play audio, stream STT, call cloud APIs, download models, scan the
  filesystem, inject transcripts into prompts, or send chat messages automatically.
- Unsafe or non-local path-style input is refused before readiness can become accepted metadata.
  QML receives summaries/counts/lists only and receives no raw provider/client/runtime objects.
- Whisper transcription safety preserves `executionAttempted = false`; subprocess execution
  remains blocked until a later explicit execution phase.

Phase 18.25 through Phase 18.27 add a deterministic voice pipeline session orchestration layer on
top of existing Whisper STT, local chat inference, and Piper TTS readiness:

- `VoicePipelineSession` metadata describes the current safe voice-session lifecycle: prepare,
  await audio input, transcription readiness, chat inference readiness, synthesis readiness, and
  completion/refusal/fallback.
- `ApplicationController` composes existing readiness/status values only. It does not call the
  Whisper or Piper clients, does not send chat from voice, and does not inject transcripts.
- Missing Whisper readiness blocks transcription. Missing local chat/model readiness blocks
  inference. Missing Piper readiness blocks synthesis. Unsafe states become refused/fallback
  metadata.
- Safety reports preserve `executionAttempted = false` and block microphone capture, playback,
  Whisper execution, Piper execution, subprocesses, chat auto-send, transcript injection,
  background workers, and autonomous loops.
- QML receives only status strings, counts, stage summaries, trace summaries, fallback summaries,
  and safety summaries. No raw filesystem paths, provider/client objects, transcript payloads, or
  runtime handles are exposed.

Phase 18.28 through Phase 18.30 add a controlled audio-file session foundation for future offline
STT ingestion:

- `AudioFileSession` metadata describes local-only, disabled-by-default audio-file readiness
  without reading files, decoding waveforms, transcribing, playing audio, scanning the filesystem,
  launching subprocesses, or calling cloud/API services.
- `AudioFileDescriptor` carries path-style and declared-size metadata only. Unsafe/non-local
  path-style values are refused as summaries, and raw path values are not exposed to QML.
- `AudioFileValidation` records deterministic validation states for local-only, supported/
  unsupported extension, empty-file, oversized-file, refused-path, sandbox-required,
  future-transcription-ready, and disabled-by-policy.
- Supported future extension metadata is wav, mp3, flac, and ogg. Unsupported extensions produce
  refusal metadata; empty/oversized/sandbox-required states produce fallback metadata.
- Safety reports preserve `executionAttempted = false` and block file loading, waveform decoding,
  transcription, playback, microphone capture, subprocess execution, filesystem scanning,
  automatic ingestion, autonomous loops, and cloud/API calls.
- `ApplicationController` and `DesktopShellViewModel` expose only QML-safe status strings,
  summaries, validation/refusal lists, trace summaries, supported-extension summaries, fallback,
  and safety checks. The UI adds no upload button, file picker, playback control, or redesign.

Phase 18.31 through Phase 18.33 checkpoint the completed Phase 18 agent/tool/voice foundation:

- `docs/PHASE_18_AGENT_VOICE_CHECKPOINT.md` records the audit of agent task runtime, queue/
  lifecycle metadata, planning sessions, capability registry, tool contracts, Whisper STT, Piper
  TTS, voice pipeline sessions, audio-file sessions, controller/view-model exposure, and
  Agents/Settings/Chat status surfaces.
- Agent runtime remains metadata-only through `IAgentTaskRuntime`; task queue, lifecycle,
  planning, arbitration, refusal, capability, and trace records do not start workers, schedulers,
  approval flows, tools/plugins, filesystem actions, shell/subprocess execution, provider/model
  calls, cloud/API calls, or autonomous loops.
- Tool contracts remain permission/sandbox metadata only. They are not grants, executors,
  sandbox implementations, plugin hosts, filesystem adapters, subprocess boundaries, or export
  actions.
- Voice runtime remains readiness/session metadata only. Whisper and Piper clients refuse before
  execution; voice pipeline sessions compose readiness only; audio-file sessions validate
  descriptor metadata only.
- `executionAttempted` remains false across agent/task/planning/capability/tool/voice/Whisper/
  Piper/pipeline/audio-file paths.
- No microphone capture, playback, subprocess execution, filesystem scanning, cloud/API calls,
  tools/plugins, autonomous loops, real STT inference, or real TTS inference are active.
- Phase 19 may proceed only by preserving these guarantees or by explicitly scoping and testing a
  controlled authority change.

Phase 19.0 through Phase 19.3 synchronize the product UI with the current backend/runtime state:

- Home is the primary local assistant surface. It exposes a floating prompt input, compact recent
  transcript preview, and streaming status using `DesktopShellViewModel` and the existing
  `sendMessage` path only.
- The right-side AI Bridge remains available on Home but is no longer the only visible chat input.
- Send controls are presented only when explicit local chat inference is enabled and a selected or
  fallback local Ollama model is available. Disabled states summarize the missing local condition.
- Settings separates normal user configuration from advanced runtime metadata through persisted
  Developer Mode. Developer Mode is a visibility boundary only and is not connected to runtime
  permission, safety, provider, tool, voice, or agent authority.
- Normal Settings surfaces General, Local AI/Ollama, Model Selection, Chat, Voice Setup, and
  Privacy/Local Data. Semantic/vector internals, retrieval budgets, arbitration, tool contracts,
  agent traces, voice pipeline traces, and raw readiness diagnostics remain visible only in
  Developer Mode.
- Agents presents metadata-only Agent Registry, Task Runtime, Task Queue, Planning Sessions,
  Capability Registry, and Tool Contracts sections. No execute, approval, sandbox, tool runtime,
  plugin, filesystem, shell, or autonomous controls are added.
- Mode selection affects UI presentation only: Companion reduces diagnostics, Focus compacts the
  Home surface and reduces motion, and Mission/System/Tactical foreground additional telemetry.
- Provider posture is local-only: local Ollama loopback is the only current inference endpoint,
  and no cloud/API provider or key configuration is active.

Phase 16.0 through Phase 16.6 add a controlled semantic memory candidate foundation and explicit
review flow beside, not inside, the existing memory contracts:

- `MemoryCandidate`, `MemoryCandidateId`, `MemoryCandidateSource`,
  `MemoryCandidateCategory`, `MemoryCandidateConfidence`, `MemoryReviewState`,
  `MemoryCandidateReviewAction`, `MemoryCapturePolicy`, `MemoryCandidateSummary`, and
  `MemoryCandidateReviewResult` are value-only metadata records.
- `MemoryRetentionPolicy` remains the shared taxonomy retention vocabulary.
- `IMemoryCandidateStore` owns reviewable candidate metadata separately from `IMemoryStore` and
  `IMemoryCatalog`.
- `InMemoryMemoryCandidateStore` provides deterministic runtime/test storage only.
- Candidates may be created from conversation text metadata supplied to the controller and default
  to Pending Review.
- Approve, reject, reset-to-pending, and archive actions change review metadata only and do not
  commit entries to key-value memory or any long-term semantic store.
- Reviewed timestamp, reviewer/source summary, and decision reason values are review metadata.
  Approved means reviewed for possible future commitment; it does not mean stored long-term.
- `DesktopShellViewModel` exposes only QML-safe ids, states, counts, summaries, state-filtered
  summaries, and last review result strings to QML.

Phase 16.7 through Phase 16.9 add commit-planning metadata on top of approved candidates:

- `MemoryCommitPlan`, `MemoryCommitTarget`, `MemoryCommitReadiness`, `MemoryCommitResult`, and
  `MemoryCommitPolicy` are value-only records.
- A candidate can move from Pending Review to Approved review metadata, then receive a future
  commit plan. Committed memory remains a separate state that does not exist operationally yet.
- Approved candidates generate deterministic candidate-to-key-value-memory summaries, but the
  default policy keeps commit disabled.
- Pending, rejected, archived, or missing candidates cannot commit and report safe readiness
  reasons.
- Commit requests currently refuse before calling `IMemoryStore::put()`, so approval and commit
  planning never mutate key-value memory.
- The key-value memory store remains the only existing persistence target. No semantic memory
  persistence target exists.
- `DesktopShellViewModel` exposes only QML-safe commit readiness strings, checks, counts, target
  summaries, per-candidate plan summaries, and last-result strings.

Phase 16.10 through Phase 16.12 add the explicit user-controlled memory commit boundary:

- `MemoryCommitStatus`, `MemoryCommitConflictPolicy`, and `MemoryCommitResult` describe committed
  state, duplicate-key behavior, committed key, timestamp, status, and summary.
- The default policy allows only explicit user commit actions and refuses existing keys. There is
  no automatic commit during approval and no autonomous memory mutation.
- Commit keys are sanitized deterministic key-value memory keys derived from candidate category,
  title, and id.
- `ApplicationController::requestMemoryCandidateCommit()` commits only approved candidates, only
  when the key-value memory store is available, and only when the explicit policy gate allows the
  user action.
- Pending, rejected, archived, missing, store-unavailable, already-committed, and duplicate-key
  requests refuse safely before calling `IMemoryStore::put()`.
- The committed key-value value is the reviewed candidate content only. Source/review metadata is
  retained in the commit result and candidate committed summary because `IMemoryStore` stores only
  exact key/value pairs.
- `IMemoryCandidateStore` records committed candidate metadata separately from the key-value memory
  entry so QML can show committed status, committed key, timestamp summary, and committed count.

Phase 16.13 through Phase 16.15 add local memory recall metadata over committed key-value memory:

- `MemoryRecallQuery`, `MemoryRecallResult`, `MemoryRecallSummary`, `MemoryRecallStatus`, and
  `MemoryRecallPolicy` are value-only read-model records.
- `ApplicationController::recallLocalMemory()` reads only `IMemoryStore::entries()` and performs
  literal key/value matching. It is deterministic and read-only.
- Empty recall queries report Empty Query and do not mutate memory, chat, candidates, providers, or
  runtime state.
- Recall summaries expose only QML-safe strings, counts, and result lists through
  `DesktopShellViewModel`.
- Recall results are not injected into chat prompts or provider/model requests.

Phase 16.16 through Phase 16.18 add context assembly planning metadata over existing read models:

- `ContextAssemblyRequest`, `ContextAssemblySource`, `ContextAssemblyResult`,
  `ContextAssemblyStatus`, `ContextAssemblyPolicy`, and `ContextAssemblySummary` are value-only
  records.
- Planning estimates which future context sources would participate: conversation context,
  committed key-value memory context, runtime metadata context, and orchestration metadata context.
- Planning produces deterministic source summaries, source counts, candidate block counts, and
  simple character-size estimates only.
- `ApplicationController` derives the read model from current transcript messages, committed
  memory entries, conversation runtime summaries, and orchestration snapshot summaries.
- `DesktopShellViewModel` exposes only QML-safe strings, counts, and lists.
- Prompt assembly, prompt injection, automatic context attachment, provider/model calls, semantic
  ranking, embeddings, vector databases, tools/plugins, and filesystem/system actions remain
  disabled.

Phase 16.19 through Phase 16.21 add safe prompt context injection for local Ollama requests:

- `PromptContextBlock`, `PromptContextBundle`, `PromptContextInjectionPolicy`,
  `PromptContextInjectionStatus`, and `PromptContextInjectionResult` describe bounded local context
  assembly output.
- Injection is disabled by default and is enabled only through explicit user/app settings.
- When enabled, `ApplicationController` assembles deterministic context after local inference busy,
  model, endpoint, permission, and safety gates pass.
- The only allowed sources are current conversation context, committed `IMemoryStore` key-value
  entries, conversation runtime summaries, and orchestration summary metadata.
- Pending, rejected, archived, and merely approved memory candidates are excluded unless they have
  been explicitly committed to key-value memory.
- The prompt context block is clearly delimited, character-budgeted, and truncated in deterministic
  source order before being prepended to the local Ollama prompt.
- QML receives enabled/status/summary, block counts, source summary, size summary, and compact
  block summaries only. It does not receive the raw assembled prompt or private context payload.

This foundation deliberately does not add embeddings, a vector database, semantic search,
autonomous capture, model/provider calls, cloud sync, tool/plugin authority, filesystem/system
authority, or automatic writes to long-term memory.

Phase 16.22 through Phase 16.24 add deterministic conversation window management to that prompt
context path:

- `ConversationWindowPolicy`, `ConversationWindowSummary`, `ConversationWindowResult`,
  `ConversationWindowStatus`, and `ConversationWindowBudget` are value-only records.
- The controller assembles a bounded recent-message conversation-history window before prompt
  context injection. Recent messages are considered first for the character budget, and included
  messages are emitted back in stable chronological order.
- The current user prompt remains outside the conversation-history window and stays in the
  existing `User prompt:` section, so the bounded history does not duplicate the active prompt.
- Prompt context sources remain separated: bounded conversation history, committed key-value
  memory, runtime metadata, and orchestration metadata are distinct delimited blocks.
- Budget summaries expose estimated, included, omitted, and truncated counts through QML-safe
  strings and integers only. The raw assembled prompt is still not exposed to QML.

The window foundation deliberately does not add semantic ranking, embeddings/vector databases,
summarization, model/provider routing changes, cloud/API-key behavior, tools/plugins,
filesystem/system actions, or broad UI redesign.

Phase 16.25 through Phase 16.27 add deterministic older-conversation summary blocks to the same
bounded prompt context path:

- `ConversationSummaryPolicy`, `ConversationSummaryStatus`, `ConversationSummaryResult`,
  `ConversationSummaryBlock`, `ConversationSummaryWindow`, and `ConversationSummaryBudget` are
  value-only records.
- The controller derives summary candidates from transcript messages omitted by the recent
  conversation window, preserving original message indexes, chronology, and visible roles.
- Summary blocks are deterministic local compaction only: grouped transcript excerpts with bounded
  truncation. They are not semantic summaries and do not call providers/models.
- Prompt context sources remain separated: recent conversation window, older conversation summary,
  committed key-value memory, runtime metadata, and orchestration metadata are distinct delimited
  blocks.
- Summary metadata reports block counts, summarized/omitted message counts, included character
  counts, and truncated block counts through QML-safe strings and integers only.

Future semantic summarization, embeddings, vector databases, semantic ranking/search, automatic
memory writes, provider/model calls, tools/plugins, cloud/API keys, and filesystem/system actions
remain separate future phase gates.

Phase 16.28 through Phase 16.30 add deterministic retrieval planning before prompt context
injection:

- `RetrievalPlanningPolicy`, `RetrievalPlanningStatus`, `RetrievalPlanningResult`,
  `RetrievalCandidate`, `RetrievalSourcePriority`, `RetrievalBudget`, and
  `RetrievalSelectionSummary` are value-only records.
- Planning decides which local context sources participate without mutating prompts during the
  planning step.
- Source priority is fixed: bounded recent conversation, deterministic older conversation
  summaries, committed key-value memory, runtime metadata, then orchestration metadata.
- Budget allocation and truncation are deterministic character-budget operations. Selected,
  excluded, and truncated counts are retained per source.
- Prompt context injection consumes only the selected retrieval candidates while preserving source
  separation. Conversation chronology remains owned by the conversation window and summary
  assemblers.
- `DesktopShellViewModel` exposes only QML-safe status, readiness, source counts, excluded counts,
  budget summaries, and source summaries.

Semantic retrieval remains a separate future boundary. The current retrieval plan does not build
embeddings, use a vector database, rank semantically, call providers/models, write memory, expose
raw prompts, execute tools/plugins, or perform filesystem/system actions. Future vector or
embedding compatibility must plug in behind a later explicit retrieval/ranking boundary while
keeping deterministic planning and source separation intact.

Phase 16.31 through Phase 16.33 add the embedding/vector abstraction foundation:

- `SemanticRetrieval.h` defines value-only embedding, vector index, vector search, and semantic
  retrieval policy/status records.
- `IEmbeddingProvider` is the future boundary for embedding providers. The desktop runtime does
  not configure a real provider yet.
- `IVectorIndex` is the future boundary for vector indexes. The desktop runtime does not configure
  a vector database or durable semantic store yet.
- `FakeEmbeddingProvider` and `FakeVectorIndex` are deterministic local fakes for tests only.
  Fake embeddings are numeric vectors derived from stable text hashing and token counting; fake
  vector search is in-memory only with deterministic scoring and tie ordering.
- `ApplicationController` exposes only summary metadata: semantic retrieval disabled state,
  embedding provider readiness, vector index readiness, indexed item count, and readiness checks.
- QML does not receive raw vectors, vector scores, prompt payloads, providers, indexes, or storage
  handles.

The abstraction layer deliberately does not affect retrieval planning or prompt assembly. Current
prompt context still comes only from deterministic selected local context candidates. Semantic
ranking, semantic prompt injection, real embedding providers, vector databases, provider/model
calls, filesystem writes, downloads, plugins/tools, and system execution remain later explicit
phase gates.

Phase 16.34 through Phase 16.36 add semantic candidate orchestration metadata on top of the
existing deterministic context sources:

- `SemanticCandidate`, `SemanticCandidateSource`, `SemanticCandidateSelection`,
  `SemanticCandidateBudget`, `SemanticCandidateWindow`, `SemanticCandidateArbitration`,
  `SemanticCandidateSummary`, `SemanticCandidateStatus`, `SemanticCandidatePolicy`,
  `HybridRetrievalStatus`, and `HybridRetrievalPolicy` are value-only readiness records.
- Candidate sources cover recent conversation windows, deterministic summaries, committed
  key-value memory, runtime metadata, orchestration metadata, and a disabled future semantic/vector
  source placeholder.
- Arbitration is deterministic: fixed source order, character budgeting, exclusion/truncation
  metadata, source isolation, and conversation-source chronology preservation.
- Hybrid readiness states that deterministic retrieval remains authoritative. The semantic path is
  disabled and cannot inject into prompts.
- `ApplicationController` and `DesktopShellViewModel` expose only safe counts, statuses, budget
  summaries, arbitration summaries, participation summaries, readiness checks, and disabled state.
- Prompt context injection still consumes only deterministic `RetrievalPlanningResult` selections.
  Semantic candidate orchestration does not mutate prompts or expose raw semantic/vector payloads.

Future semantic activation must be a separate phase gate that wires a real ranker/index behind the
existing interfaces, keeps deterministic fallback authoritative, preserves source separation, and
adds tests before any semantic candidate can affect prompt assembly.

Phase 16.37 through Phase 16.39 checkpoint the full Phase 16 memory/context/retrieval architecture:

- Memory candidates, review metadata, explicit commit, literal recall, context assembly, prompt
  injection, conversation windows, deterministic summaries, retrieval planning, embedding/vector
  abstractions, semantic candidate orchestration, hybrid readiness, and QML exposure were audited
  together.
- Deterministic retrieval planning remains the only authoritative selector for prompt context.
- Semantic candidate orchestration remains disabled readiness metadata and does not feed
  `PromptContextBlock` creation or prompt injection.
- Runtime semantic readiness remains Not Configured for embedding providers and vector indexes,
  with zero indexed items.
- QML exposure remains limited to safe strings, string lists, booleans, counts, and readiness
  checks.
- The checkpoint is recorded in `docs/PHASE_16_MEMORY_CONTEXT_CHECKPOINT.md` and does not add
  embeddings, vector databases, semantic ranking/search, provider/model calls, API keys,
  tools/plugins, filesystem/system authority, semantic prompt injection, or runtime authority
  expansion.

Phase 17.0 through Phase 17.3 add semantic provider planning and local provider selection metadata:

- `SemanticProviderDescriptor`, `SemanticProviderSelection`, `SemanticProviderReadiness`,
  `SemanticProviderHealth`, `SemanticProviderCapability`, `SemanticProviderPolicy`,
  `SemanticActivationReadiness`, and `SemanticActivationResult` describe future semantic
  activation gates.
- Planned provider modes are Disabled, Fake/InMemory test provider, Local Ollama embeddings
  provider, and Local file/vector index.
- The desktop runtime selects Disabled by default. Activation readiness reports Refused and lists
  later-phase requirements.
- Local Ollama embeddings and local file/vector index are planned-only descriptors. They do not
  issue embedding requests, create/write vector indexes, scan filesystems, download models, call
  cloud APIs, use API keys, execute tools/plugins, or mutate prompts.
- Deterministic retrieval planning remains the authoritative prompt-context selector. Semantic
  provider readiness cannot influence prompt assembly in this phase.
- `ApplicationController` and `DesktopShellViewModel` expose only QML-safe provider mode/name,
  readiness, health, capability summaries, activation readiness, activation summary, and required
  activation steps. Raw config paths, vectors, scores, provider handles, index handles, and prompt
  payloads remain unexposed.

Phase 17.4 through Phase 17.6 add safe hybrid retrieval arbitration simulation and embedding
runtime planning:

- `SemanticArbitrationPolicy`, `SemanticArbitrationStatus`, `SemanticArbitrationResult`,
  `SemanticCandidateScore`, and `SemanticBudgetSummary` describe simulated future semantic
  ranking over existing deterministic candidate metadata.
- The simulation is deterministic and local: fixed source weights, bounded content-size buckets,
  stable source ordering, and candidate-id tie handling. It does not generate embeddings, search a
  vector index, call providers/models, or inspect the filesystem.
- `EmbeddingRuntimePlan`, `EmbeddingRuntimeBudget`, and `EmbeddingRuntimeReadiness` describe
  future local embedding runtime requirements, estimated jobs/items, rough memory/storage cost,
  and activation blockers.
- Deterministic `RetrievalPlanningResult` remains the only source consumed by
  `PromptContextBlock` creation. Simulated semantic rankings are exposed only as summaries and
  checks and cannot alter prompt context injection.
- QML exposure remains compact status/readiness metadata only. Raw vectors, raw score payloads,
  candidate payload dumps, index paths, provider handles, prompt payloads, and activation controls
  remain unexposed.

Phase 17.7 through Phase 17.9 add an isolated local embedding runtime activation foundation:

- `EmbeddingRuntimeStatus`, `EmbeddingRuntimeHealth`, `EmbeddingRuntimeSession`,
  `EmbeddingGenerationResult`, `EmbeddingGenerationPolicy`, `EmbeddingGenerationReadiness`, and
  `EmbeddingIsolationPolicy` describe bounded readiness-test execution.
- Isolated generation is permitted only when local-only mode, explicit semantic enable readiness,
  local/fake provider scope, no cloud providers, no filesystem indexing, no automatic prompt
  integration, no retrieval ranking mutation, no automatic memory writes, no vector persistence,
  and no background indexing gates all pass.
- Fake/InMemory embeddings can be generated through the isolated helper for deterministic tests.
  Local Ollama embeddings may be represented as a future local-only readiness/runtime path, but no
  desktop semantic retrieval route calls Ollama embeddings.
- Timeout, stale-request, busy-state, provider failure, and policy-refusal outcomes are explicit
  metadata. Result exposure reports counts, summaries, checks, health, and bounded session state
  only.
- Deterministic retrieval planning remains authoritative. Isolated embedding generation does not
  mutate prompt assembly, retrieval planning, retrieval ranking, key-value memory, vector indexes,
  filesystem indexes, or background jobs.
- QML receives readiness/status/health/bounds/check summaries only. Raw vectors, raw provider
  payloads, raw scores, debug dumps, provider handles, and index handles remain unexposed.

Phase 17.10 through Phase 17.12 add a disabled-by-default local vector persistence foundation:

- `VectorPersistencePolicy`, `VectorPersistenceStatus`, `VectorPersistenceHealth`,
  `VectorPersistenceReadiness`, `VectorPersistenceSession`, `VectorPersistenceBudget`,
  `VectorPersistenceResult`, `VectorIndexLifecycle`, and `VectorIndexSnapshotSummary` describe
  lifecycle metadata and stable local snapshots.
- `LocalVectorPersistenceIndex` is a deterministic local lifecycle helper for explicit create,
  reset, clear, and bounded acceptance of successful isolated embedding runtime output metadata.
- The policy is local-only, isolated, bounded, and disabled by default. It refuses automatic
  indexing, filesystem scanning, background ingestion, semantic retrieval authority, prompt
  mutation, automatic memory conversion, cloud/API providers, and external vector services.
- Empty indexes are valid safe states. Stale request ids and busy sessions refuse before state
  mutation. Bounded limits produce stable limit summaries.
- The desktop controller exposes only disabled readiness, lifecycle status, bounded state, item
  count, and checks. It does not persist raw vectors, expose filesystem paths, alter
  deterministic retrieval planning, mutate prompt assembly, or inject semantic prompt context.

Phase 17.13 through Phase 17.15 add controlled bounded local semantic search:

- `SemanticSearchPolicy`, `SemanticSearchStatus`, `SemanticSearchResult`,
  `SemanticSearchCandidate`, `SemanticSearchBudget`, `SemanticSearchSession`,
  `SemanticSearchReadiness`, and `SemanticSearchArbitrationSummary` describe search lifecycle and
  hybrid arbitration metadata.
- `LocalVectorPersistenceIndex` may search only its local persisted entry summaries. It does not
  index the filesystem, ingest in the background, call cloud/API/vector providers, or download
  providers.
- Search requires local-only deterministic policy gates, successful isolated embedding runtime
  output metadata, a non-stale request id, a non-busy session, bounded timeout metadata, bounded
  candidate count, and stable similarity/tie handling.
- Semantic candidates remain non-authoritative. They can expose bounded match metadata and hybrid
  arbitration summaries only.
- Deterministic retrieval planning remains the final authority. Semantic search cannot mutate
  `RetrievalPlanningResult`, `PromptContextBlock` values, prompt assembly, prompt injection, or
  deterministic ranking.
- Empty indexes, disabled persistence, stale requests, busy sessions, timeouts, and any policy
  request for prompt/retrieval authority resolve to safe deterministic statuses.
- QML receives only readiness, runtime state, counts, summaries, and checks. Raw vectors, provider
  handles, paths, prompt payloads, and debug payload dumps remain unavailable.

Phase 17.16 through Phase 17.18 add the bounded hybrid retrieval bridge:

- `HybridRetrievalBridgePolicy`, `HybridRetrievalBridgeStatus`,
  `HybridRetrievalBridgeResult`, `HybridBridgeCandidate`, `HybridBridgeBudget`,
  `HybridBridgeReadiness`, `HybridBridgeArbitration`, and `HybridBridgeSourceSummary` describe
  bridge metadata only.
- The bridge may read `RetrievalPlanningResult` selected candidates and `SemanticSearchResult`
  candidates. It produces bounded merged metadata, arbitration summaries, source participation
  summaries, and deterministic fallback summaries.
- Bridge ordering is deterministic-first. Deterministic retrieval candidates occupy capacity
  before semantic candidates, deterministic candidates win all ties, and semantic candidates may
  only fill unused bounded metadata capacity.
- The bridge cannot mutate `RetrievalPlanningResult`, create or mutate `PromptContextBlock`
  values, alter prompt assembly, inject prompt content, or override deterministic retrieval
  authority.
- Disabled, empty, stale, busy, timeout, and refused semantic sources keep deterministic retrieval
  fully functional through explicit fallback metadata.
- QML receives only bridge status, readiness, candidate counts, source/arbitration/fallback
  summaries, and checks. Raw vectors, raw prompt payloads, provider handles, filesystem paths, and
  debug dumps remain unavailable.

Phase 17.19 through Phase 17.21 add deterministic semantic acceptance:

- `SemanticAcceptancePolicy`, `SemanticAcceptanceStatus`, `SemanticAcceptanceResult`,
  `SemanticAcceptedCandidate`, `SemanticAcceptanceBudget`, `SemanticAcceptanceReadiness`,
  `SemanticAcceptanceArbitration`, `SemanticAcceptanceFallback`, and
  `SemanticAcceptanceSourceSummary` describe bounded supplemental acceptance metadata.
- Acceptance reads `RetrievalPlanningResult`, `HybridRetrievalBridgeResult`, and
  `SemanticSearchResult` values. Deterministic retrieval candidates remain primary; semantic
  candidates may be approved only as bounded supplements after deterministic candidates have
  occupied primary order.
- Approval gates require deterministic retrieval authority, semantic-supplement-only behavior,
  deterministic conflict wins, blocked prompt mutation, blocked retrieval-planning mutation,
  blocked prompt-context mutation, and unchanged source priority.
- Runtime protections cover disabled semantic sources, semantic errors/refusals, stale requests,
  busy sessions, timeout metadata, count budgets, supplement-character budgets, deterministic tie
  handling, and deterministic-only fallback mode.
- Accepted supplements remain explicitly semantic, local-only, non-authoritative, and summary-only.
  Acceptance cannot mutate `RetrievalPlanningResult`, create or mutate `PromptContextBlock`
  values, change deterministic ordering, replace deterministic candidates, alter retrieval source
  priority, or inject prompt content.
- QML receives only status/readiness, approved supplement counts, source participation,
  arbitration/fallback summaries, bounded budget state, and safety checks. Raw vectors, raw prompt
  payloads, provider handles, filesystem paths, and debug dumps remain unavailable.

Phase 17.22 through Phase 17.24 add semantic supplement prompt assembly readiness:

- `SemanticSupplementBlock`, `SemanticSupplementBundle`,
  `SemanticSupplementAssemblyPolicy`, `SemanticSupplementAssemblyStatus`,
  `SemanticSupplementAssemblyResult`, `SemanticSupplementBudget`,
  `SemanticSupplementReadiness`, and `SemanticSupplementSafetyReport` describe a future prompt
  supplement lifecycle.
- Assembly reads `SemanticAcceptanceResult` and, only when explicitly enabled for test-only
  assembly, produces a separate bounded metadata bundle from accepted semantic supplements.
- The desktop policy is disabled by default. Live prompt inclusion is blocked, so default prompt
  assembly and local Ollama prompt behavior are unchanged.
- Supplement bundles are separate from deterministic context blocks. They cannot replace,
  reorder, or override deterministic retrieval context, conversation windows, deterministic
  summaries, committed memory, runtime metadata, or orchestration metadata.
- Runtime protections include bounded supplement count, bounded character budget, deterministic
  ordering, deterministic truncation, disabled/empty fallback, stale/busy/timed-out/refused
  fallback, and safety-report checks.
- QML receives only supplement assembly status/readiness, block count, budget summary, safety
  summary, and checks. It does not receive raw prompt blocks, vectors, scores, provider handles,
  filesystem paths, or debug dumps.

Phase 17.25 through Phase 17.27 add the semantic prompt authority policy foundation:

- `SemanticPromptAuthorityPolicy`, `SemanticPromptAuthorityStatus`,
  `SemanticPromptAuthorityResult`, `SemanticPromptAuthorityReadiness`,
  `SemanticPromptAuthorityDecision`, `SemanticPromptAuthoritySafetyReport`,
  `SemanticPromptAuthorityFallback`, and `SemanticPromptAuthorityAuditSummary` describe the
  authority gate for future semantic prompt inclusion.
- The policy is disabled by default. The default result is Disabled/Denied and reports a
  deterministic-only fallback.
- Authority evaluation reads only `SemanticSupplementAssemblyResult`; it does not read raw prompt
  payloads, raw supplement blocks, vector data, provider handles, filesystem paths, or debug dumps.
- A test-only "would include metadata" decision requires local-only semantic search, deterministic
  acceptance, a bounded supplement bundle, an explicitly enabled prompt-injection setting, an
  explicit authority-policy allow flag, and a passing safety report.
- Live prompt mutation remains blocked. Even an allowed-readiness decision is supplemental,
  clearly delimited metadata only; deterministic retrieval remains authoritative and default
  prompt assembly is unchanged.
- Memory and Settings expose only compact status, decision, readiness, fallback, safety, audit,
  count, and check summaries.

Phase 17.28 through Phase 17.30 add controlled semantic prompt inclusion:

- `SemanticPromptInclusionPolicy`, `SemanticPromptInclusionStatus`,
  `SemanticPromptInclusionResult`, `SemanticPromptInclusionBudget`,
  `SemanticPromptInclusionSafetyReport`, `SemanticPromptInclusionFallback`, and
  `SemanticPromptInclusionAuditSummary` describe the live inclusion gate.
- Inclusion is disabled by default. It can activate only when prompt context injection is enabled,
  semantic prompt authority approves, semantic supplement assembly is bounded and safe, local-only
  mode is active, and the inclusion safety report passes.
- The inclusion step runs after deterministic prompt context injection. It appends a separate
  clearly delimited semantic block after deterministic context blocks and before `User prompt:`.
- The block is labeled supplemental/non-authoritative, count-bounded, character-budgeted,
  deterministically ordered, and deterministically truncated.
- Deterministic retrieval remains final authority. Semantic supplements cannot replace or reorder
  deterministic context, override committed memory, override deterministic summaries, override the
  conversation window, or override runtime metadata.
- Disabled, denied, unsafe, empty, stale, busy, timed-out, and refused semantic states return the
  exact deterministic-only prompt.
- Controller/view-model/QML exposure is limited to enabled/status, included count, budget,
  fallback, audit, authority-preserved state, and checks. Raw prompt text, raw supplement blocks,
  vectors, scores, provider handles, filesystem paths, and debug payloads remain hidden.

Phase 17.31 through Phase 17.33 checkpoint the Phase 17 semantic retrieval and prompt inclusion
architecture:

- `docs/PHASE_17_SEMANTIC_CHECKPOINT.md` records the audit.
- Deterministic retrieval planning remains the final authority for prompt context selection.
- Semantic inclusion remains disabled by default and explicit opt-in through AppSettings and the
  inclusion policy.
- Semantic supplements can enter prompts only after deterministic context, only as a clearly
  delimited supplemental/non-authoritative block, and only when local-only, authority, assembly,
  budget, and safety gates pass.
- Disabled, denied, unsafe, empty, stale, busy, timed-out, and refused semantic states return
  deterministic-only prompts.
- QML remains limited to safe status, count, summary, fallback, audit, and check metadata. Raw
  prompts, supplement payloads, vectors, scores, provider handles, filesystem paths, and debug
  dumps are not exposed.
- No filesystem indexing, cloud/API/vector provider activation, provider download, tool/plugin,
  autonomous action, or runtime authority expansion is introduced by the checkpoint.

## Chat History Storage Contract

`IChatHistoryStore` is the persistence boundary for ordered chat messages. It is separate from `IMemoryStore` and must not be used for key-value memory entries.

`SQLiteChatHistoryStore` stores:

- `id`
- `role`
- `content`
- `timestamp`
- `status`

Rows load in ascending `id` order so the transcript remains deterministic across app launches.

The desktop app stores chat history below Qt's `AppDataLocation` as `chat_history.sqlite3`. The database includes a `chat_history_schema_metadata` table with `schema_version = 1`.

The desktop app stores transcript exports below Qt's `AppDataLocation` in an `exports` directory.
Exports are separate Markdown/JSON files and are not part of the chat-history SQLite store.

If chat persistence is unavailable, `ApplicationController` continues with the in-memory `ChatSession`. Clearing chat clears the persistent chat table only when the store is available.

The desktop shell exposes generic chat history and conversation-history UX metadata only. QML can
show whether the active transcript is Persisted or Runtime Only, message counts, last save status,
and last restore status. It does not know the database path, schema, driver, or last SQLite error.

`ConversationHistorySummary`, `ConversationPersistenceStatus`, and `ConversationClearResult` are
value-only metadata records derived by `ApplicationController`. They do not introduce
multi-conversation storage, transcript search, import/export, pruning, encryption, or any
filesystem access beyond the existing app-owned chat-history store.

Phase 15.11 through Phase 15.13 add lightweight transcript search and export-readiness metadata.
Phase 15.14 through Phase 15.16 implement controlled local transcript export without changing the
chat-history storage contract:

- `ConversationSearchQuery`, `ConversationSearchResult`, and `ConversationSearchSummary` describe
  literal case-insensitive search over the current in-memory `ChatSession` messages only.
- Search reads the active transcript and updates controller-owned summary metadata. It does not
  mutate chat history, query SQLite, create indexes, build embeddings, perform semantic/vector
  search, or persist search state.
- Clear Chat resets search metadata after the single initial system message is reseeded.
- `ConversationExportFormat`, `ConversationExportRequest`, `ConversationExportReadiness`, and
  `ConversationExportResult` describe current-transcript Markdown/JSON export only.
- Export writes only to the app-controlled export directory below Qt `AppDataLocation` through the
  path-provider boundary. The desktop app uses `StandardPathProvider::conversationExportDirectoryPath()`.
- Export filenames are sanitized, timestamped, and made unique before writing with `QSaveFile`.
- Empty transcripts with no user or assistant messages are refused.
- `ApplicationController::exportTranscript(format)` is the controller entry point. QML receives
  only safe result strings, the output filename, message count, and timestamp; it does not receive
  raw filesystem paths.
- `DesktopShellViewModel` exposes only QML-safe strings, string lists, booleans, and counts for
  search/export state.

Phase 15.17 through Phase 15.19 add a single-transcript conversation-browser foundation without
changing chat-history persistence:

- `ConversationDisplayTitle`, `ConversationListEntry`, `ConversationListSummary`, and
  `ConversationBrowserStatus` are value-only metadata records.
- `ApplicationController` derives exactly one current transcript entry from existing
  `ChatSession`/history/search/export metadata.
- `DesktopShellViewModel` exposes browser values as QML-safe strings and counts only.
- No multi-conversation/thread storage, migration, import/export redesign, cloud sync, or broader
  filesystem authority is introduced.

Phase 15.20 through Phase 15.22 add metadata-only multi-conversation planning readiness on the same
boundary:

- `ConversationId`, `ConversationDescriptor`, `ConversationLifecycleStatus`,
  `ConversationStorageMode`, `ConversationMigrationReadiness`, and `ConversationSchemaPlan` model
  future multi-conversation migration intent.
- `ApplicationController` exposes read-only readiness values for current storage mode (`Single
  Transcript`), future storage mode (`Multi Conversation`), migration readiness (`Not Started`),
  migration status summary (`Not Started / Planned`), and schema status summary.
- `DesktopShellViewModel` forwards those values as QML-safe strings.
- No SQLite schema migration, no multi-conversation storage, and no browser/thread controls are
  introduced.

Phase 15.23 through Phase 15.25 add the first real multi-conversation storage boundary without
switching the active chat transcript:

- `IConversationStore` owns future multi-conversation records and message records separately from
  `IChatHistoryStore`.
- `ConversationRecord`, `ConversationMessageRecord`, `ConversationStoreStatus`, and
  `ConversationStoreError` are value types safe to summarize through controller/view-model layers.
- `InMemoryConversationStore` supports deterministic tests and runtime-only storage.
- `SQLiteConversationStore` uses Qt SQL/QSQLITE for `conversations`, `conversation_messages`, and
  `conversation_schema_metadata` in its own database file.
- `ApplicationController` exposes conversation-store readiness as QML-safe strings, counts, and
  string lists only. It does not route current chat messages into the multi-conversation store.
- No destructive migration is performed. Existing `chat_history.sqlite3` and `IChatHistoryStore`
  behavior remain the active single-transcript path.

Phase 15.26 through Phase 15.29 make `IConversationStore` the active local transcript source while
preserving the older single-transcript history as a compatibility source when the conversation
store is empty:

- `StandardPathProvider::conversationDatabasePath()` owns the app data
  `conversations.sqlite3` path. The desktop app wires `SQLiteConversationStore` there.
- `ApplicationController` owns `activeConversationId_`. Startup ensures the active id is valid,
  creates an initial conversation if needed, and copies loaded legacy `IChatHistoryStore` messages
  into the empty conversation store when possible.
- New user/assistant/system chat messages are appended to the active conversation through
  `IConversationStore`. The visible `ChatSession` is a loaded transcript view of the active
  conversation.
- Switching conversations invalidates active async request metadata, clears live streaming preview
  text, resets conversation runtime/search metadata, loads selected conversation messages, and
  emits only QML-safe summary/list values.
- Async inference completion remains request-id guarded. A stale completion from a previous active
  conversation cannot append an assistant message or alter the newly active transcript.
- Archived conversations remain listable/loadable and can be unarchived. Sending into an archived
  active conversation is refused.

Phase 15.30 through Phase 15.32 add browser-polish and delete-readiness metadata on the same
boundary:

- `ConversationDeletePolicy`, `ConversationDeleteReadiness`, and `ConversationDeleteResult` are
  value-only safety records. They do not expose storage internals to QML.
- The current policy is archive-first. Permanent delete is disabled by default and requires a
  later explicit phase gate, destructive-mutation tests, and guarded confirmation UI before it can
  mutate storage.
- `ApplicationController` exposes QML-safe active/archived state summaries, active and archived
  counts, empty-state text, delete readiness, delete policy, and last delete-request result.
- `requestPermanentDeleteConversation()` is a guarded refusal path only. It reports a refused
  result and does not call `IConversationStore::deleteConversation()` or mutate transcripts.
- Archived active conversations stay loadable and read-only for sending, with UI hint metadata
  explaining that the conversation must be unarchived before sending.

Phase 24.0 through Phase 24.6 complete safe local conversation persistence:

- `ConversationRecord` includes persisted pinned metadata. `IConversationStore` exposes
  `pinConversation()` and `unpinConversation()` behind the same local store boundary.
- `SQLiteConversationStore` schema version 2 adds the `pinned` column with a non-destructive
  additive migration for existing local conversation databases.
- `ApplicationController::conversationRecords()` returns deterministic browser ordering: pinned
  non-archived records first, recent non-archived records next, and archived records last, with
  updated-time/title/id tie handling.
- `ApplicationController::duplicateConversation()` creates a local duplicate titled
  `Original title Copy` and copies transcript messages through `IConversationStore` load/append
  operations. It does not export files, import files, sync cloud state, or switch the active
  conversation automatically.
- Duplicate and pin results are exposed to QML as strings, counts, booleans, and string lists only.
  Raw store objects and SQLite details remain unexposed.
- Permanent delete remains disabled. The controller delete request path still refuses before
  calling `IConversationStore::deleteConversation()` and reports the safe readiness copy.

No cloud sync, import, permanent delete UI, multi-conversation export, vector/semantic search,
tool/plugin/system execution, or Ollama safety policy change is introduced.

Phase 15.33 through Phase 15.35 checkpoint the conversation browser runtime architecture:

- Multi-conversation storage remains behind `IConversationStore`; QML receives only view-model
  strings, string lists, booleans, and counts.
- Legacy `IChatHistoryStore` data remains a compatibility startup source and is not destructively
  migrated or cleared by conversation-store initialization.
- `SQLiteConversationStore::deleteConversation()` is a soft metadata operation, while the current
  controller permanent-delete request path refuses before calling it.
- Archived conversations remain loadable/readable and reject new appends through controller and
  store checks.
- Conversation switching resets active request metadata/live preview/search state and stale async
  completions remain ignored by request-id guards.

No semantic memory, embeddings/vector DB, cloud sync, import/export changes, permanent-delete
execution, broad UI redesign, model/voice/tool/plugin changes, or runtime authority expansion is
introduced by this checkpoint.

## Conversation Session Metadata

Phase 6.8 adds `ConversationSession` as a higher-level interaction/session metadata layer. It is
not a replacement for chat history and it is not the Phase 4 agent runtime context.

Separation:

- `ChatSession` owns in-memory chat messages and cooperates with `IChatHistoryStore` persistence.
- `ConversationSession` owns deterministic interaction metadata: session id/status, interaction
  mode, attention state, and a `RuntimeContextWindow` summary built from routing mode, preferred
  agent, memory affinity, and the latest orchestration snapshot summary.
- Phase 4 `RuntimeSession` owns agent pipeline metadata derived from the latest
  `AgentPipelineResult`, including active planned tool ids and placeholder execution state.

`ConversationSession` is refreshed from already-owned metadata after routing/task planning and
agent-runtime metadata changes. It does not persist transcripts, create multi-conversation storage,
stream model output, call providers/models, scan filesystems, search memory, build embeddings,
execute tools, load plugins, call networks, or start background workers.

## Conversation State Graph Metadata

Phase 6.9 adds `ConversationStateGraph` as a deterministic state-machine skeleton for high-level
conversation flow metadata. It is separate from the conversation session, chat transcript, and
agent runtime metadata.

Separation:

- `ConversationStateGraph` owns only the current state and last transition result.
- `ConversationSession` owns session identity, interaction mode, attention state, and context
  window metadata.
- `ChatSession` owns in-memory chat messages and chat history persistence coordination.
- Phase 4 `RuntimeSession` owns agent pipeline context metadata.

The graph supports deterministic states such as Idle, Listening, Planning, Routing, Waiting For
Approval, Ready To Respond, Responding, Completed, and Error. Invalid transitions are rejected with
metadata summaries and do not change state. Accepted transitions do not execute providers, execute
models, stream tokens, run tools, grant approvals, load plugins, scan filesystems, call networks,
perform semantic/vector search, or start autonomous workers.

## Phase 6 Checkpoint

Phase 6.10 closes the metadata-only orchestration foundation before Phase 7. The checkpoint is
documented in `docs/PHASE_6_CHECKPOINT.md`.

Architecture findings:

- Provider catalog, model routing, task planning, agent registry, memory taxonomy, orchestration
  snapshot, diagnostics/readiness, conversation session, and conversation state graph remain
  separate metadata responsibilities.
- `ApplicationController` owns orchestration metadata and exposes deterministic summary/status
  values.
- `DesktopShellViewModel` remains the QML boundary and does not expose raw core objects.
- Phase 7.0 should begin with local runtime boundary planning/ownership, not full model execution
  unless an explicit later scope authorizes it.

The checkpoint adds no provider integration, model execution, networking, API keys, downloads,
streaming, tool execution, plugin loading, filesystem/system actions, embeddings, semantic search,
or autonomous workers.

## Local Runtime Boundary

Phase 7.0 adds `ILocalRuntime` as the future local inference/runtime ownership boundary. The
current implementation is `NullLocalRuntime`, which exposes deterministic metadata and refuses
execution.

Separation:

- `ILocalRuntime` is not `IChatProvider`; it does not generate chat responses.
- `ILocalRuntime` is not `IModelRouter`; it does not select provider/model routes.
- `ILocalRuntime` is not `IAgentRuntime`; it does not plan or run agent actions.
- `ILocalRuntime` is not `IToolExecutor`; it does not execute tools or planned invocations.

The boundary reports local runtime status, health, capability summaries, and a placeholder refusal
response. It does not call Ollama or any provider, execute models, download models, stream output,
launch processes/subprocesses, scan filesystems, execute tools, load plugins, access networks, read
API keys, or start background workers.

## Local Inference, Model Selection, Explicit Chat Routing, And Streaming

Phase 9.3 through Phase 9.8 add a controlled local inference path and selected-model metadata.
Phase 10.0 through Phase 10.2 add explicit chat-to-Ollama routing on top of that boundary while
keeping the existing local-safe provider path as the default. Phase 10.3 through Phase 10.5 add
guarded local-only streaming for chat responses. Phase 10.6 through Phase 10.8 add action-light
model selection and runtime management UX over discovered local metadata. Phase 11.0 through Phase
11.2 add model-management readiness metadata only. Phase 11.3 through Phase 11.6 stabilize local
AI chat/runtime UX with safer summaries, clearer model-selection presentation, and streaming
preview finalization without adding new execution capabilities. Phase 11.7 through Phase 11.9
checkpoint the local AI user flow and runtime QA posture with focused tests and
`docs/PHASE_11_CHECKPOINT.md`; they do not add runtime authority or product features.

Separation:

- `ILocalInferenceClient` owns non-streaming local prompt execution.
- `OllamaLocalInferenceClient` is restricted to local loopback HTTP and non-streaming
  `/api/generate`.
- `ILocalInferenceStreamClient` owns streaming local prompt execution and reports chunk/status,
  error, cancellation, malformed-chunk, trace, and accumulated-text metadata.
- `OllamaLocalInferenceStreamClient` is restricted to local loopback HTTP `/api/generate`
  streaming and uses manual redirect policy. It does not call cloud endpoints, use API keys,
  download/pull/delete models, launch subprocesses, or execute tools.
- `ILocalInferenceWorker` is the async worker boundary above those clients. The desktop wiring
  runs real Ollama non-streaming and streaming calls on a worker thread and posts request-id
  guarded chunks/results back to `ApplicationController`.
- `ApplicationController` evaluates runtime permission and safety metadata before invoking local
  inference or streaming inference, then finalizes chat only from the matching active request id.
- `ApplicationController` owns a concise conversation-runtime read model for the current
  transcript session: current graph state, current request id, active model, active route, active
  streaming flag, last success summary, last error/refusal summary, and last latency summary.
  `DesktopShellViewModel` exposes these as QML-safe strings, string lists, and booleans only.
- `AppSettings` persists the selected local model as configuration only.
- `AppSettings` also persists local chat inference enablement. The default is disabled.
- `AppSettings` persists local inference streaming enablement. The default is disabled.
- `DesktopShellViewModel` exposes selected-model summaries, runtime badge, busy/idle/error state,
  latency summary, trace strings, local chat routing status/summary, streaming
  enabled/status/live-text/summary values, discovered model names, model metadata summaries, and
  selected model status as QML-safe values only.

Model selection behavior:

- An explicit model passed to `runLocalInference(prompt, model)` wins.
- If no explicit model is provided, the selected local model is used.
- If neither is present and discovered local model metadata is available, the first discovered
  model is used as a fallback candidate.
- If discovered model metadata is available and the selected/effective model is absent, inference
  is rejected before the inference client is called.
- If no model can be resolved, inference is rejected as an invalid request.
- Settings can select from discovered local model names only when metadata is available.
- Selected-model UX reports Missing, Fallback, Available, Unverified, or Invalid state.
- Model metadata display is limited to name, size when available, modified date when available,
  and Local Only status.

Streaming behavior:

- Streaming is disabled by default through settings.
- Chat streaming activates only when local chat inference is enabled, streaming is enabled and
  available, the selected/effective model is valid, the endpoint is loopback HTTP, and runtime
  permission/safety gates pass.
- Chunks update QML-safe live response text only while the stream is active. When streaming
  completes, the live preview is cleared and the final assistant text is appended once through
  normal chat history. Malformed chunks are ignored with metadata; timeout, cancellation, refusal,
  and errors produce safe summaries.
- Timeout metadata is carried for health checks, model discovery, non-streaming generation, and
  streaming generation. Failure summaries distinguish Ollama not running, unreachable local
  endpoint, missing selected model, selected model absent from discovery metadata, timeout,
  malformed response, interrupted stream, permission/safety block, and duplicate busy request.
- Failed streams clear live preview text and do not persist partial assistant output.
- The current worker provides request-id cancellation/stale-result metadata. It does not forcibly
  interrupt an in-flight Ollama HTTP request beyond the existing client timeout behavior.
- Clear Chat clears the in-memory transcript, clears persistent chat rows when available, reseeds
  the single system message through the chat-history boundary, resets transient runtime summaries,
  resets active request metadata and live streaming text, and returns the conversation graph to
  Idle. Restart loading uses persisted rows directly when present, so startup does not duplicate
  system or assistant messages.

Explicit chat routing:

- `ApplicationController::sendMessage` always appends the user message before routing.
- If a local inference request is already active, duplicate sends are rejected before appending
  another user message.
- If local chat inference is disabled, chat uses the existing `IChatProvider` path.
- If local chat inference is enabled, chat calls streaming or non-streaming local inference only
  after model resolution, loopback endpoint validation, runtime permission evaluation, and runtime
  safety evaluation.
- Successful local inference appends one assistant message from the local inference response.
- Refusals, invalid models, blocked endpoints, permission/safety denials, and client errors append
  one safe assistant error/refusal message. These messages avoid stack traces, secrets,
  filesystem paths, raw internal objects, provider credentials, and broad endpoint details.
- The chat route does not invoke tools, launch subprocesses, perform filesystem or system actions,
  call cloud providers, read API keys, download/pull/delete models, or start autonomous loops.

Phase 14.7 through Phase 15.0 activates this path in the desktop app for controlled local-only
Ollama chat. The production desktop wiring uses `OllamaHttpRuntimeClient`,
`OllamaLocalInferenceClient`, `OllamaLocalInferenceStreamClient`, and `ILocalInferenceWorker`
against the persisted loopback endpoint. Phase 15.8 moves generate/stream execution behind the
worker so controller/UI responsiveness does not depend on Ollama model-loading latency. A narrow
`LocalOnlyRuntimePermissionPolicy` allows only explicit local inference requests and denies
provider invocation, tool invocation, external processes, filesystem access, broader network
access, and plugin invocation. The existing static metadata-only permission policy remains
available for deterministic tests and blocked-runtime scenarios.

Controlled activation boundaries:

- Health checks call only local loopback HTTP `/api/version`.
- Model discovery calls only local loopback HTTP `/api/tags`.
- Non-streaming inference calls only local loopback HTTP `/api/generate` with `stream: false`.
- Streaming inference is opt-in and calls only local loopback HTTP `/api/generate` with
  `stream: true`.
- Real generation and streaming calls execute through the async local inference worker; QML sees
  only QML-safe busy/status/summary/live-text metadata.
- Chat invokes inference only from explicit user messages and only when local chat inference is
  enabled.
- Runtime state is exposed to QML as unavailable, idle, inferencing, streaming, or failed.
- No cloud provider, API key, autonomous agent, tool, shell, filesystem-wide action, model
  management action, Piper execution, Whisper execution, microphone access, or playback path is
  activated by local Ollama chat.

## Voice Boundary

Phase 12.0 through Phase 12.2 add a disabled voice planning boundary for future text-to-speech and
speech-to-text work. Phase 12.3 through Phase 12.6 add metadata-only voice runtime/session
orchestration on top of that boundary. Phase 12.7 through Phase 12.9 checkpoint the voice
architecture and document local Piper/Whisper integration prerequisites before any executable
voice work. Phase 13.0 through Phase 13.2 add metadata-only local voice runtime environment,
binary ownership, model ownership, permission, and safety reporting for future Piper/Whisper work.
Phase 13.3 through Phase 13.5 add a Piper TTS adapter skeleton with readiness/refusal metadata
only. Phase 13.6 through Phase 13.8 add controlled Piper local file-output synthesis behind the
same provider/client boundary. Phase 13.9 checkpoints the voice/Piper architecture and records
Phase 14 readiness without adding runtime behavior. Phase 14.0 through Phase 14.3 add persisted
local Piper/Whisper path configuration and exact-path validation metadata only. Phase 14.4 through
Phase 14.6 polish the voice configuration UX and add read-only hint metadata from fixed known
binary locations and configured paths only. Phase 15.1 through Phase 15.3 refine path setup with
explicit Apply Paths persistence, exact validation rows, and Ready/Blocked/Missing readiness for
Piper file-output preparation and future Whisper STT preparation without enabling voice execution.
Phase 15.4 through Phase 15.6 enable controlled Piper file-output execution only behind a
persisted opt-in and explicit user action, with output constrained to an app-controlled cache/temp
path and no playback, microphone, or Whisper execution.

Separation:

- `ITextToSpeechProvider` owns the future TTS provider boundary.
- `ISpeechToTextProvider` owns the future STT provider boundary.
- `NullTextToSpeechProvider` and `NullSpeechToTextProvider` are the current implementations.
- `VoiceCapability`, `VoiceProviderDescriptor`, `VoiceProviderStatus`, `VoiceRuntimeMode`,
  `VoiceRequest`, `VoiceResponse`, and `VoiceReadinessReport` are value-only metadata.
- `VoiceSession`, `VoiceSessionId`, `VoiceSessionState`, `VoicePipelineStage`,
  `VoicePipelineStatus`, `VoicePipelineTrace`, `IVoiceRuntimeCoordinator`, and
  `StaticVoiceRuntimeCoordinator` model future runtime/session orchestration without operating
  audio devices or binaries.
- `VoiceBinaryDescriptor`, `VoiceBinaryStatus`, `VoiceModelDescriptor`, `VoiceModelStatus`,
  `VoiceRuntimePermission`, `VoiceRuntimeSafetyReport`, `IVoiceRuntimeEnvironment`,
  `NullVoiceRuntimeEnvironment`, and `StaticVoiceRuntimeEnvironment` model future local Piper and
  Whisper binary/model ownership and safety posture without execution.
- `PiperTtsConfig`, `PiperVoiceModelDescriptor`, `PiperTtsRequest`, `PiperTtsResult`,
  `PiperTtsStatus`, `IPiperTtsClient`, `NullPiperTtsClient`, and
  `PiperTextToSpeechProvider` remain as compatibility/readiness metadata for the older file-output
  surface. They refuse before execution in the current phase.
- `PiperSynthesisPolicy`, `PiperSynthesisStatus`, `PiperSynthesisRequest`,
  `PiperSynthesisResult`, `PiperSynthesisSession`, `PiperSynthesisBudget`,
  `PiperSynthesisReadiness`, `PiperSynthesisSafetyReport`, `PiperSynthesisFallback`,
  `PiperSynthesisTrace`, `IPiperSynthesisClient`, `NullPiperSynthesisClient`, and
  `LocalPiperSynthesisClient` model the future Piper TTS synthesis boundary. The local client is a
  non-executing skeleton.
- `WhisperTranscriptionPolicy`, `WhisperTranscriptionStatus`, `WhisperTranscriptionRequest`,
  `WhisperTranscriptionResult`, `WhisperTranscriptionSession`,
  `WhisperTranscriptionBudget`, `WhisperTranscriptionReadiness`,
  `WhisperTranscriptionSafetyReport`, `WhisperTranscriptionFallback`,
  `WhisperTranscriptionTrace`, `IWhisperTranscriptionClient`,
  `NullWhisperTranscriptionClient`, and `LocalWhisperTranscriptionClient` model the future
  Whisper STT audio-file transcription boundary. The local client is a non-executing skeleton.
- `ApplicationController` exposes voice readiness, runtime, session, pipeline, and trace
  summaries plus binary/model/environment/permission/safety and Piper/Whisper readiness summaries
  only, including local voice configuration summaries, exact validation rows, Ready/Blocked/
  Missing preparation status, and Piper synthesis status/readiness/result/fallback/safety/trace
  summaries.
- `DesktopShellViewModel` exposes QML-safe strings, string lists, and booleans only. Voice
  configuration setters persist path strings and the Piper file-output opt-in through
  `AppSettings`; QML does not receive provider, process, filesystem, audio-device, or model
  objects.

Current behavior:

- Voice runtime mode is disabled.
- Voice runtime status is unavailable.
- TTS returns a safe disabled/refusal placeholder and produces no audio.
- STT returns a safe disabled/refusal placeholder and reads no audio.
- The static voice pipeline emits deterministic metadata for idle, preparing, awaiting-input,
  transcribing-placeholder, inference-placeholder, synthesis-placeholder, completed, blocked, and
  error states.
- Runtime summaries explicitly report runtime unavailable, TTS unavailable, STT unavailable,
  microphone disabled, playback disabled, local-only policy active, and process execution disabled.
- Runtime environment summaries report Piper and Whisper binary/model paths as missing or expected
  metadata only. They do not launch binaries, load models, scan the filesystem broadly, download
  assets, or open devices.
- Settings can store Piper binary path, Piper model path, Whisper binary path, and Whisper model
  directory/path. Validation checks only each configured path for exists/missing,
  readable/unreadable, and executable/non-executable for binaries.
- Settings can apply all four path fields explicitly. The same persisted settings still update
  readiness immediately through the view-model/controller boundary.
- Settings also shows read-only hints. Binary hints check only `/opt/homebrew/bin/piper`,
  `/usr/local/bin/piper`, `/opt/homebrew/bin/whisper`, and `/usr/local/bin/whisper`. Model hints
  check configured paths only. Hints are never applied to settings automatically.
- Piper file-output TTS preparation reports Ready only when the configured Piper binary exists as
  an executable file and the configured `.onnx` model path exists as a readable file. Blocked and
  Missing states explain the exact failed path checks. This preparation status does not execute
  Piper and is separate from the provider/client gates.
- Whisper STT preparation reports Ready only when the configured Whisper binary exists as an
  executable file and the configured model folder or model file exists and is readable. No Whisper
  adapter or execution path is added.
- Whisper transcription readiness additionally models a future local audio-file request. Missing
  binary, model, or audio metadata and unsafe/non-local path-style input are refused before any
  execution boundary. No transcript is injected into chat.
- Piper TTS is disabled/not configured by default. Missing or invalid binary/model paths produce
  deterministic refusal before any execution boundary can run.
- Piper synthesis readiness may report `Ready Metadata` only when the configured Piper binary and
  voice model are local path-style values that pass exact metadata checks. This is not an
  execution grant.
- `NullPiperSynthesisClient` refuses without side effects. `LocalPiperSynthesisClient` validates
  metadata and then refuses before subprocess execution.
- The legacy Piper file-output opt-in and generation action are non-operational compatibility
  paths in the current phase. They remain disabled/readiness-only and refuse without writing an
  audio file or reaching a process client.
- Piper synthesis status metadata is exposed as disabled, missing, unsafe, safety-blocked,
  refused, ready metadata, or timeout. No raw filesystem paths are exposed through the new
  readiness/safety summaries.
- The current TTS path is `text/config metadata -> Piper synthesis readiness -> deterministic
  refusal/fallback metadata`.
- Voice runtime safety blocks execution by default and denies microphone, playback, process
  execution, filesystem-wide scan, download, and cloud/API-key behavior.
- Settings shows compact voice configuration text fields plus voice readiness/runtime/session/
  pipeline metadata and Piper/Whisper readiness summaries, but no voice controls, setup actions,
  speak buttons, play buttons, record buttons, playback controls, downloads, or path pickers exist.
- No microphone access, audio playback, Piper execution, Whisper execution, live STT,
  filesystem-wide scan, model downloads, cloud calls, API keys, prompt injection, automatic chat
  send, automatic audio injection, or autonomous voice loop is present. The historical Piper
  file-output path is currently disabled/readiness-only compatibility metadata and refuses without
  subprocess execution, file output, or playback.

Phase 18.19-18.21 Whisper STT foundation:

- Settings and Agents show compact Whisper STT status/readiness/result/fallback/safety/trace
  summaries.
- There is no record button, no file picker, no enabled transcribe button, and no microphone or
  live-listening UI.
- Future controlled audio-file transcription must explicitly enable a bounded local execution
  gate. Future microphone/live STT must be a separate permission and lifecycle phase.

Future Piper/Whisper integration should happen only through these provider interfaces and the
runtime coordinator/environment boundaries after a later explicit phase defines audio device
permissions, user-controlled path selection, local binary/model compatibility, cancellation,
playback/capture lifecycle, cleanup, and runtime safety checks.

Phase 12 checkpoint:

- `docs/PHASE_12_CHECKPOINT.md` records the completed voice scope, current architecture, known
  limitations, safety guardrails, future Piper integration plan, future Whisper integration plan,
  and Phase 13 readiness criteria.
- Architecture review found the voice provider boundaries, session/pipeline metadata, static
  coordinator, controller ownership, and view-model exposure remain separated and metadata-only.
- Existing coverage verifies deterministic voice pipeline behavior, null TTS/STT refusal,
  blocked/error metadata paths, disabled runtime posture, and QML-safe exposure.
- Phase 13 should not add executable voice behavior until permissions, local binary/model
  ownership, lifecycle, cancellation, error handling, and safety gates are explicitly scoped.

Phase 13 checkpoint:

- `docs/PHASE_13_CHECKPOINT.md` records the completed Phase 13 voice/Piper scope, current Piper
  architecture, current TTS path, safety boundary findings, remaining limitations, future
  next-step phases, and Phase 14 readiness criteria.
- Architecture review found Piper remains behind `ITextToSpeechProvider`, file-output execution
  remains behind `IPiperTtsClient` and explicit provider gates, and QML exposure remains
  read-only/QML-safe.
- Existing coverage verifies null provider refusal, disabled/default posture, missing binary/model
  refusal, safety blocking, invalid request refusal, controlled fake file-output success, failure,
  timeout metadata, and controller/view-model exposure.
- Phase 14 may begin with planning or configuration-readiness work, but playback, microphone
  capture, Whisper execution, downloads, cloud/API-key behavior, and autonomous voice loops still
  require explicit later phase scope.

Phase 14 configuration:

- `AppSettings` persists the four local voice path strings separately from memory and chat history.
- `ApplicationController` derives voice binary/model descriptors and Piper readiness from those
  strings by inspecting only the exact configured paths.
- `ApplicationController` also derives compact status badges and hint summaries without executing
  binaries or scanning broadly.
- `DesktopShellViewModel` forwards the persisted path values, readiness summaries, status badges,
  and hint summaries to Settings as QML-safe strings and string lists.
- Configuration does not run Piper, run Whisper, access microphones, play audio, download models,
  scan directories recursively, apply hints automatically, add API keys, or start autonomous
  behavior.

Phase 15.1-15.3 voice refinement:

- Settings uses shorter Voice Configuration labels, clear help text, compact status badges, exact
  validation rows, and an Apply Paths action.
- Piper file-output TTS readiness is now shown as Ready, Blocked, or Missing with exact blocked
  reasons, but Piper execution remains disabled unless a later explicit phase enables it through
  the existing provider/client safety gates.
- Whisper remains configuration/readiness only and reports whether STT can be prepared later.

Phase 15.4-15.6 controlled Piper execution:

- Historical note: Phase 15.4-15.6 previously described a controlled Piper file-output path.
  Phase 18.22-18.24 supersedes that active behavior. Current desktop builds expose Piper
  readiness/synthesis metadata only; the opt-in/generation path is disabled and refuses without
  subprocess execution, file output, or playback.
- Future controlled synthesis and future playback/audio-device support require separate explicit
  phases.

Current local AI user flow:

- The user may view local Ollama health and discovered model metadata through loopback-only
  runtime checks.
- Settings can persist a selected local model from discovered names. If no model is selected,
  discovery metadata may supply a safe fallback candidate; if discovery reports the selected model
  missing, local inference and streaming are refused before the client is called.
- The user must explicitly enable local chat inference before chat prompts can route to Ollama.
- The user must explicitly enable streaming before chat uses the streaming boundary; otherwise
  guarded non-streaming local inference is used when local chat inference is enabled.
- Runtime status, selected/effective model status, streaming state, last-response summary, latency,
  and traces are exposed to QML as strings, string lists, booleans, and counts only.

Still out of scope:

- Model downloads, pulls, deletes, installs, broad model-management actions, cloud provider calls,
  API keys,
  endpoint expansion, subprocess launch, filesystem/system actions, tools/plugins, autonomous
  loops, and automatic routing without opt-in.

Phase 11 checkpoint:

- `docs/PHASE_11_CHECKPOINT.md` records the completed local AI scope, current user flow, known
  limitations, safety guardrails, and Phase 12 readiness criteria.
- Runtime QA coverage includes disabled local inference, missing and invalid models,
  permission/safety blocking, disabled and enabled streaming, stream-error preview cleanup, fake
  non-streaming success, fake streaming success, stable settings persistence, duplicate-message
  protection through final chat history, and QML-safe exposure checks.
- The checkpoint preserves the existing boundaries: no model management actions, cloud providers,
  API keys, filesystem/system actions, subprocess launch, tools/plugins, autonomous loops, or UI
  redesign.

## Local Model Management Readiness

Phase 11.0 through Phase 11.2 add `IModelManagementService` as a metadata/readiness boundary for
future local model management. The current implementation is `StaticModelManagementService`.

Separation:

- `IModelManagementService` is not `IOllamaRuntimeClient`; it does not discover installed models.
- `IModelManagementService` is not `ILocalInferenceClient`; it does not execute prompts.
- `IModelManagementService` is not process, filesystem, provider, cloud, or credential ownership.

The service returns deterministic recommendations and approximate descriptive RAM/disk requirement
summaries. Pull, delete, and install requests return unavailable/not implemented results.
`ApplicationController` combines this metadata with existing installed-model count and selected
model state, and `DesktopShellViewModel` exposes QML-safe strings/lists only.

This boundary does not download, pull, delete, install, scan filesystems, launch subprocesses,
call cloud services, read API keys, execute tools/plugins, or run autonomous loops.

## Local Runtime Session Metadata

Phase 7.1 adds local runtime session ownership metadata. `LocalRuntimeSession` records a
deterministic placeholder session id, lifecycle status, health, allocation metadata, reservation
metadata, revision, and summary.

Separation:

- Local runtime sessions are not `ChatSession` transcript sessions.
- Local runtime sessions are not Phase 4 `RuntimeSession` agent pipeline context.
- Local runtime sessions are not provider/model execution.
- Local runtime sessions are not tool execution or plugin ownership.

`NullLocalRuntimeSessionManager` exposes a single deterministic placeholder session in `Reserved`
and `Placeholder Only` state. Allocation and reservation records are descriptive metadata only and
do not allocate models, launch processes/subprocesses, scan filesystems, call networks, stream
output, execute tools, load plugins, or start background workers.

## Runtime Capability Negotiation Metadata

Phase 7.2 adds `IRuntimeCapabilityRegistry` as the future runtime capability negotiation metadata
boundary. The current implementation is `StaticRuntimeCapabilityRegistry`, which returns
deterministic descriptors and a metadata-only negotiation result.

Separation:

- Runtime capability negotiation is not runtime execution.
- Runtime capability negotiation is not provider/model routing.
- Runtime capability negotiation is not agent execution.
- Runtime capability negotiation is not tool execution.
- Runtime capability negotiation is not permission approval.
- Runtime capability negotiation is not plugin management.

`RuntimeCapabilityDescriptor` records descriptive capability id, name, group, state, and summary
metadata. `RuntimeNegotiationProfile` and `RuntimeNegotiationResult` summarize the static
negotiation posture. Local-only enforcement and privacy-safe mode are enabled as safety metadata
only. Local inference, streaming, multimodal, embeddings, semantic memory, memory binding, tool
bridge, plugin bridge, filesystem access, external process execution, and cloud relay support
remain disabled or unavailable and cannot execute.

The capability negotiation layer does not activate capabilities, call providers, execute models,
download models, stream output, launch processes/subprocesses, scan filesystems, execute tools,
load plugins, access networks, approve permissions, read API keys, or start background workers.

## Runtime Permission Metadata

Phase 7.3 adds `IRuntimePermissionPolicy` and `StaticRuntimePermissionPolicy` as a deterministic
runtime permission metadata boundary.

Separation:

- Runtime permission metadata is not runtime execution.
- Runtime permission metadata is not provider/model/tool/plugin invocation.
- Runtime permission metadata is not filesystem/network/process access.

`RuntimePermission`, `RuntimePermissionLevel`, `RuntimePermissionRequest`, and
`RuntimePermissionDecision` describe future permission posture only. Default static policy behavior
is deny-by-default for execution-level requests in metadata-only mode.

## Runtime Request Pipeline Metadata

Phase 7.4 adds `IRuntimePipeline` and `StaticRuntimePipeline` as a metadata-only request pipeline
boundary.

`RuntimePipelineRequest`, `RuntimePipelineStage`, `RuntimePipelineStatus`, `RuntimePipelineTrace`,
and `RuntimePipelineResult` provide deterministic ordered trace/status summaries only.

Separation:

- Runtime request pipeline metadata is not provider runtime.
- Runtime request pipeline metadata is not model execution.
- Runtime request pipeline metadata is not process/tool/plugin execution.

Execution remains blocked at the metadata boundary even when request metadata passes earlier
stages.

## Runtime Safety Policy Metadata

Phase 7.5 adds `IRuntimeSafetyPolicy` and `StaticRuntimeSafetyPolicy` as deterministic safety
posture metadata boundaries.

`RuntimeSafetyPolicy`, `RuntimeSafetyRule`, `RuntimeSafetyDecision`, and `RuntimeSafetyReport`
describe local-only and no-execution safety posture with deterministic rules.

Separation:

- Runtime safety policy metadata is not sandbox runtime enforcement.
- Runtime safety policy metadata is not provider/model/tool execution.
- Runtime safety policy metadata is not filesystem/network/process access.

## Phase 7 Checkpoint

Phase 7.6 closes the metadata-only local runtime foundation before Phase 8. The checkpoint is
documented in `docs/PHASE_7_CHECKPOINT.md`.

Architecture findings:

- Local runtime, session ownership, capability negotiation, permission policy, safety policy, and
  request pipeline remain separate metadata responsibilities.
- `ApplicationController` owns Phase 7 runtime boundaries through explicit interfaces and exposes
  deterministic summary/status values.
- `DesktopShellViewModel` remains the QML boundary and exposes only strings, counts, and string
  lists for runtime metadata.
- The Settings page remains read-only for runtime metadata and has no setup, approval, execution,
  provider, download, tool, plugin, filesystem, process, or network controls.

The checkpoint adds no provider integration, model execution, networking, API keys, downloads,
streaming, subprocess/process launch, filesystem/system actions, tool execution, plugin loading,
embeddings, semantic search, or autonomous workers.

## Execution Lifecycle And Session Coordination

Phase 8.0 through Phase 8.2 add a metadata-only execution lifecycle/session coordination layer for
future provider/runtime integration.

Separation:

- `IExecutionLifecycle` is not `ILocalRuntime`; it does not own local inference.
- `IExecutionLifecycle` is not `IChatProvider`; it does not generate chat responses.
- `IExecutionLifecycle` is not `IAgentRuntime`; it does not run agents or workers.
- `IExecutionLifecycle` is not `IToolExecutor`; it does not execute tools.
- `ExecutionCoordinator` owns read-only lifecycle/session coordination snapshots only.

`ExecutionRequest`, intent, priority, lifecycle state/status/result, ordered trace, session,
ownership, and coordination mode values are deterministic metadata. `StaticExecutionLifecycle`
records requested, validating, permission-check, safety-check, coordination, ready-placeholder, and
blocked states while always returning non-executable blocked results. Invalid transitions are
rejected safely and do not advance state.

`ExecutionSession` and `ExecutionCoordinationSnapshot` describe ownership and coordination posture
only. `ApplicationController` owns the lifecycle and coordinator interfaces, and
`DesktopShellViewModel` exposes only strings and string lists. Dashboard and Settings visibility is
read-only and has no execution controls.

The layer does not call providers or models, launch Ollama, start subprocesses/processes, access
the filesystem/system, access networks/API keys, download models, stream output, execute
tools/plugins, start autonomous workers, or run timers/background loops.

Future Phase 9 provider/runtime integration must remain behind explicit provider/runtime
interfaces and policy gates. Phase 8 lifecycle metadata is a coordination read model, not execution
authority.

## Local Runtime Adapter And Provider Bridge Readiness

Phase 8.3 through Phase 8.5 add metadata-only adapter, provider bridge, and pre-integration
readiness boundaries for future Ollama/local runtime integration.

Separation:

- `ILocalRuntimeAdapter` is not execution and does not connect to or launch a local runtime.
- `IProviderRuntimeBridge` is not an `IChatProvider` implementation and does not generate chat
  responses.
- `StaticRuntimeIntegrationReadiness` is not probing and does not inspect the system.
- Execution lifecycle remains blocked.
- `ILocalRuntime` remains placeholder-only.

`LocalRuntimeAdapterDescriptor` and capability summaries describe the future adapter contract only.
The static adapter reports placeholder metadata, no endpoint configuration, disabled model
discovery, and no inference execution.

`IProviderRuntimeBridge` describes the future boundary between provider routing and runtime
adapters. The current static bridge reports not connected and not executable for every request.

`RuntimeIntegrationReport` records ordered readiness checks for adapter contract, endpoint
configuration, model discovery, provider bridge connection, and execution permission. Failed checks
describe what must exist before future Ollama/local runtime integration can be considered.

`ApplicationController` owns adapter/bridge/readiness objects and `DesktopShellViewModel` exposes
only strings and string lists. Dashboard and Settings show read-only metadata and provide no setup
buttons, endpoint fields, model selection UI, or execution controls.

This layer does not call Ollama, OpenAI, Anthropic, or any provider; does not use API keys; does
not access networks; does not download models; does not stream; does not launch
processes/subprocesses; does not scan or mutate filesystems/systems; does not discover models; does
not execute tools/plugins; does not create embeddings/vector databases; and does not start
autonomous workers.

## Ollama Local Health And Discovery Boundary

Phase 9.0 through Phase 9.2 add the first controlled Ollama-local integration boundary. This is
limited to endpoint configuration metadata, loopback-only health checks, and optional installed
model metadata discovery.

Separation:

- `IOllamaRuntimeClient` is not `IChatProvider` and cannot generate chat responses.
- `IOllamaRuntimeClient` is not `IProviderRuntimeBridge`, `IExecutionLifecycle`,
  `IAgentRuntime`, `IToolExecutor`, or model router execution.
- Chat requests are not routed to Ollama.
- Execution lifecycle remains blocked.

`OllamaEndpoint` normalizes endpoint input. Only loopback HTTP endpoints are valid; invalid,
non-loopback, cloud, query, fragment, and non-HTTP values fall back to
`http://127.0.0.1:11434`. `AppSettings` may persist only the normalized endpoint and does not
store API keys or credentials.

`NullOllamaRuntimeClient` provides deterministic unavailable behavior for tests and default safe
ownership. `OllamaHttpRuntimeClient` is injectable and restricted to read-only local Ollama API
paths: `/api/version` for health and `/api/tags` for installed model metadata. It does not send
prompts, start generation, stream tokens, pull/download/delete/run models, launch subprocesses,
scan the filesystem/system, call cloud endpoints, read API keys, execute tools/plugins, or start
background workers.

`ApplicationController` exposes only endpoint, connection status, health status, health summary,
model count, and model summary strings. `DesktopShellViewModel` forwards those as QML-safe strings,
counts, and lists. Dashboard and Settings show read-only local status only.

## Controlled Local Inference Boundary

Phase 9.3 through Phase 9.5 add the first explicit local-only inference boundary for Ollama.

Separation:

- `ILocalInferenceClient` is the only boundary that may submit a prompt to a local runtime.
- `ILocalInferenceClient` is not `IChatProvider`, `IAgentRuntime`, `IToolExecutor`,
  `IProviderRuntimeBridge`, or model-router execution.
- `ApplicationController` must evaluate runtime permission and safety policy before calling the
  local inference client.
- `DesktopShellViewModel` exposes only status, summary, last-response summary, and trace strings.

`LocalInferenceRequest`, `LocalInferenceOptions`, `LocalInferenceResponse`,
`LocalInferenceStatus`, `LocalInferenceError`, and `LocalInferenceTrace` describe the controlled
request/response contract. `NullLocalInferenceClient` refuses deterministically.
`OllamaLocalInferenceClient` is local-only and may call only loopback HTTP Ollama endpoints. It
rejects blank prompts, missing models, unavailable models, invalid endpoints, cancellation
metadata, and streaming requests before generation. When allowed by controller policy checks, it
uses non-streaming `/api/generate` with `stream: false`.

The boundary does not call cloud endpoints, use API keys, follow redirects, pull/download/delete
models, launch subprocesses, execute tools/plugins, access the filesystem/system, stream tokens, or
start autonomous loops/background workers. Tests use injected fake clients and do not require
Ollama to be running.

## Settings Contract

`ISettingsStore` is the persistence boundary for app settings. `AppSettings` owns defaults and validation. `InMemorySettingsStore` remains the default test backend, while `JsonSettingsStore` provides a lightweight desktop persistence backend.

The desktop app stores settings below Qt's `AppConfigLocation`. Future settings backends should implement `ISettingsStore` without changing QML or the desktop view model.

## Plugin And Integration Boundaries

`IPlugin` describes future in-process plugin lifecycle boundaries. `IIntegration` describes future external or local integration metadata. Neither interface loads code, performs network calls, or talks to operating-system services in the alpha.

## Provider Contract

`IChatProvider` is the local chat pipeline boundary. Providers expose a name, a status, and a deterministic `sendMessage` result. `ApplicationController` owns blank-message validation, unavailable-provider handling, and chat transcript formatting.

`LocalEchoProvider` is the only provider in the alpha. It performs no network calls, reads no API keys, and returns a stable local response for UI and tests.

Future real providers should implement `IChatProvider` behind explicit configuration and status handling. Network transport, credentials, retries, streaming, and model selection are intentionally not part of Phase 2.2.

## Agent Runtime Contract

`IAgentRuntime` is a separate boundary from `IChatProvider`.

- `IChatProvider` remains responsible for chat response generation.
- `IAgentRuntime` is responsible for future orchestration/action runtime behavior.

Phase 4.0 adds `NullAgentRuntime` as a deterministic local-only skeleton:

- no networking
- no tool execution
- no system/file-modifying actions

Phase 4.2 adds metadata-only tool invocation planning:

- `ToolInvocationPlan` describes proposed tool-use intent.
- `PlannedToolInvocation` contains selected tool id, summary/rationale, arguments, and copied
  risk/execution metadata.
- `IAgentRuntime::plan` returns inspectable planning data only.
- `NullAgentRuntime` generates deterministic fake plans from registered tool descriptors.
- Plans do not execute, mutate files, launch processes, call networks, or approve permissions.

Phase 4.3 adds approval and permission metadata:

- `IApprovalPolicy` evaluates planned invocations without executing them.
- `StaticApprovalPolicy` provides deterministic local approval metadata.
- `ApprovalDecision` records whether approval is not required, required, approved, or denied.
- `ToolApprovalRequest` and `PermissionDescriptor` are descriptive metadata only.
- Approval does not grant runtime capabilities, run tools, or activate a sandbox.

Phase 4.4 adds sandbox and capability metadata:

- `ISandboxPolicy` evaluates planned invocation metadata and approval metadata.
- `StaticSandboxPolicy` provides deterministic local capability-boundary metadata.
- `CapabilityDescriptor` labels future runtime capability requirements.
- `SandboxEvaluationResult` records whether planned capabilities are allowed, denied, blocked by
  approval, or not evaluated.
- Sandbox evaluation does not grant OS permissions, execute tools, request privileges, or enforce a
  real sandbox.
- Approval can be required for sandbox evaluation to proceed, but approval does not override a
  capability denial.

Phase 4.5 adds a placeholder execution boundary:

- `IToolExecutor` owns the future tool execution interface.
- `NullToolExecutor` returns deterministic placeholder results only.
- `ToolExecutionRequest` carries the plan, approval decision, sandbox evaluation, and known tool
  ids as value data.
- `ToolExecutionResult` records placeholder success, blocked, empty-plan, unknown-tool, or
  not-requested status.
- The boundary performs no real tool execution, shell/process launch, subprocess execution,
  filesystem mutation, networking, plugin loading, OS automation, privilege escalation, or sandbox
  enforcement.

Phase 4.6 stabilizes the controller-level pipeline result:

- `AgentPipelineResult` aggregates the value-only outputs from planning, approval, sandbox
  metadata evaluation, and placeholder execution.
- The full Phase 4 route is:
  registry -> planning -> approval -> sandbox capability metadata -> placeholder execution boundary.
- The result exposes deterministic generic status and summary text for controller/view-model use.
- The result does not expose mutable runtime internals and does not add execution controls.
- Execution remains placeholder-only.

Phase 4.7 adds runtime context/session ownership:

- `RuntimeSession` owns one local `AgentRuntimeContext`.
- `AgentRuntimeContext` records the latest `AgentPipelineResult`, active planned tool ids,
  approval metadata, sandbox metadata, and placeholder execution metadata through value copies.
- Runtime context ids and revisions are deterministic local metadata.
- Runtime context is not persistence, planning, approval policy, sandbox enforcement, plugin
  loading, or execution.
- Runtime context does not launch processes, mutate files, call networks, load plugins, request
  privileges, or enforce a real sandbox.

Phase 4.8 adds in-memory agent activity/audit metadata:

- `AgentActivityEntry` records deterministic sequence id, activity type, status, and generic
  summary.
- `AgentActivityLog` owns the in-memory activity list and deterministic ordering.
- `ApplicationController` appends activity events for request receipt, planning, approval,
  sandbox evaluation, placeholder execution evaluation, and final pipeline outcome.
- The activity layer stores metadata-only summaries and does not persist, export, record secrets,
  record raw system paths, execute tools, launch processes, call networks, load plugins, or enforce
  sandbox behavior.
- Future persistence/export/security work should sit behind an explicit audit/logging boundary and
  is not implemented in the desktop alpha.

Phase 4.9 adds read-only desktop UI visibility for the metadata-only pipeline:

- Dashboard displays latest pipeline status/summary, runtime context status/summary, active planned
  tool ids, activity count, and latest activity summary.
- The UI reads QML-safe `DesktopShellViewModel` properties only.
- No execution buttons, approval controls, raw runtime objects, paths, secrets, provider
  integrations, plugin loading, persistence, shell/process launch, filesystem mutation, networking,
  OS automation, or real sandbox runtime are added.

Phase 4.10 closes Phase 4 with an architecture checkpoint:

- Provider/agent separation and registry/planning/approval/sandbox/execution boundaries remain
  intact.
- Runtime context and activity logging remain in-memory metadata ownership only.
- Dashboard visibility remains read-only and QML-safe.
- Phase 5 readiness is documented in `docs/PHASE_4_CHECKPOINT.md`.

Phase 4.11 adds AI orchestration planning only:

- Future `ModelRouter` and `RoutingPolicy` concepts are documented in
  `docs/AI_ORCHESTRATION_PLAN.md`.
- Future routing may use provider capability profiles, task classification, local/cloud fallback,
  device-aware model selection, user routing modes, and privacy-aware policy.
- This planning layer should remain separate from `IChatProvider`, `IAgentRuntime`, tool execution,
  and UI model-management screens.
- No provider integration, networking/API keys, model download, model execution, plugin loading,
  filesystem/system action, real tool execution, or runtime behavior change is added.

Phase 6.0 adds a metadata-only model routing skeleton:

- `ModelRouting.h` defines `ModelDescriptor`, `ProviderDescriptor`,
  `ProviderCapabilityProfile`, `RoutingMode`, `TaskType`, task classification, and route result
  value data.
- `IModelRouter` is the future model/provider selection boundary.
- `StaticModelRouter` returns a deterministic local-only placeholder route.
- `ApplicationController` owns the router and exposes only routing mode, routing status, and a
  selected model/provider summary.
- `DesktopShellViewModel` forwards those values as QML-safe read-only strings.
- The router does not call providers, execute models, download models, use API keys, access the
  network, execute tools, load plugins, or perform filesystem/system actions.

Phase 6.1 persists routing mode preference:

- `AppSettings` owns the routing mode preference and defaults it to `Local Only`.
- `JsonSettingsStore` persists the setting in the existing settings JSON file.
- `ApplicationController` updates `IModelRouter` routing mode metadata and emits route-summary
  changes.
- `DesktopShellViewModel` synchronizes settings and controller state for QML.
- Settings UI exposes only a routing mode selector and read-only route metadata.
- No provider setup, credentials, API keys, networking, model downloads, model execution, or tool
  execution are introduced.

Phase 6.2 adds provider catalog metadata:

- `ProviderCatalog.h` defines value-only provider/model catalog entries with availability,
  local/cloud kind, supported task metadata, privacy level, and rough RAM/disk hints.
- `IProviderCatalog` is the read-only catalog boundary.
- `StaticProviderCatalog` provides deterministic placeholder metadata for Local Metadata Provider,
  Sentinel Local Placeholder, Ollama Local, OpenAI Cloud, and Anthropic Cloud.
- Cloud placeholders are marked not configured and are not exposed to the router as available
  models.
- `StaticModelRouter` seeds default routing from available catalog metadata only.
- Controller and desktop view-model expose only read-only catalog counts and summary strings.
- Settings UI shows text-only catalog metadata without setup buttons, API key fields, downloads,
  networking, or execution controls.

Phase 6.7 adds orchestration diagnostics/readiness metadata:

- `OrchestrationDiagnostics.h` defines diagnostic levels, diagnostic entries, readiness checks, and
  readiness reports as value data.
- `StaticOrchestrationDiagnostics` generates deterministic ordered diagnostics from
  `OrchestrationSnapshot` and provider catalog metadata.
- Checks cover routing mode, selected provider/model route, provider catalog, agent registry,
  memory taxonomy, task planner, snapshot health, local-only privacy posture, cloud provider
  unavailability, and disabled execution capability.
- Controller and desktop view-model exposure is read-only and QML-safe: status, summary, and
  `QStringList` diagnostic lines.
- Diagnostics do not probe real providers/models, read API keys, call networks, scan filesystems,
  execute tools, load plugins, start workers, run external processes, build embeddings, or perform
  semantic/vector search.

Phase 6.8 adds conversation session/context metadata:

- `ConversationSession.h` defines value-only session id, status, interaction mode, attention state,
  context scope, and runtime context-window metadata.
- `ConversationSessionStore` owns one deterministic local conversation session.
- `ConversationSessionContextBuilder` builds a context-window summary from existing routing,
  preferred agent, memory affinity, and orchestration snapshot summaries.
- `ApplicationController` owns the conversation session separately from `ChatSession` and Phase 4
  `RuntimeSession`.
- `DesktopShellViewModel` exposes only QML-safe strings: session id/status, interaction mode,
  attention state, and context-window summary.
- The layer does not add provider/model execution, streaming, networking, filesystem/system
  actions, tool execution, plugin loading, embeddings, vector search, semantic search, or
  autonomous workers.

Phase 6.9 adds conversation state graph metadata:

- `ConversationStateGraph.h` defines value-only conversation states, transition requests, and
  transition results.
- `StaticConversationStateGraph` applies deterministic valid-transition rules and returns
  accepted/rejected metadata.
- Controller and desktop view-model exposure is read-only and QML-safe: current state, transition
  status, and transition summary strings.
- Dashboard and Settings show minimal text-only state metadata.
- State transitions do not execute providers, execute models, stream output, run tools, approve
  actions, load plugins, call networks, scan filesystems, perform semantic/vector search, or start
  autonomous workers.

Phase 6.3 adds capability graph and task planner metadata:

- `TaskPlanning.h` defines value-only task planning metadata: capability graph nodes, planned task
  steps, task plan status, and task plan summaries.
- `ITaskPlanner` owns the high-level planning boundary.
- `StaticTaskPlanner` evaluates task classification, routing mode, provider catalog availability,
  local/cloud suitability, privacy sensitivity, supported task metadata, and resource hints.
- Sensitive or private tasks require local metadata.
- Unknown tasks use a safe local metadata fallback.
- Cloud placeholders remain not configured; cloud-allowed planning falls back to local metadata
  when available or reports blocked metadata otherwise.
- `IModelRouter` still chooses model/provider route metadata; `ITaskPlanner` only describes
  high-level task plan metadata.
- `IAgentRuntime` remains the tool/action orchestration boundary, and `IChatProvider` remains the
  chat response generation boundary.
- Controller and desktop view-model expose only task plan status, summary, and step count.
- The task planner does not call providers, execute models, execute tools, access the network,
  download models, load plugins, or perform filesystem/system actions.

Phase 6.4 adds agent registry metadata:

- `AgentMetadata.h` defines value-only agent descriptors, roles, states, priorities, capability
  profiles, task affinities, and runtime snapshot metadata.
- `IAgentRegistry` is the read-only agent metadata boundary.
- `StaticAgentRegistry` returns deterministic static descriptors for Atlas, Orin, Vela, Kaze, Nyx,
  and Sol.
- `StaticTaskPlanner` may annotate task plans with preferred agent metadata, but it does not
  execute agents or start orchestration.
- `IAgentRuntime` remains the future runtime/action orchestration boundary.
- Controller and desktop view-model expose only registered agent count, active agent summary
  strings, and the current preferred agent summary.
- Dashboard and Settings show text-only agent metadata without execution controls.
- The agent registry does not run autonomous loops, create threads/background workers, call
  providers/models, execute tools, access memory stores, load plugins, access the network, or
  perform filesystem/system actions.

Phase 6.5 adds memory taxonomy metadata:

- `MemoryMetadata.h` defines memory type, shard status, retention policy, privacy level, recall
  hint, affinity, and association value metadata.
- `IMemoryCatalog` is separate from `IMemoryStore`; it describes future semantic memory categories
  while `IMemoryStore` remains the explicit key-value persistence contract.
- `StaticMemoryCatalog` exposes deterministic Episodic, Semantic, Procedural, Reflective, and
  Ambient metadata.
- `StaticTaskPlanner` may attach preferred memory affinity labels to task plans.
- Controller and desktop view model expose only category counts, text summaries, and current memory
  affinity summary.
- No vector database, embeddings, semantic search, provider/model execution, autonomous memory
  writes, tool execution, networking, plugin loading, or filesystem/system action is added.

Phase 6.6 adds an orchestration snapshot read model:

- `OrchestrationSnapshot.h` defines value-only orchestration health, signal, workspace state, and
  snapshot metadata.
- `ApplicationController` builds the current snapshot deterministically from existing routing,
  provider catalog, task plan, preferred agent, memory affinity, runtime context, and activity
  metadata.
- `DesktopShellViewModel` exposes only snapshot status, summary, and signal strings.
- Dashboard shows the snapshot as read-only workspace intelligence metadata.
- The snapshot is a read model, not a scheduler or execution system. It does not create background
  refresh, threads, timers, provider/model calls, memory search, tool execution, networking,
  plugins, filesystem/system actions, embeddings, or vector search.

Phase 5.0 adds UI/UX planning and design-system foundation only:

- `docs/UI_UX_PLAN.md` records the design direction and motion constraints.
- `SentinelTheme.qml` provides QML presentation tokens.
- Existing QML components consume tokens where safe without changing view-model contracts or app
  behavior.
- Advanced motion, assistant visuals, provider/model management, networking, execution, plugin
  loading, sandbox runtime, and filesystem/system actions remain unimplemented.

Phase 5.1 adds lightweight motion and interaction foundation:

- `SentinelTheme.qml` includes motion duration and easing tokens.
- Dock navigation, command buttons, text-field focus rings, and page opacity transitions use
  subtle tokenized transitions.
- No heavy animation, particle systems, assistant-face rendering, custom OpenGL/Vulkan rendering,
  provider/model execution, networking, plugin loading, filesystem/system actions, or runtime
  behavior changes are added.

Phase 5.2 adds adaptive layout foundation:

- `SentinelTheme.qml` includes compact/wide breakpoint and responsive spacing helpers.
- `Main.qml` owns shell-level compact/wide state for presentation sizing only.
- Header, status, dashboard, chat, memory, settings, and dock layouts wrap or elide content at
  narrower widths while keeping view-model contracts unchanged.
- No provider/model execution, networking, plugin loading, filesystem/system actions, approval
  controls, sandbox behavior, assistant-face rendering, particle systems, or custom rendering
  systems are added.

Phase 5.3 adds component consistency and visual QA foundation:

- `SentinelTextField.qml` and `InfoRow.qml` normalize repeated presentation-only patterns.
- Button height, input height, card padding, section heading wrapping, dashboard status rows, and
  settings status rows use shared component/token patterns.
- `docs/UI_QA_CHECKLIST.md` records manual compact/normal/wide and platform visual checks.
- No runtime behavior, provider/model execution, networking, plugin loading, filesystem/system
  actions, approval controls, assistant-face rendering, particle systems, or custom rendering
  systems are added.

Phase 5.4 adds workspace UX integration from the former `lovable-tasarim` design reference:

- The React/TypeScript/Tailwind reference remains outside the production architecture.
- Qt/QML owns the translated central presence workspace, right chat panel, ambient
  background, glass panel treatment, and lightweight motion.
- Mode-aware visual helpers live in `SentinelTheme.qml` and are presentation-only.
- QML continues to bind to `DesktopShellViewModel`; no raw core objects, provider changes, tool
  execution paths, WebView, Node runtime, or web framework dependencies are introduced.

Phase 5.4.5 audits the completed workspace integration before Phase 5.5:

- Core, desktop view-model, persistence, provider/agent, planning, approval, sandbox, placeholder
  execution, runtime context, and activity-log boundaries remain unchanged.
- Small QML cleanup may reduce duplicated presentation styling or fragile bindings, but must not
  redesign the shell or add product behavior.
- Known UI risks are token growth, mode visual helpers becoming logic-heavy, compact layout
  regressions, unavailable `qmllint` tooling, and manual-only visual QA coverage.

`ApplicationController` and `DesktopShellViewModel` expose only generic agent status, placeholder
response text, latest plan status/summary, latest approval status/summary, latest sandbox
status/summary, latest placeholder execution status/summary, and latest aggregate pipeline
status/summary, generic runtime context status/summary and active planned tool ids, plus activity
count/latest activity summary to QML.

## Chat Session Pipeline

Chat history is owned by `ChatSession`, an in-memory session model with structured `ChatMessage` entries:

- message id
- role: `system`, `user`, or `assistant`
- content
- timestamp from an injectable clock
- status: `sent`, `received`, or `error`

`ApplicationController::sendMessage` validates input, appends a user message, calls `IChatProvider`, then appends an assistant reply or error message. Blank messages do not mutate history.

The desktop layer exposes history through `ChatMessageListModel`, a QML-safe `QAbstractListModel`. QML reads roles and never receives raw core objects.

Chat history can be persisted through `IChatHistoryStore`. QML still reads `ChatMessageListModel` and does not write persistence data directly.

The current UX treats chat history as one local transcript. The clear action resets the runtime transcript and clears persisted chat history after confirmation in QML.

## Not Implemented Yet

- Real AI provider inference.
- Real tool execution.
- Shell/process launching.
- Subprocess execution.
- Filesystem mutation.
- Voice input or output.
- Automation agents.
- Cloud sync.
- Runtime plugin loading.
- Real sandbox runtime.
- Wearable or edge-device support.
