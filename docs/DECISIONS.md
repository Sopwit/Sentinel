# Architecture Decisions

## 1. Cross-platform Qt Desktop

Decision: Sentinel uses C++20, Qt 6, and QML for a cross-platform desktop application.

Reason: The product is desktop-first and should remain portable while allowing a Linux/Fedora KDE Plasma optimized experience.

Avoided:

- Electron.
- Browser-first desktop shell.
- Python backend for the app core.
- Linux-only core assumptions.

## 1.1 Platform Abstraction Direction

Decision: Keep platform-specific behavior behind explicit service interfaces.

Reason: Linux integrations may be richer, but the core architecture must remain portable to Windows and macOS.

Planned interfaces:

- `IPlatformService`
- `IPathProvider`
- `INotificationService`
- `ISystemIntegrationService`

Rule: platform services should not leak into storage contracts, provider contracts, controllers, or QML pages.

Phase 3.4 implementation status:

- Added `IPathProvider`.
- Added `IPlatformService`.
- Added `INotificationService` as a lightweight placeholder.
- Added `ISystemIntegrationService` as a lightweight placeholder.
- Kept these as boundaries only; no OS-specific integration/automation implementation yet.

## 1.2 Companion/Menu Bar/System Tray Boundary

Decision: Companion/menu-bar/system-tray behavior is represented by a platform-neutral,
value-only `CompanionService` boundary plus a desktop-owned Qt native adapter.

Reason: Sentinel needs a future lightweight desktop entry point, but shell integration must not
become hidden background autonomy or an execution side channel.

Runtime behavior:

- The user preference "Show Sentinel in menu bar / system tray" controls native companion
  visibility when Qt reports tray/status item support.
- macOS uses Qt tray/status item behavior first, Windows uses Qt system tray behavior, and Linux
  uses Qt tray/status notifier behavior where available.
- If native tray/status item support is unavailable, the companion degrades to unavailable
  metadata and does not create an alternate daemon.
- Native actions are foreground-safe shell actions: Open Sentinel, New Conversation, Settings,
  Pause Companion, and Quit. Quick Note / Capture remains disabled and metadata-only.
- Pause Companion changes only companion presentation/readiness metadata, not runtime/model
  behavior.
- QML receives only safe booleans, strings, and string lists through `DesktopShellViewModel`.

Out of scope:

- Startup agents, hidden background workers, timers, autonomous loops, provider/model calls,
  cloud/API calls, filesystem scanning, microphone capture, playback, STT/TTS activation,
  tools/plugins, subprocess execution, hidden indexing, and OS-specific handle exposure to QML.

## 1.3 Workspace Readiness And Permission Posture

Decision: Workspace access is represented as readiness metadata with explicit permission posture
labels before any filesystem authority exists.

Reason: Users need a clear workspace surface, but selecting or viewing workspace readiness must
not silently grant directory access, scan files, index content, or feed workspace data into prompts.

Runtime behavior:

- The current workspace is a local metadata placeholder.
- The current permission posture is Disabled.
- Future posture labels are Ask Every Time, Trusted, and Enabled, but they are inert in this
  build.
- Choose Workspace and Clear Workspace are disabled placeholders until native picker, revocation,
  and core policy behavior are explicitly implemented.
- QML receives only QML-safe strings and string lists through `DesktopShellViewModel`.

Out of scope:

- Native file pickers, path grants, filesystem reads, recursive scans, indexing, embeddings,
  provider/model calls, cloud/API calls, subprocesses, tools/plugins, autonomous agents,
  background workers, hidden memory writes, and workspace-derived prompt context.

## 1.4 Skill/Profile Metadata Boundary

Decision: Assistant profiles are represented by a value-only `SkillProfileService` registry and a
persisted selected profile id.

Reason: Users need presentation-level profile choices before Sentinel has policy-enforced skills,
plugins, tools, workspace access, or automation.

Runtime behavior:

- The default selected profile is Developer.
- Available profiles are Developer, Student, Researcher, Personal Assistant, and Custom.
- `AppSettings` persists only `selectedSkillProfile`.
- `DesktopShellViewModel` exposes only safe strings and string lists for selected profile
  metadata, summaries, readiness checks, capability labels, and developer diagnostics.
- Profiles shape presentation and future policy metadata only.

Out of scope:

- Prompt mutation, hidden system prompt changes, custom instruction loading, tool execution,
  agent activation, workspace/filesystem authority, cloud/API calls, STT/TTS activation,
  subprocess launch, background workers, autonomous behavior, and runtime permission changes.

## 2. Modular Monolith

Decision: Keep the repository as a modular monolith with clear internal boundaries.

Reason: The project is still in alpha. Interfaces give enough separation without the operational cost of separate services or packages.

## 3. C++ Core Owns Business Logic

Decision: Core behavior belongs in C++ classes and interfaces. QML owns presentation and user interaction.

Reason: This keeps behavior testable, deterministic, and independent from UI layout.

## 4. QML Boundary Through View Models

Decision: QML interacts with `DesktopShellViewModel` and QML-safe models.

Reason: Raw core objects should not leak into the UI layer.

## 5. Provider Isolation

Decision: Chat providers are hidden behind `IChatProvider`.

Reason: Real providers, local providers, status handling, and future configuration can evolve without changing UI code.

Provider posture decision:

- The current product UI must state that Local Ollama is the only active execution path and no
  cloud provider is active. Future-facing provider labels should use generic selected local
  provider wording where copy is not specifically describing the active Ollama execution path.
- Future external API/cloud providers must be explicit opt-in integrations with separate provider
  boundaries, settings, safety review, tests, and user-facing activation. They must not appear as
  implied active configuration in normal UI.

## 5.1 Runtime Provider Abstraction

Decision: Runtime/provider readiness is exposed through a value-only runtime provider registry.

Reason: Sentinel needs multi-runtime local-first orchestration metadata without granting new
provider authority or coupling QML to runtime clients.

Runtime behavior:

- Local Ollama is the only enabled provider/runtime path.
- OpenAI-compatible/API providers are disabled placeholders only.
- Selected provider metadata is persisted, but disabled providers cannot become active execution
  paths; active provider metadata falls back to Local Ollama.
- Capability metadata is deterministic and does not activate capabilities.
- Readiness checks remain explicit/manual through existing loopback-only Ollama health/model
  metadata. There are no background probes or discovery daemons.

Out of scope:

- Cloud execution, API-key storage, automatic fallback routing, background model pulls,
  provider auto-discovery, filesystem scanning, embeddings/vector search, tool execution, agent
  planning, hidden retries, and automatic provider switching.

## 5.2 Model Registry Boundary

Decision: Model metadata is represented by a deterministic value-only `ModelRegistry`.

Reason: Sentinel needs to display, validate, and persist model choices across providers without
granting model management or execution authority.

Runtime behavior:

- Ollama model entries are built only from existing local Ollama discovery metadata.
- Unknown RAM, context, and model-specific capabilities remain unknown.
- Selected model values are persisted per provider; Ollama keeps the existing selected local model
  setting for compatibility.
- Disabled/future providers may expose placeholder model metadata only.
- Send validation refuses before transcript mutation unless the selected provider is Local
  Ollama, the selected local model is present in discovered metadata, Ollama is reachable through
  loopback HTTP, the active conversation is unarchived, and no request is active.

Out of scope:

- Pulling, installing, deleting, refreshing, importing, exporting, filesystem scanning, model file
  inspection, cloud/API capability lookup, automatic fallback routing, and hidden model execution.

## 5.3 Provider Credential Boundary

Decision: Provider credentials are represented by value-only readiness metadata until a later
explicit credential phase.

Reason: Sentinel needs UI and settings posture for future OpenAI-compatible, Claude, and Gemini
providers without creating secret-handling, cloud-network, or routing authority prematurely.

Runtime behavior:

- Local Ollama requires no API key and remains the only active provider path.
- OpenAI-compatible, Claude, and Gemini providers are disabled placeholder-ready metadata only.
- Selected provider metadata may persist, but disabled cloud providers cannot become active
  execution paths; active provider metadata falls back to Local Ollama.
- API key values are not accepted, stored, logged, or exposed. Settings and QML receive only
  configured/not-required, missing, and refused metadata.

Out of scope:

- API-key entry, secret storage, keychain integration, cloud calls, connect/test-call buttons,
  provider fallback routing, hidden retries, background provider discovery, automatic provider
  switching, and remote model discovery.

## 5.4 Secure Credential Store Boundary

Decision: Secure credential infrastructure is represented by a value-only `CredentialStore`
boundary until a later explicit storage phase.

Reason: Sentinel needs to prepare for future API-key storage without adding plaintext secret
persistence, cloud execution, provider testing, or platform-specific secret-store calls
prematurely.

Runtime behavior:

- The preferred future backend is OS-native: macOS Keychain, Windows Credential Manager, or Linux
  Secret Service.
- The current implementation is readiness metadata only; all storage, remove, and test actions
  return disabled, non-mutating results.
- Provider credential records may reference backend readiness, but they remain missing/not
  configured for cloud providers and cannot enable execution.
- QML receives only safe strings: store summary, backend summary, safety summary, provider
  summaries, disabled action readiness, execution disabled status, and bounded traces.

Out of scope:

- Real secret storage, plaintext API-key persistence, secret logging, raw secret exposure, key
  entry/removal/testing, OS keychain calls, provider calls, cloud routing, background probing,
  hidden retries, fallback execution, and autonomous behavior.

## 5.5 Secure Credential Backend Interface

Decision: Future credential storage is hidden behind `ICredentialBackend`.

Reason: Sentinel needs a testable provider-scoped secret-storage contract before real OS
keychain integration, while keeping QML and provider routing away from raw secrets.

Runtime behavior:

- The desktop app uses the disabled backend by default; store, read, delete, and contains refuse
  safely and do not mutate state.
- macOS Keychain, Windows Credential Manager, and Linux Secret Service are represented as
  non-executing placeholders with no platform-specific hard dependency.
- The in-memory backend exists for tests only and is not wired into the desktop controller.
- Backend operation results expose provider id, credential name, backend kind, readiness, success
  posture, mutation posture, and safe reason text. They do not include API-key values.

Out of scope:

- User key entry, real OS secret-store calls, plaintext persistence, cloud/provider calls,
  provider test calls, hidden retries, background probing, filesystem scanning, subprocess
  execution, and autonomous provider switching.

## 6. Memory Persistence Boundary

Decision: Memory storage is hidden behind `IMemoryStore`.

Reason: In-memory and SQLite implementations must share one small contract.

Storage decision:

- Desktop memory is stored at `QStandardPaths::AppDataLocation + "/memory.sqlite3"`.
- SQLite is accessed through Qt SQL and `QSQLITE`.
- Path ownership now routes through `IPathProvider` (`StandardPathProvider` by default).

Lifecycle decision:

- Memory maintenance includes a clear operation through the `IMemoryStore` boundary.
- UI receives generic maintenance status strings only.

## 7. Settings Persistence Boundary

Decision: Settings storage is hidden behind `ISettingsStore`.

Reason: Settings defaults, validation, and persistence can evolve without coupling QML to file IO.

Storage decision:

- Desktop settings are stored at `QStandardPaths::AppConfigLocation + "/settings.json"`.
- Path ownership now routes through `IPathProvider` (`StandardPathProvider` by default).

## 8. Settings And Memory Stay Separate

Decision: Settings and memory persistence use separate stores and separate files.

Reason: Settings are user/application configuration. Memory entries are runtime AI context data. Combining them would blur ownership and migration paths.

## 9. Chat History Persistence Boundary

Decision: Chat history persistence is hidden behind `IChatHistoryStore`.

Reason: Chat messages are ordered conversation records, not key-value memory. They need their own contract, schema, clear operation, and migration path.

Storage decision:

- Desktop chat history is stored at `QStandardPaths::AppDataLocation + "/chat_history.sqlite3"`.
- SQLite is accessed through Qt SQL and `QSQLITE`.
- Stored fields are `id`, `role`, `content`, `timestamp`, and `status`.
- Rows load in ascending `id` order.
- The schema metadata table stores `schema_version = 1`.

Runtime behavior:

- `ApplicationController` loads persisted messages when available.
- New runtime messages are appended to the chat history store when available.
- Clearing chat clears runtime and persistent chat history when available.
- If persistent chat storage is unavailable, runtime clear still succeeds with generic runtime-only status.
- Runtime chat continues if chat persistence is unavailable.
- QML receives only generic chat history status, such as `Available` or `Runtime Only`.
- QML receives only generic maintenance statuses, such as `Ready`, `Clear completed`, `Runtime Only`, or
  `Unavailable`.
- The desktop UI confirms before clearing local chat history.

Data ownership rule:

- Local data categories remain explicit and separate:
  - Settings (`ISettingsStore`)
  - Memory (`IMemoryStore`)
  - Chat history (`IChatHistoryStore`)
- Clear actions for memory/chat must not delete settings.

Export decision:

- Desktop transcript exports are stored at `QStandardPaths::AppDataLocation + "/exports"`.
- Export is current-transcript only and supports Markdown and JSON.
- Export filenames are sanitized, timestamped, and unique.
- QML receives only status, safe filename, message count, timestamp, and summary metadata.
- No arbitrary output path, file picker, import, cloud sync, or external process is available.

Current limitation:

- Chat history is a single local transcript.
- Multi-conversation/thread support is intentionally not implemented yet.
- Encryption, import, arbitrary export locations, and pruning are intentionally not implemented yet.

## 9.1 Conversation Runtime State And Continuity

Decision: Keep transient conversation runtime state in `ApplicationController` and expose it
through `DesktopShellViewModel` as QML-safe summaries only.

Reason: Chat history persistence, conversation graph metadata, and async inference request state
are related but separate responsibilities. The UI needs a concise current-session read model
without gaining access to raw workers, provider objects, request structs, traces, or SQLite state.

Runtime behavior:

- Runtime state tracks current graph state, current request id, active model, active route, active
  streaming flag, last successful response summary, last error/refusal summary, and last latency
  summary.
- Chat send state is explicit and controller-owned: idle, validating, sending, streaming,
  completed, refused, failed, and cancelled. QML receives state and summary strings only.
- A refusal happens before transcript mutation. Empty prompts, archived conversations, duplicate
  active requests, invalid endpoints, unreachable Ollama, missing model selection, and selected
  model missing states do not add user prompts or fake assistant messages.
- An accepted request may later fail. In that case the user prompt has already been committed, the
  lifecycle becomes failed, streaming preview is cleared, and exactly one safe assistant error
  message may be appended for that accepted request.
- Conversation history UX metadata tracks only the active single transcript: persistence status,
  message counts, last save status, last restore status, and clear result summary.
- Async local inference completions remain request-id guarded. Stale completions after metadata
  cancellation do not update visible state or chat history.
- Restart loading uses persisted chat rows directly when available. Startup creates the default
  system message only when no persisted transcript exists.
- Clear Chat clears runtime state, active request metadata, live streaming text, and persistent
  chat consistently through `IChatHistoryStore` when available, then reseeds the single system
  message.

Out of scope:

- Multi-conversation storage, transcript browser/search, export/import, encryption, pruning,
  provider expansion, cloud/API keys, model downloads/deletes, tools/plugins, filesystem/system
  actions, and voice execution changes.

## 9.2 Transcript Search And Local Export

Decision: Keep transcript search and export controller-owned, local-only, and scoped to the current
single transcript.

Reason: The current product still has one local transcript. The UI needs small QA/readiness
visibility and a safe local export action without adding database indexing, arbitrary filesystem
authority, import, or multi-conversation workflows.

Runtime behavior:

- Search uses `ConversationSearchQuery`, `ConversationSearchResult`, and
  `ConversationSearchSummary` over the current in-memory `ChatSession` messages.
- Search is literal and case-insensitive. Empty queries return an empty-query summary.
- Search updates only search metadata and does not append, remove, persist, reorder, or mutate chat
  messages.
- Clear Chat resets search state.
- Export uses `ConversationExportFormat`, `ConversationExportRequest`,
  `ConversationExportReadiness`, and `ConversationExportResult` metadata for Markdown and JSON.
- Export writes only to the app-controlled export directory below Qt `AppDataLocation`.
- Empty transcripts with only the initial system message are refused.
- Export result metadata includes status, safe output filename, exported message count, timestamp,
  and safe error/refusal summaries.
- Phase 15.17-15.19 adds value-only browser-readiness metadata on the same boundary:
  `ConversationDisplayTitle`, `ConversationListEntry`, `ConversationListSummary`, and
  `ConversationBrowserStatus`.
- Browser metadata currently exposes exactly one current transcript entry with title, message count,
  persistence status, last updated/saved summary, and search/export availability summaries.
- Phase 15.20-15.22 adds metadata-only multi-conversation planning readiness:
  `ConversationId`, `ConversationDescriptor`, `ConversationLifecycleStatus`,
  `ConversationStorageMode`, `ConversationMigrationReadiness`, and `ConversationSchemaPlan`.
- Planning metadata reports current mode (`Single Transcript`), future mode (`Multi Conversation`),
  migration readiness (`Not Started`), migration status summary (`Not Started / Planned`), and
  schema status summary without mutating storage.
- Phase 15.23-15.25 adds `IConversationStore` as the real multi-conversation storage boundary:
  `InMemoryConversationStore` for deterministic runtime/test use and `SQLiteConversationStore` for
  future persisted multi-conversation records.
- The conversation store is separate from `IChatHistoryStore`; the current desktop chat transcript
  remains single-transcript, and no automatic migration or destructive cleanup is performed.
- QML receives only strings, string lists, booleans, and counts through `DesktopShellViewModel`.

Out of scope:

- Vector search, semantic search, embeddings, SQLite full-text indexes, persisted search state,
  file pickers, arbitrary output paths, import, full transcript browser UI, cloud sync, encryption,
  pruning, broader filesystem/system actions, cloud/API keys, tools, and plugins.

## 9.2.1 Conversation Compression Readiness Metadata

Decision: Conversation compression readiness is controller-owned, deterministic metadata only.

Reason: Long-conversation pressure needs visible planning before any summarization or transcript
mutation phase. The UI can explain pressure and candidate shape without gaining authority to
summarize, replace, persist, or inject hidden content.

Runtime behavior:

- Readiness uses message count, estimated character/token budget, active conversation length,
  context injection state, existing deterministic summary availability, and salience budget
  pressure.
- Candidate planning labels recent conversation window, older conversation segment,
  high-salience user facts, low-salience repeated turns, and system/runtime metadata exclusion.
- Planning returns QML-safe summaries, counts, fallback text, budget usage, and traces only.

Out of scope:

- Hidden summarization, model calls, transcript mutation/replacement, committed memory writes,
  raw prompt/debug dumps, prompt alteration outside the existing explicit context-injection path,
  semantic/vector activation, filesystem indexing, cloud/API calls, and background workers.

## 9.2.2 Explicit Summary Generation Preparation

Decision: Summary generation is controller-owned, manual-only, active-conversation-only local
execution through the existing local inference boundary.

Reason: Users need an explicit local compression aid for long conversations, but the app must not
create summaries autonomously, alter transcripts, write memory, use cloud providers, expose hidden
prompts, or expand runtime authority.

Runtime behavior:

- A summary request must carry explicit user action metadata and must target the active
  conversation.
- Background requests, inactive or archived conversation requests, busy generation, transcript
  mutation, committed memory writes, hidden prompt exposure, tools/plugins, filesystem authority,
  missing local runtime/model readiness, and runtime/system metadata inclusion are refused before
  execution.
- Planning emits deterministic segments for recent-window retention, older-window summary
  preparation, important facts, repeated turns, and system/runtime metadata exclusion.
- Execution uses only the existing local inference worker and local Ollama readiness path, with
  request-id and active-conversation stale completion suppression.
- Persistence stores only sanitized summary text, summary timestamp, source conversation id,
  covered message range, estimated reduction, readiness state, and a safe summary line.
- Explicit prompt context injection may include the generated summary as the deterministic
  Conversation Summary candidate, preserving stable source ordering and transcript history.

Out of scope:

- Autonomous/background summaries, hidden prompt persistence, transcript replacement, automatic
  memory writes, semantic/vector authority, filesystem indexing, tools/plugins, subprocess
  expansion, and cloud/API providers.

## 9.2.3 Summary-Aware Long Conversation Continuity

Decision: Explicit persisted local summaries may act as deterministic continuity context only after
readiness, ownership, safety, freshness, and coverage validation.

Reason: Long-running conversations need durable compressed continuity after restart and context
pressure, but summaries must not become autonomous memory, semantic authority, or hidden transcript
replacement.

Runtime behavior:

- Summary validation checks active conversation ownership, unarchived state, `Ready` readiness,
  valid timestamp, valid covered range, transcript compatibility, non-empty sanitized text, and a
  deterministic freshness window.
- Valid summaries are selected through the existing context path after active conversation recency
  and before committed memory, runtime metadata, orchestration metadata, and budget constraints.
- Recent transcript turns stay preserved; old messages are not deleted, replaced, reordered, or
  hidden.
- The controller exposes QML-safe continuity status, freshness, coverage, contribution, fallback,
  ordering, and deterministic budget trace strings.
- Stale, invalid, incompatible, archived, unavailable, and budget-excluded summaries fall back to
  transcript-only context.

Out of scope:

- Autonomous summary generation, background execution, hidden transcript replacement, semantic/
  vector retrieval, filesystem indexing, cloud/API providers, tools/plugins activation, hidden
  prompt dumping, and automatic committed-memory writes.

## 9.3 Active Conversation Lifecycle And Switching

Decision: Use `IConversationStore` as the active local transcript source, with
`ApplicationController` owning the active conversation id and QML receiving only safe browser
metadata.

Reason: Multi-conversation persistence is now available, but session switching must not give QML
direct store access or weaken the async local inference safety model.

Runtime behavior:

- Startup ensures there is always a valid active conversation when the conversation store is ready.
- If the store is empty, the controller creates a local conversation and copies existing
  single-transcript startup messages into it when possible.
- Switching conversations loads messages from `IConversationStore` into `ChatSession`, resets
  runtime/search metadata, clears streaming preview text, and invalidates any active request id.
- Stale async results are ignored by the request-id guard and cannot append assistant messages to a
  newly selected conversation.
- Local inference requests also capture the active conversation id at acceptance time. If a result
  arrives after the active conversation changed, the controller clears busy/preview metadata and
  ignores the result instead of committing it to the wrong transcript.
- Rename/archive/unarchive are controller actions over `IConversationStore`; no permanent delete UI
  is exposed.
- Pin/unpin are controller actions over persisted local `IConversationStore` metadata. Pin state is
  local-only and has no cloud sync behavior.
- Duplicate is a controller-owned local operation. It creates a deterministic `Original title Copy`
  conversation and copies transcript messages through the conversation store when the current store
  safely supports message load/append.
- Archived conversations are visible and loadable, but sending into an archived active
  conversation is refused.
- Phase 15.30 through Phase 15.32 keep archive as the primary safe removal flow. Delete readiness
  is metadata-only through `ConversationDeletePolicy`, `ConversationDeleteReadiness`, and
  `ConversationDeleteResult`.
- Permanent delete is not enabled yet. A delete request is a guarded refusal path that reports
  status and does not call the store delete operation or mutate transcripts. Archive remains the
  available safe removal flow.
- Future permanent delete requires an explicit phase gate, destructive-mutation tests, guarded
  confirmation UI, and continued QML-safe status exposure before any destructive action is enabled.

Out of scope:

- Cloud sync, import, multi-conversation export, permanent delete UI, embeddings/vector DB,
  semantic memory, tools/plugins/system execution, and changes to Ollama/runtime safety policy.

## 9.4 Context Selection And Prompt Injection

Decision: Keep prompt context selection deterministic, local, and controller-owned.

Reason: Sentinel needs clearer local context quality and visibility without granting semantic,
filesystem, cloud, tool, or background authority.

Runtime behavior:

- Context sources are classified as recent conversation window, deterministic conversation summary
  metadata, committed key-value memory, runtime metadata, orchestration metadata, and selected
  conversation metadata.
- Selection uses fixed source priority, stable tie-breaking, bounded character budget, bounded
  candidate/source counts, duplicate suppression, and explicit exclusion reasons.
- Committed memory enters prompt-time context only when deterministic literal/key metadata overlaps
  the user prompt. Pending/rejected memory candidates are never injected.
- Injection remains disabled by default and only prepends a compact delimited local context block
  through the existing explicit setting.
- QML receives safe source, count, budget, and trace summaries only. Raw prompts, vector scores,
  provider handles, and filesystem paths are not exposed.

Out of scope:

- Semantic/vector retrieval activation, embeddings, vector databases, filesystem indexing,
  background summarization, autonomous memory writes, cloud/API calls, tools/plugins, subprocesses,
  raw prompt dumps, and prompt mutation outside the existing explicit injection path.

## 9.4.1 Adaptive Context Budgeting And Conversation Salience

Decision: Add conversation salience as deterministic value metadata over existing local context
candidates.

Reason: Local prompt context quality needs to account for the active conversation and recent
dialogue without introducing semantic/vector authority or hidden background processing.

Runtime behavior:

- Salience records are value-only: policy, candidate, score, reason, budget, selection, trace, and
  summary.
- Scoring uses literal active title overlap, recent user message overlap, recent assistant message
  overlap, pinned conversation metadata, committed memory overlap, explicit user query terms, and
  deterministic recency weighting.
- Context capacity is split deterministically between active conversation context, selected
  committed memories, and runtime/orchestration metadata.
- Prompt injection remains opt-in. When disabled, prompts are unchanged.
- QML receives concise summaries, counts, budget allocation, reasons, and traces only.

Out of scope:

- Embeddings, vector search, semantic ranking authority, filesystem indexing, background
  summarization, autonomous memory writes, tools/plugins, cloud/API calls, STT/TTS activation,
subprocesses, hidden workers, and raw prompt/debug dump exposure.

## 9.4.2 Context Observability And Explainability

Decision: Expose context orchestration explainability as deterministic value-only metadata.

Reason: Users and developers need to understand why local context was included, excluded,
truncated, ordered, or placed in fallback without gaining access to hidden prompts, raw provider
payloads, semantic/vector internals, or unsafe runtime authority.

Runtime behavior:

- `ContextDecisionReason`, `ContextDecisionTrace`, `ContextDecisionBudget`,
  `ContextDecisionContribution`, `ContextDecisionFallback`, `ContextDecisionSummary`, and
  `ContextDecisionVisibility` are read models over existing context injection, salience, memory
  relevance, and summary continuity metadata.
- Budget diagnostics report allocated characters, approximate tokens, remaining budget,
  compression gain, and contribution by transcript, summary, memory, and runtime metadata.
- Ordering diagnostics report recent transcript, continuity summary, committed memory, and runtime
  metadata in stable order.
- Normal UI shows concise context reasoning. Developer Mode shows richer bounded traces, still as
  strings/counts only.

Out of scope:

- Hidden prompt dumping, raw provider payload exposure, raw system prompt exposure, semantic/vector
  authority, filesystem indexing, cloud/API expansion, tools/plugins activation, autonomous
  behavior, transcript mutation, and hidden background workers.

## 9.4.3 Context Explainability Visibility Control

Decision: Persist `contextExplainabilityVisible` as a UI visibility preference, default enabled.

Reason: Users should be able to keep normal chat surfaces visually quiet without changing the
deterministic context path or weakening Developer Mode diagnostics.

Runtime behavior:

- `AppSettings` owns the persisted setting and `DesktopShellViewModel` exposes it to QML.
- The setting controls normal UI visibility for concise context reasoning surfaces only.
- Safe context decision metadata continues to be generated internally when the UI surface is
  hidden.
- Developer Mode remains a separate diagnostic visibility gate and does not inherit runtime
  authority from this setting.

Out of scope:

- Prompt assembly changes, retrieval behavior changes, metadata generation disablement, hidden
  prompt exposure, semantic/vector authority, provider/cloud activation, tool/plugin execution,
  autonomous behavior, transcript mutation, and background workers.

## 9.5 Conversation Delete Readiness Checkpoint

Decision: Keep archive/unarchive as the only supported local removal lifecycle and keep permanent
delete disabled until a later explicit destructive phase.

## 10. Product UI Synchronization And Developer Mode

Decision: Treat Phase 19 as a UI presentation synchronization pass over the existing local runtime
state, not as a runtime authority phase.

Reason: The backend exposes substantial metadata and a controlled local Ollama chat path, while
the desktop UI had drifted toward a diagnostics dashboard. The product needs a clear assistant
entry point without weakening the existing execution, provider, tool, semantic, or voice
boundaries.

Runtime behavior:

- Home is the primary chat surface and calls only the existing controller/view-model send path.
- The right-side AI Bridge remains a secondary chat and transcript surface.
- Send controls appear only when local chat inference is explicitly enabled and a local Ollama
  model is available.
- Developer Mode is persisted in settings and reveals advanced metadata only.
- Developer Mode does not alter runtime permission policy, safety policy, provider routing, tool
  contracts, agent behavior, voice readiness, semantic authority, or execution gates.
- Mode selection affects presentation density and diagnostic visibility only.
- Local Ollama loopback remains the only current inference endpoint. No cloud provider or external
  API key configuration is active.

Out of scope:

- External API providers, cloud routing, model downloads/deletes, tool/plugin execution, approval
  workflows, autonomous loops, microphone capture, playback, Whisper/Piper execution, subprocess
  launch, filesystem scanning, and semantic authority expansion.

Reason: Multi-conversation browsing is active, but destructive deletion needs a separate phase gate,
confirmation UX, mutation tests, and migration/retention decisions. Current QA should prove the
path is non-mutating instead of enabling deletion.

Runtime behavior:

- `ConversationDeletePolicy`, `ConversationDeleteReadiness`, and `ConversationDeleteResult` expose
  value-only status.
- `ApplicationController::requestPermanentDeleteConversation()` refuses and does not call
  `IConversationStore::deleteConversation()`.
- `SQLiteConversationStore::deleteConversation()` remains soft metadata only for future readiness:
  deleted conversations are hidden from normal list/load APIs, while stored message rows remain in
  the database.
- Archived conversations are visible and loadable, but new sends are blocked until unarchived.
- Switching conversations invalidates active async request ids so stale completions cannot mutate
  the newly active transcript.

## 11. Local Ollama Send Readiness

Decision: Local chat send availability is a controller-owned readiness contract exposed through
QML-safe view-model properties.

Reason: The UI previously had to infer readiness from separate Ollama/model/busy fields, which
could make Ollama appear connected while generation was blocked or unclear. A single readiness
summary keeps normal UI understandable while Developer Mode retains detailed diagnostics.

Runtime behavior:

- Send availability requires local chat enabled, local loopback Ollama, reachable health, a
  discovered installed model list, an explicitly selected model present in that list, an
  unarchived active conversation, and no active request.
- Readiness failures refuse before appending a user message. This applies to Ollama unreachable,
  invalid endpoint, no selected model, selected model missing, unavailable model list, archived
  conversation, and runtime busy states.
- Started local inference requests still use the async worker/request-id guard. Cancellation,
  clear-chat, and conversation switching invalidate active request ids so stale completions cannot
  append to the current transcript.
- Streaming preview is transient. Completed streams append one assistant response; failed streams
  clear preview text and append one safe failure response only for requests that actually started.

Out of scope:

- Cloud/API providers, API keys, model downloads/deletes, tools/plugins, filesystem/shell/
  subprocess execution, STT/TTS activation, autonomous actions, and semantic authority expansion.

Out of scope:

- Permanent delete execution, cloud sync, import/export workflow changes, semantic/vector memory,
  model/voice/tool/plugin changes, broad UI redesign, and runtime authority expansion.

## 10. Agent Capability Registry Boundary

Decision: Represent future agent capabilities as deterministic value-only registry metadata until
a later explicit runtime phase grants authority.

Reason: The desktop alpha needs visibility into planned agent capability categories without
creating hidden execution paths, approval flows, or tool/plugin/filesystem/shell access.

Runtime behavior:

- `AgentCapabilityRegistry` lives behind `IAgentTaskRuntime` beside task queue and planning
  session metadata.
- Local metadata capabilities may report readiness for conversation summarization, memory
  inspection, retrieval preparation, semantic supplement preparation, export preparation, and voice
  response preparation.
- Future filesystem access, shell execution, and plugin runtime entries remain disabled or refused
  and expose safe restriction/refusal summaries only.
- `executionAttempted` remains false for registry and capability safety reports.
- QML receives only counts, strings, and string lists through the controller/view-model boundary.

Out of scope:

- Runtime permission grants, tools, plugins, filesystem actions, shell/subprocess execution,
  provider/model calls, cloud/API calls, autonomous loops, and approval workflows.

## 10.1 Tool Contracts Are Permission Metadata Only

Decision: Represent future tools through deterministic `ToolContractRegistry` metadata before any
tool runtime, permission grant, sandbox implementation, approval workflow, filesystem adapter,
subprocess boundary, plugin host, or export action exists.

Reason: The Agents surface needs visibility into future tool categories and their permission/
sandbox posture without implying that the desktop app can execute tools or request approval.

Runtime behavior:

- `ToolContractRegistry` lives behind `IAgentTaskRuntime` beside the task queue, planning session,
  and capability registry metadata.
- Enabled contracts are local-only, read-only metadata contracts for conversation summary, memory
  inspection, retrieval preparation, semantic supplement preparation, voice response preparation,
  and export preparation.
- Permission labels are summaries only: local-only, approval-required, sandbox-required,
  read-only, disabled, refused, future filesystem access, future subprocess execution, future
  plugin runtime, and future export action.
- Sandbox labels summarize readiness only: not required, required metadata, or denied.
- Future filesystem, subprocess, plugin, and export action contracts remain disabled or refused.
  Unsafe scopes are denied and expose refusal metadata.
- `executionAttempted` remains false for registry and contract safety reports.
- QML receives only counts, strings, and string lists through the controller/view-model boundary.

Out of scope:

- Tool execution, plugin loading, filesystem actions, subprocess execution, export execution,
  sandbox enforcement, approval UI, provider/model calls, cloud/API calls, autonomous loops, and
  runtime permission grants.

## 10. Controlled Semantic Memory Candidates

Decision: Semantic memory begins as reviewable candidate metadata, separate from key-value memory,
memory taxonomy, chat history, and conversation storage.

Reason: The desktop alpha needs a safe architecture path for future semantic memory without adding
embeddings, vector storage, autonomous capture, provider calls, or hidden long-term memory
mutation.

Runtime behavior:

- Candidates are represented by value-only records and stored through `IMemoryCandidateStore`.
- The default implementation is `InMemoryMemoryCandidateStore`; no durable semantic memory store
  exists yet.
- Candidates created from conversation text metadata default to Pending Review.
- Approve/reject/reset/archive actions update review metadata only and return
  `MemoryCandidateReviewResult` summaries.
- Reviewed timestamp, reviewer/source summary, and decision reason fields describe the review
  lifecycle; they are not committed memory payloads.
- Approved candidate metadata is not automatically committed to `IMemoryStore` or any semantic
  database.
- Approved means reviewed candidate metadata. Committed memory remains a separate future phase gate.
- Clear Chat does not clear approved candidate metadata unless a later phase explicitly scopes
  that lifecycle.

Out of scope:

- Embeddings, vector database, semantic search, automatic memory writes, cloud sync, model/provider
  calls, filesystem/system actions, tools/plugins, and automatic capture toggles.

## 10.1 Approved Memory Commit Planning

Decision: Approved memory candidates may produce explicit commit-planning metadata, but actual
commit execution was disabled by default and future-gated in Phase 16.7-16.9.

Reason: The app needs a clear candidate -> approved -> committed vocabulary before any long-term
memory mutation is enabled. Approval should never be mistaken for storage.

Runtime behavior:

- `MemoryCommitPlan`, `MemoryCommitTarget`, `MemoryCommitReadiness`, `MemoryCommitResult`, and
  `MemoryCommitPolicy` describe commit intent and safety status only.
- The only current target vocabulary is key-value memory through the existing `IMemoryStore`
  boundary.
- Approved candidates produce deterministic key/value plan summaries.
- Pending, rejected, archived, missing, and store-unavailable candidates cannot commit.
- QML receives strings, string lists, and counts only.

Phase 16.10-16.12 follow-up from this planning decision:

- The explicit policy gate, narrow Commit action, mutation tests, duplicate-key refusal, and
  candidate/key-value/chat separation are now implemented.

Out of scope:

- Automatic memory writes, autonomous memory mutation, embeddings, vector DB, semantic search,
  provider/model calls, cloud sync, filesystem/system actions, tools/plugins, and durable semantic
  memory persistence.

## 10.2 Explicit User-Controlled Memory Commit

Decision: Approved memory candidates may be committed to local key-value memory only through an
explicit user Commit action.

Reason: Approval is a review decision. Commit is a separate local storage mutation and must remain
visible, user-controlled, and test-covered.

Runtime behavior:

- `MemoryCommitStatus`, `MemoryCommitConflictPolicy`, and `MemoryCommitResult` record committed
  state, conflict policy, committed key, timestamp, status, and summary.
- The default commit policy allows explicit user action and refuses existing keys. Overwrite is not
  enabled.
- Commit keys are sanitized deterministic keys derived from candidate category, title, and id.
- `requestMemoryCandidateCommit()` writes to `IMemoryStore` only after the candidate is Approved,
  the store is available, the explicit policy allows the action, and duplicate-key checks pass.
- Pending, rejected, archived, missing, store-unavailable, already-committed, and duplicate-key
  requests refuse before storage mutation.
- The key-value memory value is reviewed candidate content only. Source/review metadata is exposed
  in the result and candidate committed summary because the current memory store supports exact
  key/value entries only.
- Clear Chat does not clear committed key-value memory.
- QML receives only strings, string lists, booleans, and counts.

Out of scope:

- Automatic commit on approval, autonomous memory mutation, overwrite UI, embeddings, vector DB,
  semantic search, provider/model calls, cloud/API keys, tools/plugins, filesystem/system actions
  beyond the existing memory store, and durable candidate persistence.

## 10.3 Semantic Candidate Orchestration Before Semantic Retrieval

Decision: Add semantic candidate orchestration as deterministic metadata before any semantic
ranking, vector search, or prompt participation is enabled.

Reason: Hybrid retrieval needs a stable vocabulary for candidate sources, participation budgets,
arbitration, and fallback behavior before real semantic retrieval can safely influence prompts.

Runtime behavior:

- Semantic candidate records are value-only and derived from existing local metadata sources:
  recent conversation windows, deterministic summaries, committed memory, runtime metadata,
  orchestration metadata, and a disabled future semantic/vector source.
- Arbitration uses fixed deterministic source ordering and character budgeting. Excluded and
  truncated candidates are reported as metadata.
- Source isolation is preserved. Conversation-derived sources keep their existing chronology
  guarantees from the window/summary assemblers.
- Hybrid retrieval readiness reports deterministic retrieval as authoritative and semantic
  retrieval as disabled.
- `ApplicationController` and `DesktopShellViewModel` expose only QML-safe strings, string lists,
  booleans, and counts.
- Prompt assembly remains owned by deterministic retrieval planning. Semantic candidates do not
  mutate prompts or inject semantic/vector payloads.

Out of scope:

- Real embeddings, semantic ranking/search, vector database activation, transformer inference,
  provider/model calls, cloud/API keys, semantic prompt injection, filesystem/system actions,
  tools/plugins, raw vector/score UI, and runtime authority expansion.

## 10.4 Isolated Embedding Runtime Before Semantic Authority

Decision: Local embedding generation may run only as an isolated readiness-validation path and
must not grant semantic retrieval authority.

Reason: The app needs to validate future local embedding runtime behavior without allowing
embeddings to affect prompts, retrieval ranking, memory persistence, vector storage, filesystem
indexing, or background work.

Runtime behavior:

- `EmbeddingRuntimeStatus`, `EmbeddingRuntimeHealth`, `EmbeddingRuntimeSession`,
  `EmbeddingGenerationResult`, `EmbeddingGenerationPolicy`, `EmbeddingGenerationReadiness`, and
  `EmbeddingIsolationPolicy` describe the isolated path.
- Generation is allowed only when local-only mode, explicit semantic readiness, local/fake
  provider scope, no cloud providers, no filesystem indexing, no prompt integration, no retrieval
  ranking mutation, no automatic memory writes, no vector persistence, and no background indexing
  gates all pass.
- Fake/InMemory generation is allowed for deterministic tests. Local Ollama embeddings remain a
  local-only readiness/runtime path, not an active semantic retrieval provider.
- Timeout, stale request, busy session, provider failure, and policy refusal are explicit result
  states.
- QML receives status, health, readiness, bounded session state, counts, and checks only. Raw
  vectors, provider payloads, debug dumps, vector scores, provider handles, and index handles are
  not exposed.

Out of scope:

- Semantic retrieval activation, semantic prompt injection, vector database persistence, automatic
  ranking mutation, prompt assembly mutation, automatic memory writes, filesystem indexing,
  cloud/API keys, provider downloads, autonomous actions, tools/plugins, and background jobs.

## 10.5 Semantic Prompt Authority Default-Deny Policy

Decision: Semantic supplements may not enter live prompts unless a future phase explicitly
activates a separate prompt authority policy. The current authority policy is disabled and denies
by default.

Reason: Accepted semantic supplements and supplement assembly are readiness metadata. A separate
authority gate is required before semantic metadata can ever influence prompt construction.

Runtime behavior:

- `SemanticPromptAuthorityPolicy`, `SemanticPromptAuthorityStatus`,
  `SemanticPromptAuthorityResult`, `SemanticPromptAuthorityReadiness`,
  `SemanticPromptAuthorityDecision`, `SemanticPromptAuthoritySafetyReport`,
  `SemanticPromptAuthorityFallback`, and `SemanticPromptAuthorityAuditSummary` describe the gate.
- The evaluator reads `SemanticSupplementAssemblyResult` and emits Disabled/Denied by default.
- A test-only "would include metadata" decision requires local-only semantic search,
  deterministic acceptance, bounded supplement assembly, explicit prompt-injection enablement,
  explicit authority-policy allow, and a passing safety report.
- Disabled, stale, busy, refused, timed-out, unsafe, or unbounded states fall back to
  deterministic-only prompt behavior and produce audit reasons.
- Live prompt mutation remains blocked, and deterministic retrieval remains authoritative.
- QML receives only summaries, counts, statuses, decisions, checks, fallback, safety, and audit
  text. It does not receive raw prompts, raw supplement blocks, vectors, scores, filesystem paths,
  provider handles, or debug dumps.

Out of scope:

- Live semantic prompt injection, semantic authority escalation, cloud/API/vector providers,
  filesystem indexing, autonomous actions, tools/plugins, and prompt payload display.

## 10. AI Context Layer

Decision: Add repository-local AI instruction files in Phase 3.1.5.

Reason: Future AI coding sessions need a compact source of truth for phase status, architecture rules, and prompt guidance.

Expected effect:

- Smaller prompts.
- Less repeated context.
- Lower risk of architecture drift.
- Consistent constraints across Codex, Claude, ChatGPT, and similar agents.

## 11. UI/UX Roadmap Direction

Decision: Keep current UI as a foundation while planning a more mature Qt Quick experience.

Current state:

- Qt/QML shell.
- Chat, memory, settings, dashboard, bottom dock, header, and status bar.
- Minimal lifecycle UX around chat history.

Future direction:

- Smooth animations.
- Responsive layouts.
- Adaptive themes.
- Assistant-like chat interaction.
- Animated panels.
- Dashboard cards.
- Extensible component system.

Rule: future UI polish should stay behind QML/view-model boundaries and must not introduce business logic into QML.

## 12. Pre-agent Release Checkpoint

Decision: Add a stabilization checkpoint before Phase 4 implementation work.

Reason: The architecture needs an explicit audit gate so agent/tool runtime work does not begin on top of stale docs, inconsistent wording, or accidental boundary drift.

Checkpoint criteria:

- Core boundaries and platform boundaries remain explicit and interface-driven.
- QML exposure stays generic and does not leak SQLite/platform internals.
- Persistence separation remains strict across settings, memory, and chat history.
- Coverage exists for path ownership and maintenance status behavior.
- Release verification (`cmake --preset tests`, `cmake --build --preset tests`, `ctest --preset tests`) and formatting checks pass.

Out of scope during this checkpoint:

- Phase 4 agent/tool runtime implementation.
- Real provider/network integration and API keys.
- Plugin loading.
- Privileged automation.

## 13. Provider And Agent Runtime Separation

Decision: Keep provider and agent runtime boundaries separate.

Reason: Chat response generation and orchestration/tool planning are different concerns and should evolve independently without forcing cross-layer coupling.

Boundary rules:

- `IChatProvider` remains the chat generation contract.
- `IAgentRuntime` is the orchestration/runtime boundary for future action flows.
- Controllers and QML consume generic agent status/response surfaces, not runtime internals.
- Phase 4.0 runtime is local deterministic skeleton only (`NullAgentRuntime`) with no networking/tool execution.

## 14. Tool Descriptor And Registry Boundary

Decision: Separate tool metadata registration from tool execution.

Reason: The architecture needs a safe incremental path where tool identity, parameters, and risk metadata can be modeled and tested before any execution runtime is introduced.

Boundary rules:

- `ToolDescriptor` and related descriptor enums/structs model metadata only.
- `IToolRegistry` owns deterministic tool metadata registration and lookup.
- `InMemoryToolRegistry` is the default local deterministic registry implementation.
- Agent runtime may expose tool metadata from the registry, but must not execute tools in Phase 4.1.

## 15. Tool Invocation Planning Before Execution

Decision: Add a planning boundary before any tool execution runtime is introduced.

Reason: The app needs a safe intermediate layer where an agent can describe intended tool use in
structured data without gaining the ability to perform actions.

Boundary rules:

- Tool descriptors describe available tool metadata.
- Proposed invocation plans describe intent to use a tool.
- Invocation plans must remain value-only and non-operational.
- Agent runtimes may return proposed invocation plans, but neither controller nor QML may execute
  them.
- Controller and view-model exposure is limited to generic plan status and summary strings.
- Permission and sandbox runtime design must remain non-operational until a real execution phase is
  explicitly approved.

## 16. Approval Metadata Before Sandbox Or Execution

Decision: Model approval and permission state as metadata before introducing sandboxing or tool
execution.

Reason: Planned tool invocations need deterministic approval state and risk visibility without
granting the application any ability to perform actions.

Boundary rules:

- Approval policy evaluates `ToolInvocationPlan` values only.
- Approval decisions describe state such as not required, requires approval, approved, or denied.
- Permission descriptors are metadata labels, not runtime grants.
- Controller and view-model exposure is limited to generic approval status and summary strings.
- Approval does not execute tools, launch processes, mutate files, call networks, load plugins, or
  activate sandbox behavior.
- Real permission prompts, audit logs, sandboxing, and executors remain future work.

## 17. Sandbox Capability Metadata Before Runtime Enforcement

Decision: Model sandbox capability boundaries as deterministic metadata before introducing any
real sandbox runtime or tool executor.

Reason: The app needs a clear boundary between a plan, approval state, and the capabilities a
future runtime would require, without granting operating-system permissions or executing actions.

Boundary rules:

- Planning describes intended tool invocation metadata.
- Approval describes user or policy approval metadata.
- Sandbox policy describes whether planned capability metadata is within the allowed boundary.
- `ISandboxPolicy` evaluates `ToolInvocationPlan` and `ApprovalDecision` values only.
- `CapabilityDescriptor` values are labels for future runtime constraints, not real permission
  grants.
- Approval does not override sandbox capability denial.
- Controller and view-model exposure is limited to generic sandbox status and summary strings.
- Sandbox evaluation does not execute tools, launch processes, mutate files, call networks, load
  plugins, request privileges, or enforce an OS sandbox.

## 18. Placeholder Execution Boundary Before Real Executors

Decision: Add an execution ownership boundary that is explicitly placeholder-only before any real
tool execution implementation.

Reason: The application needs a stable interface for future execution ownership while preserving
the current no-action safety model.

Boundary rules:

- Planning chooses intended tool invocation metadata.
- Approval represents user or policy permission metadata.
- Sandbox policy represents capability-boundary metadata.
- `IToolExecutor` represents future execution ownership, but current implementations must remain
  non-operational.
- `NullToolExecutor` returns deterministic placeholder results only.
- Approved and sandbox-allowed plans may produce placeholder success, not real action.
- Denied, unapproved, sandbox-blocked, empty, or unknown-tool plans must be represented as blocked
  or safely rejected.
- Controller and view-model exposure is limited to generic execution status and summary strings.
- The execution boundary must not launch processes, spawn subprocesses, mutate files, call
  networks, load plugins, request privileges, invoke OS automation, or enforce a real sandbox.

## 19. Agent Pipeline Result Stabilization

Decision: Consolidate the controller-facing Phase 4 pipeline state into a value-based aggregate
result.

Reason: Planning, approval, sandbox, and placeholder execution statuses should move through one
deterministic result model so controller and view-model summaries do not duplicate string fallback
logic.

Boundary rules:

- The aggregate result represents metadata only.
- The full route remains:
  registry -> planning -> approval -> sandbox capability metadata -> placeholder execution boundary.
- QML may receive generic status and summary strings only.
- The aggregate result does not expose raw mutable runtime internals or execution controls.
- Stabilizing the result model does not authorize real execution, shell/process launch,
  filesystem mutation, networking, provider integrations, plugin loading, privileged automation, or
  real sandbox enforcement.

## 20. Runtime Context Is Metadata Ownership Only

Decision: Add a local runtime session/context owner that records the latest metadata-only agent
pipeline state.

Reason: Future agent orchestration needs a deterministic place to hold current runtime context
without confusing that context with execution, persistence, plugin loading, or sandbox enforcement.

Boundary rules:

- `RuntimeSession` owns an in-memory `AgentRuntimeContext`.
- `AgentRuntimeContext` may copy the latest pipeline result, active planned tool ids, approval
  metadata, sandbox metadata, and placeholder execution metadata.
- Session ids and revisions are deterministic local metadata.
- Runtime context is read-only at the QML boundary and exposes generic summaries only.
- Runtime context is not tool execution, planning, approval policy, persistence, plugin loading,
  sandbox enforcement, OS automation, networking, or privileged automation.
- Adding runtime context must not launch processes, spawn subprocesses, mutate files, call networks,
  read API keys, load plugins, or enforce a real sandbox.

## 21. Agent Activity Log Is In-Memory Metadata Only

Decision: Add an in-memory activity/audit trail skeleton for metadata-only agent pipeline events.

Reason: The agent pipeline needs deterministic observability before any future real execution or
security audit feature is introduced.

Boundary rules:

- `AgentActivityEntry` is value data with deterministic sequence id, activity type, status, and
  generic summary.
- `AgentActivityLog` is in-memory only in Phase 4.8.
- The controller may record request received, plan created, approval evaluated, sandbox evaluated,
  placeholder execution evaluated, and pipeline completed/blocked events.
- QML receives only aggregate read-only fields such as activity count and latest summary.
- Activity logging must not persist to files or SQLite, export data, record secrets, record raw
  system paths, launch processes, spawn subprocesses, mutate files, call networks, load plugins,
  enforce sandbox behavior, or perform tool execution.
- Future durable audit, export, pruning, redaction, and security review features must be explicit
  later phases.

## 22. Agent Pipeline UI Visibility Is Read-Only

Decision: Expose the current metadata-only agent pipeline state in the desktop UI as read-only
status text.

Reason: Users need visibility into the pipeline, runtime context, active planned tool ids, and
activity count before any future approval or execution UX exists.

Boundary rules:

- QML receives only simple view-model values such as strings, counts, and string lists.
- Dashboard visibility may show latest pipeline status/summary, runtime context status/summary,
  active planned tool ids, activity count, and latest activity summary.
- The UI must not expose raw pipeline results, runtime context objects, activity entries, mutable
  controller internals, paths, secrets, or platform details.
- Phase 4.9 does not add execution buttons, approval controls, provider integration, networking,
  plugin loading, activity persistence, shell/process launch, filesystem mutation, OS automation,
  privileged automation, or real sandbox enforcement.

## 23. Phase 4 Completion Checkpoint

Decision: Close Phase 4 with an architecture checkpoint before starting Phase 5 UI/UX work.

Reason: The agent/tool foundation now has multiple metadata-only boundaries. A checkpoint keeps
the no-execution model explicit before UI polish begins.

Boundary rules:

- Provider, agent runtime, tool registry, planning, approval, sandbox metadata, placeholder
  execution, runtime context, and activity logging remain separate responsibilities.
- Phase 5 may improve presentation, but must not turn metadata-only state into real execution,
  approval, sandbox enforcement, provider integration, plugin loading, networking, filesystem
  mutation, shell/process launch, subprocess launch, or privileged automation.
- QML remains a read-only consumer of agent/runtime state unless a later explicit phase changes
  that boundary.
- Checkpoint documentation should summarize completed scope, limitations, readiness criteria, and
  explicit out-of-scope work.

## 24. AI Orchestration Planning Is Separate From Runtime Execution

Decision: Plan future AI orchestration around a separate `ModelRouter` and `RoutingPolicy` concept
before Phase 5 UI work.

Reason: Model/provider selection needs its own architecture boundary so chat providers, agent
runtime metadata, tool execution, and model-management UI do not become coupled.

Boundary rules:

- `ModelRouter` and `RoutingPolicy` are future concepts only in Phase 4.11.
- Provider capability profiles, task classification, local/cloud fallback, device-aware selection,
  and user routing modes are metadata planning topics, not implemented runtime behavior.
- Sensitive data should prefer local-only routing, and cloud use must require explicit user
  permission in any future implementation.
- Future model-management UI should display metadata such as installed/downloadable models,
  recommendations, RAM/disk requirements, and local/cloud badges without owning routing logic.
- This planning does not add provider integrations, networking, API keys, model downloads, model
  execution, plugin loading, filesystem/system actions, real tool execution, approval UX, or
  sandbox runtime.

## 25. UI Design Tokens Before Advanced Motion

Decision: Start Phase 5 with a small QML design-token singleton and concise UI/UX plan.

Reason: The desktop shell needs shared palette, spacing, radius, and typography values before
larger UI polish, motion, or assistant visual work.

Boundary rules:

- `SentinelTheme.qml` owns reusable presentation constants only.
- QML may consume tokens for colors, spacing, radii, and text sizing, but must not gain business
  logic, provider behavior, model management, downloads, execution, or platform actions.
- Motion remains documented guidance only in Phase 5.0; no heavy animation or particle assistant
  visuals are implemented.
- UI changes must preserve current Dashboard, Chat, Memory, Settings, and runtime visibility
  behavior.

## 26. Lightweight Motion Before Assistant Visuals

Decision: Add only subtle interaction motion before any advanced assistant visual system.

Reason: The shell needs consistent hover, focus, and page/content transition behavior, but the app
must remain quiet and performant on Fedora KDE Plasma and macOS.

Boundary rules:

- Motion tokens are limited to standard durations and easing values in `SentinelTheme.qml`.
- Interaction polish may use cheap color, opacity, and border-color transitions.
- Avoid blur-heavy layers, particle systems, shader-heavy rendering, OpenGL/Vulkan custom renderers,
  and continuous idle animation.
- Motion must not trigger provider/model calls, tool execution, filesystem/system actions,
  networking, plugin loading, approval actions, or sandbox behavior.

## 27. Adaptive Layout Before Advanced Visual Systems

Decision: Add compact, normal, and wide layout behavior before building advanced assistant visuals.

Reason: The desktop shell should remain readable on narrower Linux/KDE and macOS windows while
preserving the current simple Qt/QML architecture.

Boundary rules:

- Responsive behavior is limited to QML presentation tokens, spacing, wrapping, dock sizing, and
  page/card column choices.
- Breakpoints should remain cheap width checks, not runtime services or device-specific platform
  probes.
- Narrow layouts should prefer wrapping and stable spacing over hiding core state.
- Adaptive layout work must not add provider/model execution, networking, plugin loading,
  filesystem/system actions, approval actions, sandbox behavior, particle systems, assistant-face
  rendering, or custom rendering systems.

## 28. Small Component Consistency Before Visual Expansion

Decision: Normalize repeated QML presentation patterns with small shared components before adding
larger visual systems.

Reason: Consistent inputs, status rows, button sizing, and card padding reduce UI drift while
preserving the existing view-model and runtime boundaries.

Boundary rules:

- Shared QML components may wrap presentation-only controls such as text fields and read-only info
  rows.
- These components must not own business logic, persistence, provider/model routing, approval,
  execution, filesystem/system actions, networking, plugin loading, or platform behavior.
- Visual QA guidance should cover current manual screen states and responsive widths.
- Automated UI driving, assistant-face rendering, particle systems, heavy animation, and custom
  rendering systems remain future work.

## 29. Workspace UX Reference Translation Stays Native

Decision: Treat the former `lovable-tasarim` reference as a historical design input whose useful
workspace ideas were translated into native Qt/QML components.

Reason: Sentinel must remain a native Qt/C++ modular monolith while borrowing only stable UI/UX
direction from the reference.

Boundary rules:

- Do not integrate React, Vite, Tailwind, Node, WebView, or web runtime dependencies.
- Preserve the cinematic AI operating environment direction: presence-first composition,
  translucent floating surfaces, soft glow hierarchy, thin-line geometry, and generous negative
  space.
- `SentinelTheme.qml` may own mode-aware visual helpers, but those helpers remain presentation-only.
- `Atmosphere`, `WorkspacePresence`, `ShellPanel`, dock, header, dashboard, and chat panel
  changes must bind to QML-safe view-model properties only.
- Workspace UI must not add provider/model execution, real tools, approval controls, sandbox
  runtime, plugin loading, networking, filesystem/system actions, voice, hardware integration,
  assistant-face rendering, advanced particles, heavy motion, or Qt Quick 3D.

## 30. Phase 5.4.5 Is A Stabilization Gate

Decision: Insert an architecture and UI risk audit checkpoint after Phase 5.4 and before Phase 5.5.

Reason: The workspace integration added more QML visual structure and mode-aware tokens; those
changes need a boundary/readability/tooling audit before additional UI work.

Boundary rules:

- Fix only small safe issues: stale docs, naming inconsistencies, duplicated simple styling, minor
  QML binding cleanup, component organization cleanup, label clarity, and checklist gaps.
- Do not redesign the UI or add product features.
- Verify with build, tests, formatting, available QML linting, and lightweight startup smoke checks.

## 31. Phase 5.5 Reconstructs Visual Identity Natively

Decision: Reconstruct the Phase 5.x shell around the translated Sentinel visual identity while
keeping Sentinel native Qt/QML and C++.

Reason: The technically correct Phase 5.4 workspace still read too much like a developer dashboard
or utility application. Sentinel needs a presence-first cinematic operating environment before
later assistant visual work.

Boundary rules:

- QML may add presentation primitives such as dock, orb, and telemetry surfaces.
- Visual state must bind only to QML-safe view-model properties.
- Do not add provider/model execution, real tools, networking, plugin loading, filesystem/system
  actions, web runtime integration, assistant-face rendering, particle engines, Qt Quick 3D, or
  custom rendering systems.

## 32. Model Routing Starts As Metadata Only

Decision: Add Phase 6.0 model/provider routing as value-based metadata and a deterministic static
router before any provider or model execution work.

Reason: Model/provider selection needs a testable architecture boundary that stays separate from
chat generation, agent orchestration, and tool execution.

Boundary rules:

- `IModelRouter` selects `ProviderDescriptor` and `ModelDescriptor` metadata only.
- `StaticModelRouter` is local-only and deterministic by default.
- `IChatProvider` remains the response-generation boundary.
- `IAgentRuntime` remains the agent/tool orchestration boundary.
- Controller and desktop view-model exposure must stay QML-safe strings/status values.
- No provider calls, networking, API keys, model downloads, model execution, real tool execution,
  plugin loading, filesystem/system actions, or broad UI work are part of Phase 6.0.

## 33. Routing Mode Preference Is Settings Metadata

Decision: Persist the user routing mode preference through `AppSettings` and `JsonSettingsStore`.

Reason: Routing mode affects future model/provider selection metadata and should survive app
restarts, but it must stay separate from credentials, provider setup, networking, and execution.

Boundary rules:

- Default routing mode is `Local Only`.
- Unknown routing mode values normalize back to `Local Only`.
- Settings persistence remains JSON-backed through `ISettingsStore`; SQLite is not used for this.
- QML may change the routing mode through `DesktopShellViewModel`, but routing logic remains in
  C++.
- Changing routing mode updates route metadata only; it does not call providers, use API keys,
  access the network, download or execute models, execute tools, load plugins, or perform
  filesystem/system actions.

## 34. Provider Catalog Is Metadata Only

Decision: Add a provider/model catalog boundary before provider integration or model-management
actions.

Reason: The desktop app needs deterministic metadata for future provider/model UI and routing
policy work without implying that providers are configured or executable.

Boundary rules:

- `IProviderCatalog` exposes read-only value metadata only.
- `StaticProviderCatalog` may describe future providers/models, including Ollama Local, OpenAI
  Cloud, and Anthropic Cloud placeholders.
- Catalog entries may include provider id/name, local/cloud kind, availability, supported task
  types, privacy level, and rough RAM/disk hints.
- Catalog entries must not include credentials, API keys, secrets, endpoints, transport clients, or
  executable provider objects.
- Cloud catalog entries remain not configured until a later explicit provider phase.
- `StaticModelRouter` may consume available catalog metadata but must not call providers, access the
  network, download models, execute models, load plugins, or execute tools.
- QML may show read-only text summaries only; no setup buttons, API key fields, download buttons,
  or execution controls are part of this phase.

## 35. Task Planner Is Metadata Only

Decision: Add a high-level task planner and capability graph boundary as value metadata before any
provider/model execution or tool execution work.

Reason: Sentinel needs deterministic planning visibility over task type, routing mode, privacy, and
provider/model capability metadata without granting runtime execution authority.

Boundary rules:

- `ITaskPlanner` creates `TaskPlan` metadata only.
- `StaticTaskPlanner` is deterministic and local-safe.
- Capability graph nodes and planned task steps are value data, not executable operations.
- Task planning may consider task type, routing mode, provider/model catalog availability,
  local/cloud suitability, privacy sensitivity, supported task metadata, and rough resource hints.
- Sensitive/private tasks must prefer or require local metadata.
- Unknown tasks must use a safe local metadata fallback when available.
- Unavailable providers/models must not be selected for executable routes.
- `IModelRouter` remains responsible for model/provider route metadata.
- `IAgentRuntime` remains responsible for future tool/action orchestration metadata.
- `IChatProvider` remains responsible for chat response generation.
- Task planning must not call providers, access networks, use API keys, download or execute models,
  execute tools, load plugins, mutate files, or perform system actions.
- QML may show read-only status, summary, and counts only; no execution controls are part of this
  phase.

## 36. Agent Registry Is Metadata Only

Decision: Add a static agent registry and agent descriptor model before any autonomous agent
runtime behavior.

Reason: Sentinel needs stable names, roles, capability summaries, and task affinities for future
orchestration UI and planning without implying that autonomous agents can run.

Boundary rules:

- `IAgentRegistry` exposes read-only value metadata only.
- `StaticAgentRegistry` returns deterministic static descriptors for Atlas, Orin, Vela, Kaze, Nyx,
  and Sol.
- Agent descriptors may include id, display name, role, capability summary, preferred task types,
  local/cloud affinity, privacy affinity, state, and priority.
- `StaticTaskPlanner` may select a preferred agent metadata label for a plan, but it must not
  execute the agent.
- `IAgentRuntime` remains the future execution/orchestration boundary.
- No autonomous loops, threads, background workers, provider/model calls, networking, API keys,
  downloads, tool execution, memory writes, plugin loading, filesystem/system actions, or dynamic
  discovery are part of this phase.
- QML may show read-only counts and summaries only; no execution controls or autonomous toggles are
  part of this phase.

## 37. Memory Taxonomy Is Metadata Only

Decision: Add a static memory taxonomy catalog before any semantic memory, vector memory, or
autonomous recall/write behavior.

Reason: Sentinel needs stable memory category names, retention/privacy labels, recall hints, and
planner affinity metadata without changing the existing key-value memory persistence boundary.

Boundary rules:

- `IMemoryCatalog` exposes read-only value metadata only.
- `StaticMemoryCatalog` returns deterministic descriptors for Episodic, Semantic, Procedural,
  Reflective, and Ambient memory categories.
- Memory descriptors may include type, shard status, retention policy, privacy level, recall hint,
  task affinities, tags, and association metadata.
- `IMemoryStore` remains the explicit key-value memory persistence contract.
- `SQLiteMemoryStore` remains simple key-value storage and is not replaced by the taxonomy catalog.
- `StaticTaskPlanner` may select a preferred memory affinity metadata label for a plan, but it must
  not recall, search, write, embed, vectorize, or mutate memory.
- No vector database, embeddings, semantic search, autonomous memory writes, provider/model calls,
  networking, API keys, downloads, tool execution, plugin loading, filesystem/system actions, or
  dynamic discovery are part of this phase.
- QML may show read-only counts and summaries only; no semantic search controls or memory graph
  execution controls are part of this phase.

## 38. Orchestration Snapshot Is A Read Model

Decision: Add a deterministic orchestration snapshot that aggregates existing metadata for
workspace visibility without adding orchestration execution.

Reason: The desktop shell needs one compact read model for routing, task planning, provider
catalog, agent, memory taxonomy, runtime context, and activity metadata before any future execution
or automation phase.

Boundary rules:

- `OrchestrationSnapshot` and `WorkspaceStateSummary` are value data only.
- The snapshot is built from existing metadata already owned by `ApplicationController`.
- The snapshot may include health status, summary text, and compact signal strings for QML.
- Snapshot updates are tied to existing metadata-only changes such as routing changes and runtime
  metadata updates.
- The snapshot must not execute plans, call providers, execute models, search memory, embed data,
  vectorize data, execute tools, load plugins, access networks, mutate files, perform system
  actions, or start autonomous loops.
- No background refresh, timers, threads, workers, downloads, API keys, credentials, or dynamic
  discovery are part of this phase.
- QML may show read-only snapshot status, summary, and signal strings only; no execution controls
  are part of this phase.

## 39. Orchestration Diagnostics Are Metadata Readiness Checks

Decision: Add deterministic orchestration diagnostics and readiness reports over existing metadata
without probing external systems or enabling execution.

Reason: Sentinel needs a compact readiness checklist for routing, catalogs, planning, privacy, and
execution-boundary posture before future provider/model work, but this phase must not imply hidden
provider setup or runtime capability.

Boundary rules:

- `OrchestrationDiagnostic`, `OrchestrationReadinessCheck`, and
  `OrchestrationReadinessReport` are value data only.
- `StaticOrchestrationDiagnostics` inspects existing `OrchestrationSnapshot` and provider catalog
  metadata only.
- Diagnostic ordering must remain deterministic for tests and UI.
- Checks may report metadata state for routing mode, selected route, provider catalog, agent
  registry, memory taxonomy, task planner, snapshot health, local-only privacy posture, cloud
  provider unavailability/not-configured status, and disabled execution capability.
- Diagnostics must not call providers, execute models, probe local model runtimes, read API keys,
  access networks, scan the filesystem, run external processes, execute tools, load plugins, build
  embeddings, run semantic/vector search, mutate memory, or start background workers.
- Controller and QML exposure stays read-only and QML-safe: status, summary, and diagnostic
  strings only.

## 40. Conversation Session Context Is Separate Metadata

Decision: Add a higher-level conversation/session context layer as deterministic metadata without
replacing chat history or Phase 4 runtime context.

Reason: The desktop shell needs a stable interaction context summary for future AI workspace
behavior, but the current alpha must keep chat messages, agent runtime metadata, and orchestration
metadata separate and non-operational.

Boundary rules:

- `ChatSession` remains the in-memory chat transcript owner and stays connected to
  `IChatHistoryStore` for persistence.
- `ConversationSession` owns interaction/session metadata only: session id/status, interaction
  mode, attention state, context scope, and context-window summaries.
- Phase 4 `RuntimeSession` remains the agent pipeline metadata owner and is not merged into
  `ConversationSession`.
- The conversation context window may copy routing mode, preferred agent summary, memory affinity
  summary, and latest orchestration snapshot summary.
- Controller and QML exposure stays read-only and QML-safe: strings only.
- This layer must not add multi-conversation persistence, provider/model calls, streaming, API key
  handling, networking, model downloads, model execution, tool execution, plugin loading,
  filesystem/system actions, embeddings, vector search, semantic search, autonomous workers,
  timers, threads, or external process calls.

## 41. Conversation State Graph Is Metadata Only

Decision: Add a deterministic conversation state graph for high-level interaction state metadata
without using state transitions as execution triggers.

Reason: The shell needs a readable current state and last-transition summary for future
conversation orchestration, while preserving the existing separation between chat transcripts,
conversation session context, and agent runtime metadata.

Boundary rules:

- `ConversationStateGraph` owns only the current conversation state and last transition result.
- `ConversationSession` remains the owner of session/context metadata.
- `ChatSession` remains the owner of chat message history.
- Phase 4 `RuntimeSession` remains the owner of agent pipeline runtime metadata.
- Valid transitions are deterministic metadata rules; invalid transitions are rejected with
  deterministic summaries.
- Controller and QML exposure stays read-only and QML-safe: current state, transition status, and
  transition summary strings only.
- Transitions must not call providers, execute models, stream tokens, execute tools, approve
  actions, load plugins, access networks, scan or mutate filesystems, perform system actions,
  build embeddings, run semantic/vector search, start background workers, or run external
  processes.

## 42. Phase 6 Checkpoint Before Runtime Work

Decision: Close Phase 6 with a pre-runtime architecture checkpoint before starting Phase 7.

Reason: Phase 6 introduced multiple metadata orchestration surfaces. A checkpoint keeps the
no-execution model explicit, records readiness criteria, and prevents Phase 7 from being treated
as implicit provider/model/tool execution.

Boundary rules:

- Phase 6.10 may update documentation and small safe consistency gaps only.
- The checkpoint must not add product features, provider integrations, networking, API key
  handling, downloads, streaming, model execution, real tool execution, plugin loading,
  filesystem/system actions, vector search, embeddings, semantic search, or autonomous workers.
- Phase 7.0 should begin with local runtime boundary planning and ownership mapping.
- Full provider/model execution, cloud routing, credentials, downloads, streaming, plugins, vector
  memory, and real tool execution require later explicit scopes.
- QML exposure remains through `DesktopShellViewModel` and QML-safe values only.
- `docs/PHASE_6_CHECKPOINT.md` is the durable checkpoint record for completed scope, known
  limitations, readiness criteria, and recommended Phase 7 breakdown.

## 43. Local Runtime Boundary Is Metadata Only

Decision: Add `ILocalRuntime` as the future local inference/runtime boundary with a
non-executable null implementation.

Reason: Sentinel needs an explicit owner for future local runtime metadata before any provider or
model execution is considered. The boundary should make local runtime readiness visible without
probing local services or implying executable capability.

Boundary rules:

- `ILocalRuntime` is separate from `IChatProvider`, `IModelRouter`, `IAgentRuntime`, and
  `IToolExecutor`.
- `LocalRuntimeDescriptor`, status, health, and capability values are metadata only.
- `NullLocalRuntime` reports deterministic metadata and refuses requests with a placeholder
  non-executable response.
- Controller and QML exposure stays read-only and QML-safe: status, health, summary, capability
  summaries, and refusal summary strings only.
- The local runtime boundary must not call Ollama/providers, execute models, download models,
  stream tokens, launch processes/subprocesses, scan or mutate filesystems, execute tools, load
  plugins, access networks, read API keys, or start background workers.

## 44. Local Runtime Sessions Are Ownership Metadata

Decision: Add local runtime session ownership/lifecycle metadata without allocating or executing
runtime resources.

Reason: Future local runtime work needs a stable place to describe session ownership, allocation,
and reservation state before any local provider/model execution can be considered.

Boundary rules:

- `LocalRuntimeSession` is not `ChatSession`.
- `LocalRuntimeSession` is not Phase 4 `RuntimeSession`.
- `LocalRuntimeSession` is not provider/model execution.
- `LocalRuntimeSession` is not tool execution or plugin ownership.
- `LocalRuntimeAllocation` and `LocalRuntimeReservation` are descriptive metadata only.
- `NullLocalRuntimeSessionManager` returns deterministic placeholder sessions only.
- Controller and QML exposure stays read-only and QML-safe: counts, status strings, summaries, and
  string lists.
- Session metadata must not allocate models, call providers, use API keys, access networks,
  download models, stream output, launch processes/subprocesses, scan or mutate filesystems,
  execute tools, load plugins, or start background workers.

## 45. Runtime Capability Negotiation Is Metadata Only

Decision: Add a runtime capability negotiation registry that describes what future runtime work may
support without activating or executing any capability.

Reason: Future runtime phases need a stable vocabulary for local inference, streaming, multimodal,
memory, tool/plugin bridge, filesystem/process, cloud relay, local-only, and privacy-safe
capabilities before any execution or permission policy is implemented.

Boundary rules:

- `IRuntimeCapabilityRegistry` exposes deterministic capability descriptors and negotiation
  metadata only.
- `StaticRuntimeCapabilityRegistry` enables only safety posture metadata: local-only enforcement
  and privacy-safe mode.
- Disabled and unavailable capabilities remain descriptive and non-executable.
- Runtime capability negotiation is separate from `ILocalRuntime`, local runtime sessions,
  `IChatProvider`, `IModelRouter`, `IAgentRuntime`, `IToolExecutor`, approval policy, sandbox
  policy, and plugin management.
- Controller and QML exposure stays read-only and QML-safe: counts, enabled/disabled summaries,
  negotiation profile summary, negotiation result summary, and local-only enforcement summary.
- Capability negotiation must not activate capabilities, call providers, use API keys, access
  networks, download models, execute models, stream output, launch processes/subprocesses, scan or
  mutate filesystems, execute tools, load plugins, approve permissions, or start background
  workers.

## 46. Runtime Permission Policy Is Metadata-Only Default-Deny

Decision: Add `IRuntimePermissionPolicy` with `StaticRuntimePermissionPolicy` as a deterministic
permission metadata boundary that denies execution-level runtime requests by default.

Reason: Runtime phases need explicit permission vocabulary and decision metadata before any future
execution authority is considered.

Boundary rules:

- `RuntimePermission`, `RuntimePermissionLevel`, `RuntimePermissionRequest`, and
  `RuntimePermissionDecision` are value metadata only.
- Default policy behavior denies execution-level requests in metadata-only mode.
- Permission metadata is separate from capability negotiation, runtime safety reporting, and runtime
  request pipeline status.
- Permission metadata must not execute models/providers/tools/plugins, launch processes, access
  filesystems, or access networks.

## 47. Runtime Request Pipeline Is Metadata Trace Only

Decision: Add `IRuntimePipeline` with `StaticRuntimePipeline` as a deterministic request-pipeline
metadata boundary.

Reason: Runtime readiness work needs ordered request/permission/safety/execution-boundary trace
visibility before any execution runtime is introduced.

Boundary rules:

- `RuntimePipelineRequest`, stage/status enums, traces, and results are value metadata only.
- The pipeline consumes existing permission/safety metadata and returns deterministic status/summary
  traces.
- Execution boundary stage remains blocked/no-execution in current phase.
- Pipeline metadata must not call providers/models, execute tools/plugins, launch processes, access
  filesystems, or access networks.

## 48. Runtime Safety Policy Reports Local-Only No-Execution Posture

Decision: Add `IRuntimeSafetyPolicy` with `StaticRuntimeSafetyPolicy` as deterministic runtime
safety posture metadata.

Reason: Runtime boundary stabilization needs explicit local-only/no-execution safety reporting
before sandbox/runtime execution work is approved.

Boundary rules:

- `RuntimeSafetyPolicy`, `RuntimeSafetyRule`, `RuntimeSafetyDecision`, and `RuntimeSafetyReport`
  are value metadata only.
- Safety reporting is deterministic and read-only for controller/view-model/QML visibility.
- Safety policy metadata is separate from sandbox runtime enforcement and execution ownership.
- Safety policy metadata must not activate runtime capabilities, call providers/models, execute
  tools/plugins, launch processes, access filesystems, or access networks.

## 49. Phase 7 Runtime Checkpoint Keeps Execution Out Of Scope

Decision: Close Phase 7 with an architecture checkpoint before Phase 8 planning.

Reason: The local runtime boundary now includes session, capability, permission, safety, and
pipeline metadata. A checkpoint keeps those responsibilities separate and prevents metadata
visibility from being treated as execution authority.

Boundary rules:

- Phase 7.6 may update documentation, tests, naming, ownership comments, and QML read-only exposure
  consistency only.
- `docs/PHASE_7_CHECKPOINT.md` is the durable checkpoint record for completed scope, limitations,
  guardrails, readiness criteria, and strict out-of-scope work.
- `ApplicationController` remains the C++ owner for runtime boundaries.
- `DesktopShellViewModel` remains the QML-safe read-only exposure layer for runtime metadata.
- The checkpoint must not add provider/model execution, provider calls, networking/API keys,
  downloads, streaming, subprocess/process launch, filesystem/system actions, real tools, plugin
  loading, sandbox runtime enforcement, embeddings, semantic search, autonomous workers, or
  execution/setup UI.

## 50. Execution Lifecycle Is Metadata-Only Coordination

Decision: Add `IExecutionLifecycle`, `StaticExecutionLifecycle`, `ExecutionCoordinator`, and
execution session metadata as a future execution lifecycle/session coordination layer.

Reason: Future provider/runtime integration needs a deterministic lifecycle vocabulary before any
execution path is enabled.

Boundary rules:

- Execution request, intent, priority, lifecycle state/status/result/trace, session ownership, and
  coordination snapshot values are metadata only.
- The lifecycle can describe requested, validating, permission-check, safety-check, coordination,
  ready-placeholder, and blocked states.
- The current lifecycle always ends blocked and non-executable.
- Invalid transitions are rejected safely and do not advance state.
- `ExecutionCoordinator` produces read-only snapshots only; it is not a scheduler or worker.
- `ApplicationController` owns lifecycle/coordinator interfaces.
- `DesktopShellViewModel` exposes only strings and string lists.
- Execution lifecycle remains separate from `ILocalRuntime`, `IChatProvider`, `IAgentRuntime`,
  `IToolExecutor`, provider adapters, tools, plugins, and platform services.
- This decision does not allow provider/model execution, Ollama launch, process/subprocess launch,
  filesystem/system actions, networking/API keys, downloads, streaming, tool/plugin execution,
  autonomous workers, timers/background loops, execution controls, or setup UI.

Future Phase 9 integration must add explicit provider/runtime interfaces, policy gates, tests, and
documentation before any executable path can exist.

## 51. Runtime Adapter And Provider Bridge Are Pre-integration Metadata

Decision: Add local runtime adapter, provider runtime bridge, and runtime integration readiness
contracts as metadata-only pre-integration boundaries.

Reason: Future Ollama/local runtime integration needs explicit adapter and bridge ownership before
any local provider implementation, endpoint configuration, model discovery, or execution path is
introduced.

Boundary rules:

- `ILocalRuntimeAdapter` describes adapter metadata only and does not connect to, call, launch, or
  probe a local runtime.
- `LocalRuntimeAdapterDescriptor` and capability summaries are descriptive placeholders.
- `IProviderRuntimeBridge` is not an `IChatProvider` implementation and does not generate responses.
- Provider bridge requests/responses are metadata only and report not connected/not executable.
- `StaticRuntimeIntegrationReadiness` produces ordered readiness checks from already-owned
  metadata only; it does not probe the system.
- Controller and QML exposure stays read-only and QML-safe: status strings, summaries, and string
  lists.
- Settings/Dashboard may show readiness metadata but must not add setup buttons, endpoint fields,
  model selection UI, or execution controls.
- This decision does not allow Ollama/OpenAI/Anthropic calls, API keys, networking, downloads,
  streaming, process/subprocess launch, filesystem/system actions, model discovery, model
  execution, real tools/plugins, embeddings/vector DB, or autonomous workers.

## 52. Ollama Health Boundary Is Local Read-Only

Decision: Add Ollama endpoint/config metadata and an `IOllamaRuntimeClient` boundary for local
health checks and installed-model metadata discovery only.

Reason: Phase 9 needs the first controlled real local-provider contact point, but chat inference,
model execution, downloads, process launch, and tooling remain too broad for this step.

Boundary rules:

- Default endpoint is `http://127.0.0.1:11434`.
- Endpoint normalization accepts only loopback HTTP endpoints and falls back to the default for
  invalid, cloud, non-loopback, query, fragment, or non-HTTP values.
- `AppSettings` may persist the normalized endpoint only; no API keys or credentials are stored.
- `NullOllamaRuntimeClient` is deterministic and unavailable.
- `OllamaHttpRuntimeClient` is injectable and may call only local Ollama read-only metadata APIs:
  `/api/version` for health and `/api/tags` for installed model summaries.
- The Ollama client is not `IChatProvider`, provider bridge execution, execution lifecycle, model
  router execution, agent runtime, tool executor, or plugin runtime.
- Controller and QML exposure stays read-only and QML-safe: endpoint, connection/health strings,
  summary, model count, and model summary strings.
- This decision does not allow prompt execution, model generation, streaming, model
  download/pull/delete/run, subprocess/process launch, cloud provider calls, API keys, tool/plugin
  execution, filesystem/system scans/actions, background workers, setup UI, model selection UI, or
  routing chat requests to Ollama.

Phase 9.3-9.5 adds that separately scoped, policy-gated, interface-owned,
injectable/testable inference boundary.

## 53. Local Inference Requires A Dedicated Boundary

Decision: Prompt execution for local models must go through `ILocalInferenceClient`, not
`IChatProvider`, `IProviderRuntimeBridge`, `IAgentRuntime`, or tool execution.

Reason: Phase 9.3-9.5 introduces the first executable local inference path, so prompt execution
needs a narrow owner that can be injected, tested without Ollama, and gated by existing
permission/safety metadata before any runtime call is made.

Boundary rules:

- Only local loopback HTTP Ollama endpoints are valid.
- No cloud endpoints, API keys, redirects, model pulls/downloads/deletes, subprocess launch,
  filesystem/system actions, tools/plugins, or autonomous loops are allowed.
- Streaming remains out of scope and is rejected as request metadata.
- Blank prompts and missing or unavailable models are rejected safely.
- `ApplicationController` records permission/safety and client traces before exposing summaries.
- QML receives strings and string lists only; raw requests, responses, clients, and traces are not
  exposed.

## 54. Selected Local Model Metadata Before Model Management

Decision: Persist a selected local model name as configuration metadata and validate it against
discovered local model metadata only when that metadata is already available.

Reason: Users need a stable local-model preference before chat routing is allowed to target local
inference, but selection must not imply model management or execution.

Boundary rules:

- Selected local model storage is a setting, not memory or chat history.
- No model download, pull, delete, install, or process launch is introduced.
- Known invalid selected models are rejected before local inference is invoked.
- Unknown discovery state is reported as metadata rather than treated as permission to manage
  models.
- QML receives summaries, strings, lists, and booleans only.

## 55. Streaming Boundary Before Streaming Execution

Decision: Add streaming value types and an interface while keeping the implementation disabled.

Reason: Streaming needs a separate contract from non-streaming inference so future token delivery
can be guarded and tested without destabilizing the existing `/api/generate` path.

Boundary rules:

- `LocalInferenceStreamChunk`, `LocalInferenceStreamStatus`, and stream result values are
  metadata/value objects.
- `ILocalInferenceStreamClient` is a boundary, not permission to stream.
- Current behavior is deterministic disabled status and no opened stream.
- No token streaming UI is required in this phase.
- Non-streaming inference remains permission/safety gated and stable.

## 56. Chat-To-Ollama Routing Is Explicit Opt-In

Decision: Chat may use local Ollama inference only when a persisted local chat inference setting is
explicitly enabled.

Reason: Phase 10.0-10.2 is the first chat path that can call local inference, so the default must
remain the deterministic local-safe provider path and every executable local request must stay
behind the existing boundary.

Boundary rules:

- The local chat inference setting defaults to disabled.
- Disabled chat uses the existing `IChatProvider` path.
- Enabled chat still requires a valid selected/effective local model, local loopback HTTP Ollama
  endpoint, runtime permission approval, and runtime safety compliance before invoking
  `runLocalInference`.
- User messages are appended before routing; exactly one assistant message is appended from either
  the inference result or a safe refusal/error.
- QML receives only a boolean setting plus routing status/summary strings.
- Streaming, model management/download/pull/delete UI, cloud provider routing, API keys,
  tools/plugins, filesystem/system actions, subprocess launch, and autonomous loops remain out of
  scope.

## 57. Streaming Chat Requires A Separate Local-Only Opt-In

Decision: Chat streaming uses `ILocalInferenceStreamClient` and a separate persisted local
streaming opt-in.

Reason: Streaming has different runtime behavior from non-streaming inference. It needs ordered
chunk metadata, live QML-safe text exposure, cancellation/error/refusal handling, and duplicate
message protection without weakening the existing local-only inference guards.

Boundary rules:

- Streaming defaults to disabled.
- Streaming may run only when local chat inference is enabled, streaming is enabled and available,
  the effective model is valid, the endpoint is local loopback HTTP, and permission/safety checks
  pass.
- `OllamaLocalInferenceStreamClient` may call only local loopback `/api/generate` streaming with
  manual redirect policy.
- No cloud endpoints, API keys, model downloads/pulls/deletes, subprocess launch,
  filesystem/system actions, tools/plugins, or autonomous loops are introduced.
- Malformed chunks are ignored with metadata; timeout, cancellation, refusal, and client errors
  produce safe summaries.
- Streaming chunks update live view-model text, but the final assistant message is appended and
  persisted once.

## 58. Model Selection UX Is Not Model Management

Decision: Settings may expose discovered local Ollama models for selection and status visibility,
but it must not expose model management actions.

Reason: Phase 10.6-10.8 improves the local runtime workflow now that explicit local inference,
model discovery, and guarded streaming exist. The UX should make installed-model state clear
without adding downloads, deletion, process ownership, cloud setup, or autonomous behavior.

Boundary rules:

- The selected local model remains a persisted setting.
- Settings can select only from already discovered local model names.
- Controller validation remains authoritative and rejects known invalid selected/effective models
  before inference or streaming clients are called.
- Model summaries are QML-safe strings containing name, size when available, modified date when
  available, and Local Only status.
- Runtime management UX is read-only/action-light apart from selected model and existing local
  chat/streaming toggles.
- No model downloads, pulls, deletes, installs, cloud providers, API keys, filesystem/system
  actions, subprocess launch, tools/plugins, autonomous loops, or broad model-management workflow
  are introduced.

## 59. Model Management Readiness Is Metadata-Only

Decision: Add `IModelManagementService` and a deterministic `StaticModelManagementService` for
readiness metadata, recommendations, requirements, and unavailable action results only.

Reason: The UI needs to prepare users for future local model management without quietly granting
Sentinel authority to download, delete, install, scan, or launch anything.

Boundary rules:

- Recommendations and RAM/disk requirements are static, approximate, descriptive metadata.
- Pull, delete, and install requests return unavailable/not implemented results.
- Controller and view-model exposure is limited to strings and string lists.
- Existing Ollama model discovery remains the only installed-model metadata source.
- No model downloads, pulls, deletes, installs, filesystem/system scans or actions, process launch,
  cloud calls, API keys, tools/plugins, autonomous loops, or broad setup workflow is introduced.

## 60. Local AI Runtime UX Must Finalize Through Chat History

Decision: Local inference and streaming UX may expose transient runtime state, but final assistant
responses must be represented by normal chat history messages.

Reason: Chat history is the durable transcript boundary. Live streaming text is useful while a
response is in progress, but keeping it after completion risks duplicate or stale assistant text in
QML and weakens transcript correctness.

Boundary rules:

- Live streaming text is QML-readable only while streaming is active.
- When streaming completes, the live preview is cleared and the accumulated final response is
  appended once through `ChatSession` and `IChatHistoryStore` when available.
- Refusals and errors are converted to safe user-facing summaries before they become assistant
  chat messages.
- Summaries may include technical categories such as missing model, invalid model, endpoint
  blocked, permission blocked, safety blocked, timeout, invalid response, and client unavailable.
- Summaries must not expose stack traces, secrets, filesystem paths, raw internal objects, provider
  credentials, or broad endpoint details.
- No real cancellation button, downloads, pulls, deletes, installs, cloud providers, API keys,
  filesystem/system actions, subprocess launch, tools/plugins, or autonomous loops are introduced.

## 61. Phase 11 Local AI Checkpoint Is Runtime QA Only

Decision: Close Phase 11 with a local AI usability and runtime QA checkpoint instead of adding new
local AI features.

Reason: The current local Ollama path now has opt-in chat inference, opt-in streaming, model
selection, runtime summaries, safe refusal/error states, and fake-client testability. Before Phase
12, the priority is confirming that these paths stay bounded and predictable.

Boundary rules:

- Checkpoint work may add focused tests, documentation, safe wording, and QML-safe exposure checks.
- Checkpoint work must preserve existing local chat inference, streaming, model validation,
  endpoint, permission, and safety gates.
- Selected model, local chat inference, and streaming settings remain settings persistence only.
- Streaming live-preview text must clear after completion, refusal, cancellation, or error; final
  assistant output remains a normal chat history message.
- QML must not expose raw local inference clients, stream clients, requests, responses, runtime
  policies, Ollama internals, model-management service objects, stores, paths, secrets, or
  credentials.
- No new product features, model pull/delete/install, cloud providers/API keys,
  filesystem/system actions, subprocess launch, tools/plugins, UI redesign, or autonomous behavior
  are introduced by the checkpoint.

## 62. Voice Starts As A Disabled Provider Boundary

Decision: Add text-to-speech and speech-to-text provider interfaces with deterministic null
providers before adding any real voice runtime.

Reason: Voice affects microphones, speakers, local binaries, model files, permissions, privacy, and
runtime lifecycle. Sentinel needs a testable architecture boundary and UI readiness surface before
any audio I/O or Piper/Whisper integration is allowed.

Boundary rules:

- `ITextToSpeechProvider` and `ISpeechToTextProvider` own future voice provider behavior.
- Current `NullTextToSpeechProvider` and `NullSpeechToTextProvider` implementations are disabled
  and return safe refusals.
- `VoiceCapability`, `VoiceProviderDescriptor`, `VoiceProviderStatus`, `VoiceRuntimeMode`,
  `VoiceRequest`, `VoiceResponse`, and `VoiceReadinessReport` are value-only metadata.
- Controller and view-model exposure is limited to strings, string lists, and booleans.
- Settings may display read-only voice readiness metadata only.
- Future Piper integration must stay behind `ITextToSpeechProvider`.
- Future Whisper integration must stay behind `ISpeechToTextProvider`.
- No microphone access, audio playback, recording, synthesis, transcription, subprocess/process
  launch, filesystem/system action, download, cloud call, API key, voice button, record button,
  speak button, or broad UI redesign is introduced.

## 63. Voice Runtime Coordination Is Metadata-Only

Decision: Add voice session and pipeline orchestration as deterministic metadata before any audio
runtime is allowed.

Reason: Voice runtime work needs session ownership, pipeline state, trace visibility, and readiness
reporting before Sentinel can safely define microphone capture, playback, Piper, Whisper, process
ownership, or cancellation behavior.

Boundary rules:

- `VoiceSession`, `VoiceSessionId`, `VoiceSessionState`, `VoicePipelineStage`,
  `VoicePipelineStatus`, and `VoicePipelineTrace` are value-only metadata.
- `IVoiceRuntimeCoordinator` owns future voice runtime/session coordination.
- `StaticVoiceRuntimeCoordinator` emits deterministic idle, preparing, awaiting-input,
  transcribing-placeholder, inference-placeholder, synthesis-placeholder, completed, blocked, and
  error metadata.
- Runtime summaries explicitly report runtime unavailable, TTS unavailable, STT unavailable,
  microphone disabled, playback disabled, local-only policy active, and process execution disabled.
- Controller and view-model exposure remains limited to strings, string lists, and booleans.
- Settings may display read-only voice session/runtime/pipeline metadata only.
- Future Piper integration must stay behind `ITextToSpeechProvider` and the runtime coordinator
  boundary.
- Future Whisper integration must stay behind `ISpeechToTextProvider` and the runtime coordinator
  boundary.
- No microphone access, audio playback, recording, synthesis, transcription, subprocess/process
  launch, filesystem/system action, download, cloud call, API key, voice activation, autonomous
  loop, or broad UI redesign is introduced.

## 64. Phase 12 Voice Checkpoint Plans Integration Before Execution

Decision: Close Phase 12 with a voice architecture checkpoint and local Piper/Whisper integration
plan instead of adding real audio behavior.

Reason: The provider boundaries and runtime/session metadata are now in place. Before Phase 13 can
add any executable local voice capability, Sentinel needs a durable record of ownership,
limitations, safety guardrails, and readiness criteria.

Boundary rules:

- Checkpoint work may update documentation, clarify architecture decisions, and confirm existing
  test coverage.
- `ITextToSpeechProvider`, `ISpeechToTextProvider`, and `IVoiceRuntimeCoordinator` remain the
  required boundaries for any future Piper, Whisper, playback, capture, or voice session work.
- `ApplicationController` owns injected/default voice providers and the runtime coordinator.
- `DesktopShellViewModel` and QML remain limited to read-only strings, string lists, and booleans.
- Future Piper work must define local binary ownership, model path ownership, playback lifecycle,
  cancellation, permission prompts, and safety checks before execution.
- Future Whisper work must define microphone permission, capture lifecycle, local binary/model
  ownership, transcription privacy, cancellation, and error handling before execution.
- No microphone access, audio playback, Piper execution, Whisper execution, subprocess/process
  launch, filesystem/system action, download, cloud call, API key, voice button, activation flow,
  autonomous voice loop, or broad UI redesign is introduced by the checkpoint.

## 65. Local Voice Runtime Environment Owns Binary And Model Readiness Before Execution

Decision: Add a metadata-only voice runtime environment boundary for future local Piper/Whisper
binary, model, permission, and safety ownership before any execution is allowed.

Reason: Piper and Whisper integration requires local binary paths, model paths, permission gates,
and runtime safety checks. These must be visible and testable before Sentinel can safely execute
voice binaries or touch audio devices.

Boundary rules:

- `VoiceBinaryDescriptor` and `VoiceModelDescriptor` describe expected future Piper/Whisper binary
  and model ownership only.
- `VoiceRuntimePermission` describes denied/default-off microphone, playback, process execution,
  and model-read posture as metadata.
- `VoiceRuntimeSafetyReport` blocks execution by default and reports no microphone, playback,
  process execution, filesystem-wide scan, download, cloud call, or API-key behavior.
- `IVoiceRuntimeEnvironment` is separate from `ITextToSpeechProvider`,
  `ISpeechToTextProvider`, and `IVoiceRuntimeCoordinator`.
- `NullVoiceRuntimeEnvironment` and `StaticVoiceRuntimeEnvironment` are deterministic and
  non-operational.
- Controller and view-model exposure remains limited to strings, string lists, and booleans.
- Settings may display read-only environment, binary, model, permission, and safety metadata only.
- No microphone access, audio playback, Piper execution, Whisper execution, subprocess/process
  launch, filesystem-wide scan, model loading, download, cloud call, API key, setup button, path
  picker, or broad UI redesign is introduced.

## 66. Piper TTS Adapter Starts As A Non-Executable Provider Boundary

Decision: Add a Piper text-to-speech adapter skeleton behind the voice provider boundary without
enabling audio playback, file-output synthesis, or Piper subprocess execution.

Reason: Piper integration needs typed request/result/configuration metadata, binary/model readiness
checks, and deterministic refusal behavior before Sentinel can safely define any executable local
TTS flow.

Boundary rules:

- `PiperTtsConfig`, `PiperVoiceModelDescriptor`, `PiperTtsRequest`, `PiperTtsResult`, and
  `PiperTtsStatus` are value-only Piper TTS metadata.
- `IPiperTtsClient` is the future low-level client boundary, and `NullPiperTtsClient` is the
  default deterministic non-operational implementation.
- `PiperTextToSpeechProvider` stays behind `ITextToSpeechProvider` and reports status/readiness
  metadata only.
- The default Piper adapter is disabled/not configured.
- Missing Piper binary or voice model metadata causes deterministic refusal before any client
  boundary is reached.
- The sketched execution path remains non-callable by default; even metadata-ready requests refuse
  until a later explicit phase defines controlled file-output synthesis.
- Controller and view-model exposure remains limited to QML-safe strings, string lists, and
  booleans.
- Settings may display read-only Piper readiness only.
- No audio playback, microphone access, Whisper/STT, Piper execution, subprocess/process launch,
  model loading, download, cloud/API-key behavior, filesystem-wide scan, speak button, model/path
  picker, or broad UI redesign is introduced.

## 67. Piper TTS File Output Is Explicit, Local, And Controlled

Decision: Add controlled Piper text-to-audio file output behind `ITextToSpeechProvider` and
`IPiperTtsClient`, while keeping Piper disabled by default and keeping playback/microphone
behavior out of scope.

Reason: Sentinel can safely advance Piper integration only by separating synthesis-to-file from
playback, requiring explicit local policy gates, and keeping test execution injectable without a
real Piper binary or voice model.

Boundary rules:

- The default Piper configuration remains disabled/not configured.
- File-output synthesis requires an enabled Piper config, an existing executable Piper binary, an
  existing readable voice model, an app-controlled output directory, explicit process execution
  permission in request/config/safety policy, a local-only request, playback disabled, microphone
  disabled, downloads blocked, cloud/API-key behavior blocked, and filesystem-wide scans blocked.
- Output files are generated only inside the configured controlled cache/temp output directory.
- Historical note: this decision originally allowed a process-backed Piper file-output client.
  Decision 98 supersedes current behavior with a non-executing Piper synthesis boundary.
- Results report configured, refused, missing, safety-blocked, succeeded, failed, and timeout
  metadata with safe output path summaries, timeout values, exit code, and error summaries.
- Controller and view-model exposure remains QML-safe strings, string lists, and booleans.
- Settings may display Piper configured/missing/file-output readiness only.
- No automatic playback, microphone access, Whisper/STT, model download, cloud/API-key behavior,
  broad filesystem scan, autonomous voice loop, speak/play button, path picker, model picker, or
  broad UI redesign is introduced.

## 68. Phase 13 Voice/Piper Checkpoint Preserves The Safety Boundary

Decision: Close Phase 13 with a Voice/Piper checkpoint and readiness review before adding any
voice setup, playback, STT, or conversation-loop behavior.

Reason: Phase 13 introduced Piper ownership metadata, a provider/client boundary, and controlled
file-output synthesis. The project needs a durable review of the resulting safety boundary before
Phase 14 work can choose whether to focus on configuration UX, playback, Whisper, or voice-loop
planning.

Boundary rules:

- Checkpoint work may update documentation and confirm existing architecture/test coverage only.
- The current TTS path is `text -> Piper provider -> gated file-output metadata`.
- Piper remains disabled by default.
- File output remains reachable only after explicit enabled configuration, local-only request
  posture, process permission, executable binary, readable model, controlled output path, playback
  disabled, microphone disabled, no downloads, no cloud/API-key behavior, no filesystem-wide
  scans, and safety approval gates pass.
- `PiperTextToSpeechProvider` remains behind `ITextToSpeechProvider`; current Piper synthesis
  readiness is now owned by `IPiperSynthesisClient` per Decision 98.
- `ISpeechToTextProvider` remains a null-provider boundary; no Whisper adapter or execution is
  introduced.
- Controller and view-model exposure remains limited to QML-safe strings, string lists, and
  booleans.
- Settings remains read-only for voice/Piper readiness and does not expose setup, path/model
  picker, speak, play, record, or activation controls.
- No new runtime behavior, audio playback, microphone access, Whisper execution, downloads,
  cloud/API-key behavior, autonomous voice loop, voice conversation loop, or broad UI redesign is
  introduced by the checkpoint.

## 69. Local Voice Configuration Is Persisted Metadata, Not Execution

Decision: Store Piper and Whisper binary/model path configuration in settings and validate only
the exact configured paths as metadata.

Reason: Users need a visible setup surface before a future voice runtime phase, but path storage
must not grant Sentinel authority to run local voice binaries, open audio devices, download models,
or scan the filesystem broadly.

Boundary rules:

- `AppSettings` owns persisted strings for Piper binary path, Piper model path, Whisper binary
  path, and Whisper model directory/path.
- `ApplicationController` derives readiness summaries from those strings using exact-path
  metadata checks: configured/missing, exists/missing, readable/unreadable, and
  executable/non-executable for binaries.
- `DesktopShellViewModel` exposes QML-safe strings and string lists only.
- Settings may provide compact text entry for paths and readiness summaries.
- Piper readiness may reflect configured metadata, but Piper execution remains behind existing
  provider/client safety gates and remains blocked by default.
- Whisper remains configuration metadata only; no STT adapter or execution path is added.
- No Piper execution, Whisper execution, microphone access, playback, downloads,
  filesystem-wide scans, cloud/API keys, autonomous loops, or path picker integration are added in
  this decision.

## 70. Voice Auto-Detection Is Hint-Only And Non-Invasive

Decision: Expose Piper and Whisper path hints as read-only Settings suggestions, not automatic
configuration or runtime probing.

Reason: Users benefit from common-location hints, but Sentinel must not expand configuration UX
into filesystem discovery, binary execution, downloads, or automatic setup.

Boundary rules:

- Binary hints may check only fixed known locations: `/opt/homebrew/bin/piper`,
  `/usr/local/bin/piper`, `/opt/homebrew/bin/whisper`, and `/usr/local/bin/whisper`.
- Model hints may validate only already configured model paths for readability.
- Hints are QML-safe strings/string lists and are never applied to settings automatically.
- Status badges may report configured/missing, valid/missing, readable/unreadable, and
  executable/non-executable metadata only.
- Settings may show concise help text, compact badges, and short hint rows.
- No Piper execution, Whisper execution, microphone access, playback, downloads,
  filesystem-wide scans, recursive model discovery, cloud/API keys, autonomous loops, path
  pickers, or automatic settings writes are added in this decision.

## 71. Local Ollama Chat Activation Is Narrow And Explicit

Decision: Activate real local Ollama inference only for explicit user chat requests and only
through the existing local runtime/inference interfaces.

Reason: Phase 14.7-15.0 is the first phase where desktop chat may produce real model responses.
That activation must stay narrower than provider routing, agent execution, tool execution, voice
runtime work, or model management.

Boundary rules:

- The desktop app uses the persisted Ollama endpoint after loopback-only normalization.
- Health checks are limited to local loopback HTTP `/api/version`.
- Model discovery is limited to local loopback HTTP `/api/tags`.
- Inference is limited to local loopback HTTP `/api/generate` for an explicitly selected or
  safely resolved local model.
- `LocalOnlyRuntimePermissionPolicy` allows only `LocalInference` execution and denies provider
  invocation, tool invocation, external process, filesystem access, broader network access, and
  plugin invocation.
- `ApplicationController::sendMessage` remains the only chat path that may invoke local inference,
  and only when local chat inference is enabled by settings.
- QML receives only QML-safe strings, string lists, booleans, and chat models from
  `DesktopShellViewModel`.
- No cloud/API keys/providers, autonomous agents, tools, shell execution, filesystem-wide actions,
  model downloads/pulls/deletes, microphone access, playback, Piper execution, Whisper execution,
  or autonomous voice loop is added by this activation.

## 72. Voice Path Readiness Is Explicit But Non-Executable

Decision: Refine Piper and Whisper path setup with explicit Apply Paths persistence,
Ready/Blocked/Missing readiness, and exact path validation while keeping Piper and Whisper
execution disabled.

Reason: Users need to understand whether their local voice paths are usable before a later runtime
phase. That feedback should be immediate and specific without turning configuration into process
launch, model loading, audio I/O, downloads, or filesystem discovery.

Boundary rules:

- Settings may expose an Apply Paths action that stores only the entered path strings through
  `AppSettings`.
- `ApplicationController` validates only exact configured paths.
- Piper file-output TTS preparation is Ready only when the Piper binary exists as an executable
  file and the Piper `.onnx` model path exists as a readable file.
- Whisper STT preparation is Ready only when the Whisper binary exists as an executable file and
  the Whisper model folder or model file exists and is readable.
- Blocked readiness must include the exact failed path checks.
- Piper provider/client execution remains behind the existing disabled-by-default safety gates.
- Whisper remains configuration metadata only; no STT adapter or execution path is added.
- No Piper execution, Whisper execution, microphone access, playback, downloads,
  filesystem-wide scans, cloud/API keys, autonomous loops, voice action controls, or automatic
  hint application are added in this decision.

## 73. Controlled Piper File Output Requires Persisted Opt-In And Explicit Action

Status: Superseded for current desktop behavior by Decision 98. The current Piper TTS runtime is
readiness-only and refuses before subprocess execution, file output, or playback.

Decision: Enable Piper file-output execution only behind a persisted user opt-in, exact configured
path readiness, provider safety gates, and an explicit Generate TTS File action.

Reason: Piper execution is the first local voice subprocess path. It needs a narrow product and
architecture boundary before any playback, microphone, Whisper, downloads, or autonomous voice
loop work can be considered.

Boundary rules:

- Piper file-output execution is disabled by default and persisted separately from path strings.
- Historical rule superseded by Decision 98: the current controller does not call a Piper process
  client and file generation requests refuse before subprocess execution.
- The configured Piper binary path must exist as an executable file.
- The configured Piper `.onnx` model path must exist as a readable file.
- The output path is generated by the provider inside the app-controlled cache/temp output
  directory. Users cannot supply arbitrary output paths.
- Status metadata must stay QML-safe and report disabled, blocked/safety-blocked, missing binary,
  missing model, running, succeeded, failed, and timeout states plus generated file path summary
  metadata when available.
- Tests should use fake Piper clients for success, failure, and timeout behavior; no real Piper
  binary or model is required for test success.
- No playback, microphone access, Whisper execution, downloads, cloud/API keys,
  filesystem-wide scans, autonomous voice loops, raw process internals, or background voice actions
  are added in this decision.

## 74. Local Ollama Failures Must Be Categorized And Bounded

Decision: Local Ollama chat reliability is part of the controlled local inference boundary, not a
new provider/runtime authority expansion.

Reason: Users need deterministic failures when Ollama is stopped, unreachable, slow, missing a
model, returning malformed data, or interrupting a stream. Silent fallthrough makes the local
runtime path hard to debug and can leave chat UI in an ambiguous state.

Boundary rules:

- Health checks, model discovery, non-streaming generation, and streaming generation carry explicit
  timeout metadata.
- Failure summaries must stay QML-safe and must not expose secrets, raw internals, stack traces,
  broad endpoint details, filesystem paths, or credentials.
- Duplicate sends while local inference is active are rejected before appending another user
  message.
- Failed streams clear live preview text and never persist partial assistant output as the final
  assistant message.
- Successful streaming and non-streaming local inference append one assistant message only.
- Permission and safety denials remain explicit categories and still occur before model execution.
- No cloud providers, API keys, model downloads/pulls/deletes, Ollama process management, tools,
  plugins, filesystem/system actions, autonomous loops, playback, microphone access, Piper
  behavior changes, or Whisper execution are added by this reliability decision.

## 75. Local Inference Uses An Async Worker Boundary

Decision: Real local Ollama generation and streaming must run behind `ILocalInferenceWorker`
instead of directly inside `ApplicationController`.

Reason: Ollama model loading and generation can be slow even with bounded timeouts. The controller
and QML thread should own policy, state, and chat finalization, while blocking network waits stay
behind a worker boundary.

Boundary rules:

- `ApplicationController` still performs local chat opt-in, model validation, loopback endpoint
  validation, runtime permission evaluation, runtime safety evaluation, and busy duplicate-send
  rejection before a worker request starts.
- Worker results are accepted only when their request id matches the active controller request.
  Stale results are ignored.
- Successful non-streaming and streaming requests append exactly one assistant message.
- Failed requests do not persist partial assistant output, and streaming preview text is cleared
  on completion, error, or cancellation.
- Cancellation is currently metadata/request-id invalidation only; it does not promise immediate
  interruption of an in-flight Ollama HTTP request beyond existing client timeouts.
- The worker boundary does not add cloud providers, API keys, model downloads/pulls/deletes,
  Ollama process management, tools, plugins, filesystem/system actions, Piper changes, Whisper
  execution, microphone access, playback, or autonomous loops.

## 76. Local Memory Recall Is Literal And Read-Only

Decision: Memory recall starts as deterministic metadata over the existing local key-value memory
store.

Reason: Committed memory now exists, but using it must remain user-visible and bounded before any
semantic recall or prompt-context assembly is introduced.

Boundary rules:

- Recall reads only `IMemoryStore::entries()`.
- Matching is literal key/value matching only.
- Empty queries return a safe empty-query summary.
- Recall must not call providers/models, build embeddings, create a vector DB, use semantic
  search, load tools/plugins, or perform filesystem/system actions.
- Recall must not mutate memory, candidates, chat history, conversation state, or runtime state.
- Recall results are QML-safe strings/counts/lists only and are not automatically injected into
  chat prompts.
- Semantic recall, ranking, embeddings/vector indexing, prompt injection, and automatic context
  assembly require a later explicit phase.

## 77. Context Assembly Starts As Planning Metadata Only

Decision: Context assembly begins as a deterministic readiness/read-model layer, not as prompt
construction.

Reason: Sentinel now has conversation transcripts, committed memory, runtime metadata, and
orchestration metadata, but joining them into prompts would create a new execution boundary. The
Desktop Alpha needs visible planning metadata first.

Boundary rules:

- Context assembly planning may estimate participation for conversation context, committed
  key-value memory context, runtime metadata context, and orchestration metadata context.
- Planning may expose source availability, source counts, candidate block counts, source
  summaries, and simple character-size estimates.
- Planning must not build a final LLM prompt, inject context into chat, automatically attach
  context, call providers/models, run tools/plugins, or perform filesystem/system actions.
- Planning must not mutate chat messages, memory entries, memory candidates, runtime state, or
  orchestration state.
- QML receives only safe strings, counts, and lists through the controller/view-model boundary.
- Semantic ranking, embeddings/vector indexes, prompt assembly, and automatic context attachment
  require a later explicit phase.

## 78. Prompt Context Injection Is Local, Bounded, And Opt-In

Decision: Prompt context injection may prepend explicitly assembled local context to local Ollama
requests only when the user/app setting is enabled.

Reason: Sentinel now has committed local memory and context planning metadata, but prompt assembly
can expose private data to a model. It therefore needs a deterministic, visible, local-only policy
boundary before richer recall exists.

Boundary rules:

- Injection is disabled by default and controlled by a persisted explicit setting.
- Injection runs only after existing local inference busy, model, endpoint, permission, and safety
  gates pass.
- Allowed sources are current conversation context, committed key-value memory, runtime metadata
  summaries, and orchestration metadata summaries.
- Pending, rejected, archived, and merely approved memory candidates are never injected unless they
  have been explicitly committed into `IMemoryStore`.
- The assembled context block must be clearly delimited and budgeted by character count.
- Truncation order is deterministic: conversation, committed memory, runtime metadata, then
  orchestration metadata.
- UI exposure is summary-only: status, block count, source summary, size summary, and block
  summaries. The raw assembled private prompt is not exposed to QML.
- Injection must not mutate memory, memory candidates, chat history, runtime metadata, or
  orchestration metadata.
- No embeddings, vector database, semantic ranking/search, automatic memory writes, cloud/API-key
  behavior, tools/plugins, filesystem/system actions, or voice/runtime scope changes are added.

## 79. Conversation History Uses A Deterministic Recent Window

Decision: Prompt context injection must include conversation history through a bounded recent
message window instead of injecting the full active transcript.

Reason: Long local conversations can exceed useful prompt budgets. Sentinel needs predictable
context behavior before semantic recall, summarization, or provider-specific context management is
introduced.

Boundary rules:

- Windowing uses `ConversationWindowPolicy`, `ConversationWindowBudget`,
  `ConversationWindowSummary`, `ConversationWindowResult`, and `ConversationWindowStatus`.
- Recent messages are prioritized for inclusion, but included messages are emitted in their
  original chronological order.
- Truncation is deterministic and character-budget based only.
- The current user prompt is not duplicated inside the conversation-history window; it remains in
  the existing prompt section.
- Conversation history, committed memory, runtime metadata, and orchestration metadata remain
  separate delimited prompt-context blocks.
- UI exposure is summary-only: status, budget summary, included count, omitted count, and
  truncated count.
- Window assembly must not mutate chat history, memory, memory candidates, runtime state, or
  orchestration metadata.
- No semantic ranking/search, embeddings, vector database, transcript summarization, cloud/API-key
  behavior, provider/model change, tools/plugins, filesystem/system actions, or broad UI redesign
  is added.

## 80. Conversation Summaries Are Deterministic Local Compaction

Decision: Older conversation context may be represented by bounded deterministic summary blocks
only after the recent conversation window has selected its messages.

Reason: Long chats need a local bridge between recent-window context and future semantic
summarization. That bridge must preserve chronology and source separation without adding provider
calls or memory writes.

Boundary rules:

- Summary metadata uses `ConversationSummaryPolicy`, `ConversationSummaryStatus`,
  `ConversationSummaryResult`, `ConversationSummaryBlock`, `ConversationSummaryWindow`, and
  `ConversationSummaryBudget`.
- Summary candidates are messages omitted by the recent conversation window.
- Blocks preserve original message indexes, chronological order, and visible roles.
- Summary text is heuristic/local compaction only; it is not semantic summarization.
- Recent conversation history, older conversation summaries, committed key-value memory, runtime
  metadata, and orchestration metadata remain separate prompt-context blocks.
- Summary budgeting is deterministic character budgeting with included size, block count,
  summarized/omitted message count, and truncated-block metadata.
- Summary planning and assembly must not mutate chat history, memory, memory candidates, runtime
  state, or orchestration metadata.
- Semantic summarization, embeddings/vector DB, semantic ranking/search, provider/model calls,
  cloud/API keys, tools/plugins, filesystem/system actions, automatic memory writes, and broad UI
  redesign require later explicit phase gates.

## 81. Retrieval Planning Is Deterministic Source Selection

Decision: Prompt context sources pass through deterministic retrieval planning before injection.

Reason: Sentinel now has several local context sources. A planning layer is needed to keep source
priority, budget allocation, and UI-visible readiness deterministic before future semantic
retrieval exists.

Boundary rules:

- Retrieval planning uses `RetrievalPlanningPolicy`, `RetrievalPlanningStatus`,
  `RetrievalPlanningResult`, `RetrievalCandidate`, `RetrievalSourcePriority`, `RetrievalBudget`,
  and `RetrievalSelectionSummary`.
- Priority order is fixed: recent conversation window, deterministic conversation summaries,
  committed key-value memory, runtime metadata, then orchestration metadata.
- Planning allocates a character budget in priority order and truncates deterministically.
- Source separation is preserved. Conversation chronology remains preserved inside the recent
  window and summary blocks before retrieval planning selects those blocks.
- Planning exposes selected/excluded source counts, selected/excluded candidate counts, truncated
  counts, readiness, budget summaries, and source summaries only.
- Planning must not mutate prompts, chat history, committed memory, memory candidates, runtime
  metadata, or orchestration metadata.
- The prompt context injection step may consume selected retrieval candidates, but QML must not
  receive the raw assembled prompt or private payload.
- Semantic retrieval, vector databases, embeddings, semantic ranking/search, provider/model calls,
  cloud/API keys, automatic memory writes, filesystem/system actions, tools/plugins, and broad UI
  redesign require later explicit phase gates. Future vector/embedding compatibility must remain
  behind a separate retrieval/ranking boundary and keep this deterministic planning contract
  intact.

## 81.1 Memory Relevance Is Deterministic Metadata

Decision: Committed key-value memory uses a local deterministic relevance layer before becoming
prompt context.

Reason: Context injection needs better memory quality and explainability without granting
semantic/vector authority or adding hidden background behavior.

Boundary rules:

- Memory relevance uses value-only records: `MemoryRelevancePolicy`,
  `MemoryRelevanceCandidate`, `MemoryRelevanceScore`, `MemoryRelevanceReason`,
  `MemoryRelevanceBudget`, `MemoryRelevanceSelection`, `MemoryRelevanceTrace`, and
  `MemoryRelevanceSummary`.
- Scoring is deterministic and limited to literal key overlap, literal value overlap, active
  conversation title overlap, recent conversation terms, and explicit pinned/committed memory
  priority metadata.
- Selected memories remain key-value memory context candidates. Duplicate suppression, character
  budgeting, candidate limits, stable tie ordering, included/excluded counts, and exclusion
  reasons are exposed as QML-safe summaries.
- Prompt context injection remains opt-in and still runs through existing local inference gates.
- This layer must not add embeddings, semantic/vector ranking authority, cloud/API calls,
  filesystem indexing, background summarization, autonomous memory writes, prompt debug dumps, raw
  hidden prompt exposure, tools/plugins, or voice authority.

## 82. Embedding And Vector Support Starts As Interfaces

Decision: Semantic/vector retrieval begins with disabled abstraction boundaries, not real semantic
runtime behavior.

Reason: Future semantic retrieval needs clean provider and index compatibility without changing
the current deterministic prompt pipeline or expanding runtime authority.

Boundary rules:

- `IEmbeddingProvider` owns future embedding generation behind an interface.
- `IVectorIndex` owns future vector insert/search/remove behavior behind an interface.
- `EmbeddingVector`, `EmbeddingDocument`, `EmbeddingRequest`, `EmbeddingResult`,
  `EmbeddingProviderStatus`, `EmbeddingProviderPolicy`, `VectorIndexStatus`,
  `VectorIndexPolicy`, `VectorSearchQuery`, `VectorSearchResult`, `VectorSearchCandidate`,
  `SemanticRetrievalStatus`, and `SemanticRetrievalPolicy` are value/interface records only.
- The desktop runtime exposes semantic/vector readiness metadata but does not configure a real
  embedding provider or vector index.
- `FakeEmbeddingProvider` and `FakeVectorIndex` are deterministic local test fakes only. They do
  not call providers/models, use cloud/API keys, write files, download assets, load plugins, or
  execute tools/system actions.
- Fake embeddings use stable text hashing/token counting. Fake vector search is in-memory only and
  uses deterministic scoring and stable tie ordering.
- Retrieval planning and prompt assembly remain unchanged. Semantic readiness metadata must not
  affect source priority, ranking, selected context, prompt injection, or raw prompt exposure.
- QML receives only safe strings, counts, booleans, and checks. Raw vectors, scores, providers,
  index handles, and private prompt payloads are not exposed.

## 83. Semantic Activation Requires A Separate Phase Gate

Decision: Phase 16 closes with semantic retrieval disabled and deterministic retrieval
authoritative.

Reason: The architecture now has memory candidates, committed memory recall, opt-in context
injection, retrieval planning, embedding/vector interfaces, and semantic candidate orchestration.
Activating semantic retrieval would change prompt authority and therefore needs its own scoped
phase, implementation, and tests.

Boundary rules:

- Deterministic retrieval planning remains the authoritative prompt-context selector.
- Semantic candidate orchestration is readiness metadata only and must not create
  `PromptContextBlock` values or mutate prompts.
- Runtime embedding provider and vector index readiness remain Not Configured until a later phase
  wires concrete implementations behind `IEmbeddingProvider` and `IVectorIndex`.
- Committed key-value memory remains authoritative. Pending, rejected, archived, and merely
  approved candidates must not participate in recall or prompt injection.
- Prompt context injection remains opt-in and runs only after existing local inference gates.
- QML exposure remains limited to safe summaries, statuses, counts, booleans, string lists, and
  readiness checks.
- Raw vectors, scores, provider/index handles, private prompt payloads, and raw semantic candidate
  content must not be exposed to QML.
- Phase 17 semantic activation requires explicit indexing policy, deterministic fallback behavior,
  privacy/safety gates, and tests proving no cloud/API-key, tool/plugin, filesystem/system, or
  unauthorized prompt-injection authority is introduced.

## 84. Phase 17 Starts With Provider Selection Metadata

Decision: Semantic activation planning begins with local provider/index selection metadata and
safety gates, while real semantic retrieval remains disabled.

Reason: The next risk is not ranking quality; it is choosing which local semantic provider/index
path can become active without weakening deterministic retrieval authority, prompt safety, or QML
exposure boundaries.

Boundary rules:

- Provider planning uses `SemanticProviderDescriptor`, `SemanticProviderSelection`,
  `SemanticProviderReadiness`, `SemanticProviderHealth`, `SemanticProviderCapability`,
  `SemanticProviderPolicy`, `SemanticActivationReadiness`, and `SemanticActivationResult`.
- Supported planned modes are Disabled, Fake/InMemory test provider, Local Ollama embeddings
  provider, and Local file/vector index.
- Disabled is the desktop default. Activation readiness refuses by default.
- Local Ollama embeddings and local file/vector index are planned-only until a later explicit
  phase enables real embedding calls or vector writes.
- Fake/InMemory remains an isolated deterministic test path and must not feed prompt assembly.
- Deterministic retrieval remains authoritative, and current prompt assembly remains unchanged.
- No cloud/API keys, downloads, filesystem scans, tool/plugin actions, system actions, vector
  database writes, semantic prompt injection, or prompt mutation are authorized by provider
  selection metadata.
- QML may show selected provider name/mode, readiness, health, capabilities, activation
  readiness, and required steps only. Raw config paths, provider handles, index handles, vectors,
  scores, and prompt payloads remain hidden.

## 85. Semantic Arbitration Simulation Before Semantic Retrieval

Decision: Add deterministic semantic arbitration simulation and embedding runtime planning before
any real semantic retrieval activation.

Reason: Hybrid retrieval needs predictable arbitration, cost/readiness vocabulary, and prompt
authority guarantees before semantic ranking can safely influence context selection.

Boundary rules:

- `SemanticArbitrationPolicy`, `SemanticArbitrationStatus`, `SemanticArbitrationResult`,
  `SemanticCandidateScore`, `SemanticBudgetSummary`, `EmbeddingRuntimePlan`,
  `EmbeddingRuntimeBudget`, and `EmbeddingRuntimeReadiness` are metadata-only records.
- Simulated scores are deterministic local metadata. Ordering uses simulated score, source rank,
  and candidate id for tie handling.
- Deterministic retrieval planning remains authoritative. Semantic simulation cannot create
  `PromptContextBlock` values, change `RetrievalPlanningResult`, or mutate prompts.
- Embedding runtime planning may estimate future local cost and requirements only. It must not
  call Ollama embeddings, generate vectors, write vector indexes, scan filesystems, download
  models, use cloud/API keys, or execute providers/tools/plugins.
- QML may show compact readiness, budget, checks, requirements, and selection summaries only. Raw
  vectors, raw score payloads, provider/index handles, paths, prompt payloads, and activation
  controls remain hidden.

## 86. Local Vector Persistence Does Not Grant Retrieval Authority

Decision: Add local vector persistence lifecycle metadata and a bounded local foundation while
keeping semantic retrieval disabled and deterministic retrieval authoritative.

Reason: A future semantic path needs explicit index lifecycle vocabulary before any index can
participate in retrieval. The lifecycle can be tested safely now if it accepts only isolated
embedding runtime output metadata and remains disconnected from ranking and prompt assembly.

Boundary rules:

- `VectorPersistencePolicy`, `VectorPersistenceStatus`, `VectorPersistenceHealth`,
  `VectorPersistenceReadiness`, `VectorPersistenceSession`, `VectorPersistenceBudget`,
  `VectorPersistenceResult`, `VectorIndexLifecycle`, and `VectorIndexSnapshotSummary` describe
  local lifecycle state only.
- Vector persistence is disabled by default in the desktop runtime and remains local-only,
  deterministic, bounded, and isolated.
- Lifecycle operations are explicit create, reset, and clear. There is no automatic indexing,
  filesystem scanning, background ingestion, semantic retrieval authority, prompt mutation,
  automatic memory conversion, cloud/API provider, or external vector service.
- The accept path may consume only successful isolated embedding runtime output metadata plus
  deterministic local summaries. Raw vectors are not exposed to QML and do not alter retrieval
  planning, ranking, prompt context injection, or memory storage.
- Empty indexes, stale sessions, busy sessions, and bounded-limit overflow are safe deterministic
  outcomes.
- Future semantic retrieval activation still requires a separate phase gate for indexing policy,
  vector storage migration/refresh, fallback behavior, privacy/safety checks, prompt authority,
  and QML non-exposure tests.

## 88. Controlled Semantic Search Is Non-authoritative

Decision: Activate only bounded local semantic candidate search over local vector persistence
entries, and keep deterministic retrieval as the only prompt authority.

Reason: Hybrid orchestration needs real search lifecycle signals before semantic retrieval can be
considered for future authority. That validation can be done safely if semantic search returns
metadata only and cannot affect retrieval planning or prompt assembly.

Boundary rules:

- `SemanticSearchPolicy`, `SemanticSearchStatus`, `SemanticSearchResult`,
  `SemanticSearchCandidate`, `SemanticSearchBudget`, `SemanticSearchSession`,
  `SemanticSearchReadiness`, and `SemanticSearchArbitrationSummary` describe local search metadata
  and arbitration summaries only.
- Search may read only local vector persistence entries and may require isolated embedding runtime
  output metadata. It must not scan filesystems, start background ingestion, call cloud/API/vector
  providers, or download providers.
- Runtime behavior is bounded by candidate count, timeout metadata, similarity range, stale
  request protection, busy-state refusal, and deterministic tie handling.
- Semantic search may produce candidate metadata, bounded similarity matches, and hybrid
  arbitration summaries. It must not alter `RetrievalPlanningResult`, `PromptContextBlock` values,
  prompt assembly, prompt injection, or deterministic source ranking.
- Empty indexes and disabled persistence are safe fallback states. Future authority activation
  requires a separate explicit phase with indexing policy, privacy/safety gates, fallback tests,
  prompt authority review, and continued QML non-exposure guarantees.

## 89. Hybrid Retrieval Bridge Is Advisory Metadata Only

Decision: Add a bounded hybrid retrieval bridge that reads deterministic retrieval candidates and
semantic search candidates, but keeps deterministic retrieval as the final authority.

Reason: The system needs a concrete bridge lifecycle before semantic candidates can be evaluated
beside deterministic retrieval. That bridge must be testable without giving semantic search prompt
authority or mutation rights.

Boundary rules:

- `HybridRetrievalBridgePolicy`, `HybridRetrievalBridgeStatus`,
  `HybridRetrievalBridgeResult`, `HybridBridgeCandidate`, `HybridBridgeBudget`,
  `HybridBridgeReadiness`, `HybridBridgeArbitration`, and `HybridBridgeSourceSummary` are
  metadata-only records.
- Bridge arbitration is deterministic-first. Deterministic retrieval candidates fill bridge
  capacity first, deterministic candidates win all ties/conflicts, and semantic candidates may
  only fill unused bounded metadata capacity.
- Semantic candidates are advisory only. They cannot override deterministic source priority,
  selected retrieval candidates, prompt context blocks, prompt assembly, prompt injection, or
  deterministic fallback behavior.
- Disabled, empty, stale, busy, timeout, and refused semantic sources resolve to deterministic
  fallback summaries. Deterministic retrieval must remain fully functional without semantic search.
- QML may expose status, readiness, candidate counts, deterministic-vs-semantic participation,
  arbitration summaries, fallback summaries, and checks only. Raw vectors, prompt payloads,
  provider handles, filesystem paths, and debug dumps remain hidden.
- Future semantic-authority activation requires a separate explicit phase with indexing policy,
  privacy/safety gates, prompt-authority review, deterministic fallback tests, mutation tests, and
  QML non-exposure tests.

## 90. Semantic Acceptance Is Bounded And Supplemental Only

Decision: Allow deterministic gates to approve a tightly bounded subset of semantic advisory
candidates as retrieval supplements, while keeping deterministic retrieval authoritative.

Reason: Hybrid retrieval needs a testable acceptance lifecycle before semantic candidates can ever
participate near prompt context. The safe step is bounded supplemental approval metadata that
cannot replace deterministic candidates or mutate prompt construction.

Boundary rules:

- `SemanticAcceptancePolicy`, `SemanticAcceptanceStatus`, `SemanticAcceptanceResult`,
  `SemanticAcceptedCandidate`, `SemanticAcceptanceBudget`, `SemanticAcceptanceReadiness`,
  `SemanticAcceptanceArbitration`, `SemanticAcceptanceFallback`, and
  `SemanticAcceptanceSourceSummary` are value-only records.
- Acceptance may read `RetrievalPlanningResult`, `HybridRetrievalBridgeResult`, and
  `SemanticSearchResult`. It must not mutate those inputs, create or mutate
  `PromptContextBlock` values, change deterministic candidate ordering, or alter retrieval source
  priority.
- Deterministic retrieval candidates remain primary and win all conflicts. Accepted semantic
  candidates are explicitly semantic, supplemental-only, local-only, non-authoritative, count
  bounded, and character-budgeted.
- Disabled semantic search, semantic errors/refusals, stale requests, busy state, timeouts, empty
  semantic results, and exhausted supplement capacity resolve to deterministic fallback summaries.
- QML may expose status/readiness, approved supplement counts, source participation,
  arbitration/fallback summaries, bounded budget state, and safety checks only. Raw vectors,
  prompt payloads, provider handles, filesystem paths, and debug dumps remain hidden.
- Future semantic-authority activation requires a separate explicit phase with indexing policy,
  privacy/safety gates, prompt-authority review, deterministic fallback tests, mutation tests, and
  QML non-exposure tests.

## 91. Semantic Supplement Assembly Has No Prompt Authority

Decision: Accepted semantic candidates may be prepared as a separate bounded supplement metadata
bundle, but semantic supplement assembly is disabled by default and cannot affect live prompts.

Reason: The application needs a safe bridge between semantic acceptance and future prompt assembly
without allowing semantic retrieval to replace deterministic context or silently mutate provider
requests.

Boundary rules:

- `SemanticSupplementBlock`, `SemanticSupplementBundle`,
  `SemanticSupplementAssemblyPolicy`, `SemanticSupplementAssemblyStatus`,
  `SemanticSupplementAssemblyResult`, `SemanticSupplementBudget`,
  `SemanticSupplementReadiness`, and `SemanticSupplementSafetyReport` are value-only records.
- Assembly reads `SemanticAcceptanceResult` and produces supplement metadata only when an explicit
  test-only assembly gate is enabled. The desktop default remains disabled.
- Live prompt inclusion is blocked. Existing prompt context injection remains unchanged and still
  consumes deterministic retrieval-planning selections only.
- Semantic supplements stay separate from deterministic context blocks. They cannot replace
  deterministic context, reorder deterministic context, override conversation windows, override
  deterministic summaries, override committed memory, or override runtime metadata.
- Runtime protections include bounded block count, bounded character budget, deterministic
  ordering, deterministic truncation, disabled fallback, empty fallback, stale/busy/timed-out/
  refused fallback, and safety-report checks.
- QML may expose readiness/status, supplement block count, budget summary, safety summary, and
  checks only. Raw prompt blocks, raw vectors, raw scores, provider handles, filesystem paths, and
  debug dumps remain hidden.
- Future semantic prompt activation requires a separate explicit phase with prompt-authority
  policy, privacy/safety gates, deterministic fallback tests, live prompt inclusion tests,
  mutation tests, and QML non-exposure tests.

## 92. Semantic Prompt Inclusion Is Controlled And Supplemental

Decision: Semantic supplements may enter local prompt assembly only through an explicit
disabled-by-default inclusion gate, after deterministic context, and only when semantic authority
and safety reports approve the bounded local supplement bundle.

Reason: Phase 17 needs a live inclusion path without weakening deterministic retrieval authority
or exposing raw semantic internals to QML.

Boundary rules:

- `SemanticPromptInclusionPolicy`, `SemanticPromptInclusionStatus`,
  `SemanticPromptInclusionResult`, `SemanticPromptInclusionBudget`,
  `SemanticPromptInclusionSafetyReport`, `SemanticPromptInclusionFallback`, and
  `SemanticPromptInclusionAuditSummary` are value records for the inclusion gate.
- Inclusion is disabled by default and requires prompt context injection enablement, semantic
  prompt authority approval, bounded/safe supplement assembly, local-only mode, and a passing
  inclusion safety report.
- Included semantic content is appended after deterministic context blocks as a clearly delimited
  supplemental/non-authoritative block. Deterministic context ordering is preserved.
- Semantic supplements cannot replace deterministic context, reorder deterministic context,
  override committed memory, override conversation windows, override deterministic summaries, or
  override runtime metadata.
- Disabled, denied, unsafe, empty, stale, busy, timed-out, and refused semantic states return the
  exact deterministic-only prompt and audit the fallback.
- QML may expose enabled/status, included count, budget, fallback, audit, authority-preserved
  state, and checks only. Raw prompt text, raw supplement blocks, vectors, scores, provider
  handles, filesystem paths, and debug dumps remain hidden.

## 93. Phase 17 Semantic Checkpoint Preserves Deterministic Authority

Decision: Close Phase 17 with semantic retrieval and prompt inclusion bounded by deterministic
authority, default-disabled inclusion, local-only policy, and QML non-exposure guarantees.

Reason: Phase 17 introduced a controlled semantic prompt inclusion path. Before Phase 18, the
project needs an explicit checkpoint proving that inclusion remains subordinate to deterministic
retrieval and does not expand provider, filesystem, tool, plugin, or autonomous runtime authority.

Boundary rules:

- Deterministic retrieval planning remains the final prompt-context authority.
- Semantic inclusion remains disabled by default in both settings persistence and inclusion
  policy. Enabling it is explicit opt-in and still requires prompt context injection and semantic
  authority approval.
- Semantic supplements are supplemental-only, local-only, bounded by count and character budgets,
  clearly delimited, and appended only after deterministic context.
- Semantic supplements cannot replace deterministic context, reorder deterministic context,
  override committed memory, override conversation windows, override deterministic summaries, or
  override runtime metadata.
- Disabled, denied, unsafe, empty, stale, busy, timed-out, and refused semantic states return
  deterministic-only prompts.
- QML may expose status, readiness, counts, budget, fallback, audit, authority-preserved state, and
  checks only. Raw prompts, supplement content, vectors, scores, provider handles, filesystem
  paths, and debug dumps remain hidden.
- The checkpoint does not authorize filesystem indexing, cloud/API/vector provider activation,
  provider downloads, tools/plugins, autonomous actions, or runtime authority expansion.

## 95. Agent Planning Sessions Are Metadata-Only

Decision: Phase 18.7 through Phase 18.9 add bounded planning-session, arbitration, refusal,
safety-report, and fallback records to `IAgentTaskRuntime`, but planning sessions are not an
executor, approval flow, scheduler, or tool runtime.

Reason: The Agents surface needs safe visibility into how future task plans would be ordered and
refused before any execution authority exists. Planning must remain deterministic, local-only, and
bounded so it cannot become implicit autonomous behavior.

Boundary rules:

- Planning candidates are derived from the deterministic task queue and ordered by priority, queue
  sequence, and task id.
- Candidate count, step count, and summary length budgets bound the planning session.
- Unsafe planning candidates become refusal metadata with safe summaries; blocked reasons are
  exposed without raw private payloads.
- Budget overflow uses deterministic fallback metadata.
- `executionAttempted` remains false for planning results and task results.
- There are no tools/plugins, filesystem/system actions, shell/subprocess execution, provider or
  model calls, cloud/API calls, background workers, approval controls, or autonomous loops.
- QML may expose only status strings, counts, candidate summaries, arbitration summaries, refusal
  summaries, and fallback summaries.

## 96. Voice Runtime Readiness Is Permission And Path Metadata Only

Decision: Phase 18.16 through Phase 18.18 model Piper and Whisper runtime readiness with
deterministic local-only metadata, not runtime activation.

Reason: Future STT/TTS activation needs explicit permission, sandbox, path-readiness, and safety
vocabulary before Sentinel can safely open microphones, play audio, run local binaries, load voice
models, or add voice controls.

Boundary rules:

- `VoiceRuntimePolicy`, `VoiceRuntimeStatus`, `VoiceRuntimeReadiness`, `VoiceRuntimeHealth`,
  `VoiceRuntimeSandbox`, `VoiceRuntimeRestriction`, `VoiceRuntimeBudget`,
  `VoiceRuntimeReadinessReport`, and `VoiceRuntimeSafetyReport` remain value-only records.
- `WhisperRuntimeDescriptor`, `WhisperRuntimeStatus`, `WhisperRuntimeReadiness`,
  `WhisperRuntimeConfiguration`, and `WhisperRuntimePathSummary` describe future STT readiness.
- `PiperRuntimeDescriptor`, `PiperRuntimeStatus`, `PiperRuntimeReadiness`,
  `PiperRuntimeConfiguration`, and `PiperRuntimePathSummary` describe future TTS readiness.
- Readiness reports only configured/missing/refused counts and safe summaries. Unsafe or non-local
  path-style values are refused, and raw configured paths are not exposed by the new readiness
  summaries.
- Local-only, disabled, sandbox-required, future microphone access, future audio playback, future
  transcription runtime, and future synthesis runtime labels are metadata only and grant no
  authority.
- `executionAttempted` remains false. No Piper/Whisper inference, subprocess execution,
  microphone capture, playback, streaming, filesystem scanning, downloads, cloud/API calls,
  background workers, path picker, start/stop control, or autonomous voice loop is added.
- Future STT/TTS activation requires a separate explicit phase with permission prompts, sandbox
  implementation, runtime clients, audio lifecycle, UI controls, and tests.

## 97. Whisper STT Runtime Starts As A Non-Executing Audio-File Boundary

Decision: Phase 18.19 through Phase 18.21 add a Whisper transcription client boundary and
request/result metadata for future local audio-file STT, while keeping Whisper execution disabled.

Reason: Whisper STT needs a controlled runtime contract for binary/model/audio metadata, timeout
fallbacks, safety reporting, and QML summaries before Sentinel can safely allow any local
transcription process or audio input lifecycle.

Boundary rules:

- `WhisperTranscriptionPolicy`, status, request, result, session, budget, readiness, safety,
  fallback, and trace records are value metadata.
- `IWhisperTranscriptionClient` owns the future low-level STT client boundary.
- `NullWhisperTranscriptionClient` refuses without side effects. `LocalWhisperTranscriptionClient`
  is a bounded skeleton that validates metadata and refuses before subprocess execution.
- Missing binary, missing model, missing audio, unsafe/non-local paths, invalid timeout budget, and
  runtime privilege requests produce deterministic refusal/fallback metadata.
- `executionAttempted` remains false for results, sessions, traces, and safety reports.
- No microphone capture, live recording, audio playback, streaming STT, subprocess execution,
  cloud/API calls, downloads, filesystem scanning, prompt injection, automatic chat send, file
  picker, record button, enabled transcribe button, or autonomous voice loop is added.
- Future controlled audio-file transcription and future microphone/live STT require separate
  explicit phases with permission, sandbox, lifecycle, UI, and regression tests.

## 98. Piper TTS Runtime Starts As A Non-Executing Synthesis Boundary

Decision: Phase 18.22 through Phase 18.24 add a Piper synthesis client boundary and request/result
metadata for future local TTS, while keeping Piper execution and playback disabled.

Reason: Piper TTS needs a controlled runtime contract for binary/model/text metadata, timeout
fallbacks, safety reporting, and QML summaries before Sentinel can safely allow any local
synthesis process or audio output lifecycle.

Boundary rules:

- `PiperSynthesisPolicy`, status, request, result, session, budget, readiness, safety, fallback,
  and trace records are value metadata.
- `IPiperSynthesisClient` owns the future low-level TTS client boundary.
- `NullPiperSynthesisClient` refuses without side effects. `LocalPiperSynthesisClient` is a
  bounded skeleton that validates metadata and refuses before subprocess execution.
- Missing binary, missing model, unsafe/non-local paths, invalid timeout budget, empty text, and
  runtime privilege requests produce deterministic refusal/fallback metadata.
- `executionAttempted` remains false for results, sessions, traces, and safety reports.
- The older Piper file-output opt-in/generation path is compatibility metadata only and refuses
  without writing audio or reaching a process client.
- No audio playback, live voice streaming, microphone capture, subprocess execution, cloud/API
  calls, downloads, filesystem scanning, automatic chat/audio injection, file picker, speak/play
  button, or autonomous voice loop is added.
- Future controlled synthesis and future playback/audio-device support require separate explicit
  phases with permission, sandbox, lifecycle, UI, and regression tests.

## 99. Voice Pipeline Session Orchestration Is Readiness Metadata Only

Decision: Phase 18.25 through Phase 18.27 add deterministic voice pipeline session orchestration
metadata that composes Whisper STT readiness, local chat inference readiness, and Piper TTS
readiness without enabling voice execution.

Reason: The app now has separate readiness foundations for voice runtime configuration, Whisper
STT, local chat inference, and Piper TTS. A future voice feature needs an ordered session lifecycle
and safe failure visibility before any microphone, playback, or executable STT/TTS work can be
considered.

Boundary rules:

- `VoicePipelineSession`, id/status/policy/result/step/trace/budget/readiness/safety/fallback/
  summary records are value metadata.
- The deterministic lifecycle is prepare, await audio input, transcription readiness, chat
  inference readiness, synthesis readiness, and completion/refusal/fallback.
- Missing Whisper readiness blocks transcription. Missing local chat/model readiness blocks
  inference. Missing Piper readiness blocks synthesis.
- Unsafe stage transitions become refused/fallback metadata only.
- `ApplicationController` derives the session from existing readiness/status values and does not
  invoke Whisper, Piper, local inference workers, chat send, transcript injection, playback, or
  subprocess execution.
- `executionAttempted` remains false. Microphone capture, playback, Whisper execution, Piper
  execution, subprocess execution, voice chat auto-send, transcript auto-injection, background
  workers, and autonomous loops are blocked.
- Controller and view-model exposure remains QML-safe strings, counts, and string lists. No raw
  filesystem paths, client/provider handles, transcript payloads, audio paths, or runtime objects
  are exposed.
- Future controlled audio-file STT, controlled synthesis, and live voice phases require separate
  explicit scopes, permission/sandbox gates, lifecycle ownership, UI controls, and regression
  tests.

## 100. Controlled Audio-File Sessions Are Metadata Only

Decision: Phase 18.28 through Phase 18.30 add deterministic audio-file session and validation
metadata for future controlled offline STT ingestion without enabling file loading, decoding,
transcription, playback, or runtime execution.

Reason: Whisper STT readiness and voice pipeline orchestration need a safe file-session boundary
before any later phase can authorize controlled local audio-file transcription. The desktop UI can
show readiness/refusal/fallback state now without gaining file input authority.

Boundary rules:

- `AudioFileSession`, id/status/policy/result/readiness/safety/fallback/summary, descriptor,
  validation/status/restriction/budget, and trace records are value metadata.
- Supported future extensions are wav, mp3, flac, and ogg. Unsupported extensions become
  deterministic refusal metadata.
- Unsafe or non-local path-style values are refused before any file access and raw path values are
  not exposed to QML.
- Empty, oversized, sandbox-required, and disabled states become deterministic fallback metadata.
- `executionAttempted` remains false. File loading, waveform decoding, transcription, playback,
  microphone capture, subprocess execution, filesystem scanning, automatic ingestion, autonomous
  loops, and cloud/API calls are blocked.
- Controller and view-model exposure remains QML-safe strings and string lists only. No raw
  filesystem paths, file handles, waveform data, transcripts, provider/client handles, or runtime
  objects are exposed.
- Future controlled transcription and future live microphone phases require separate explicit
  scopes with permission, sandbox, lifecycle, UI, and regression tests.

## 101. Phase 18 Agent/Voice Checkpoint Preserves No-Execution Boundaries

Decision: Close Phase 18 with the agent task runtime, capability registry, tool contracts, voice
runtime readiness, Whisper STT, Piper TTS, voice pipeline sessions, and audio-file sessions still
metadata-only.

Reason: Phase 18 introduced several adjacent foundations for future agent/tool/voice runtime
work. Before Phase 19, the project needs an explicit checkpoint proving those foundations did not
combine into implicit execution authority.

Boundary rules:

- `IAgentTaskRuntime` remains task orchestration metadata only. Queue, lifecycle, planning,
  arbitration, refusal, capability, and trace records do not execute or schedule work.
- Tool contracts remain permission/sandbox metadata only. They are not grants, executors,
  approval flows, sandbox implementations, plugin hosts, filesystem adapters, subprocess
  boundaries, or export actions.
- Voice runtime readiness remains path/permission/session metadata only. It is not a Piper or
  Whisper launcher, model loader, microphone permission flow, playback system, or filesystem
  scanner.
- Whisper and Piper client skeletons validate metadata and refuse before execution.
- Voice pipeline sessions compose existing readiness values only and do not call STT/TTS clients,
  local inference workers, chat send, transcript injection, playback, or subprocess execution.
- Audio-file sessions validate descriptor metadata only and do not read files, decode waveforms,
  transcribe, play audio, scan filesystems, or ingest automatically.
- `executionAttempted` remains false across agent/task/planning/capability/tool/voice/Whisper/
  Piper/pipeline/audio-file paths.
- No microphone capture, playback, subprocess execution, filesystem scanning, cloud/API calls,
  tools/plugins, autonomous loops, real STT inference, or real TTS inference are authorized.
- Future Phase 19 work must preserve this posture or explicitly scope and test a controlled
  authority change.

## 94. Agent Task Queue Is Metadata-Only

Decision: Phase 18.4 through Phase 18.6 add an agent task queue and lifecycle read model, but the
queue is not an executor or scheduler.

Reason: The Agents surface needs deterministic task readiness and lifecycle visibility before any
future execution phase. That visibility must not create implicit authority to run tools, spawn
workers, touch the filesystem, call providers, or loop autonomously.

Boundary rules:

- Queue ordering is deterministic by priority, queue sequence, and task id.
- Lifecycle transitions are metadata-only: queued, planned, blocked, completed as metadata, and
  refused.
- Execution attempts are refused safely and keep `executionAttempted` false.
- There are no execution buttons, approval controls, tool/plugin controls, shell controls,
  filesystem controls, background workers, or autonomous loops.
- QML may expose only counts, task summaries, latest lifecycle summaries, and ordered trace
  summaries.

## 94. Agent Task Runtime Is Metadata-Only

Decision: Agent task orchestration starts as a metadata-only runtime boundary through
`IAgentTaskRuntime`.

Reason: Sentinel needs task planning/readiness visibility before any future tool or autonomous
runtime authority is introduced. Keeping tasks value-only allows controller and QML exposure to
stabilize without permitting execution.

Boundary rules:

- `AgentTask`, task ids, type/status/priority/source enums, plans, steps, results, traces, safety
  policy, and runtime status are value records.
- `StaticAgentTaskRuntime` creates deterministic local task metadata and ordered traces.
- Execution is refused by design and `AgentTaskResult::executionAttempted` remains false.
- Safety policy blocks tools, plugins, filesystem/system actions, shell/subprocess execution,
  cloud/API calls, background workers, and autonomous loops.
- QML receives only status strings, counts, latest summaries, and trace summaries.
- Any real tool/task runtime must arrive in a separate phase behind explicit permission, safety,
  approval, sandbox, and UI boundaries.

## 102. Product UI Density And Developer Mode

Decision: Normal UI surfaces prioritize user-facing local chat, local memory, registered profile,
and settings summaries. Advanced retrieval, semantic/vector, prompt authority, arbitration, trace,
tool-contract, voice-pipeline, and raw diagnostic summaries belong behind Developer Mode or
Developer segments.

Reason: Developer Mode is a diagnostic visibility boundary only. It must not grant backend
authority or imply active execution.

## 103. Localization Direction

Decision: Use Qt-native localization for Sentinel's core desktop UI.

Reason: `qsTr` and Qt `.ts`/`.qm` catalogs work across QML and C++, integrate with Qt Linguist,
support context-aware translation, and fit the native desktop build. A JSON/string-catalog pattern
can be reconsidered only for non-Qt plugin content or remote documentation later.

Language policy:

- Preferred languages are English and Turkish.
- `AppSettings::appLanguage` persists `system`, `en`, or `tr`.
- System Default resolves to Turkish only when the system locale is Turkish; otherwise English is
  the fallback.
- Manual language selection is exposed in Settings through `DesktopShellViewModel`.
- Language selection is presentation-only. It must not change prompt assembly, model/provider
  routing, runtime permission/safety policy, context behavior, memory behavior, provider behavior,
  filesystem behavior, or tool execution authority.
- Internal ids, enum values, provider names, model names, filesystem paths, and diagnostic machine
  tokens are not translation targets.

Current status: English and Turkish `.ts` catalogs exist and key QML surfaces use `qsTr()`.
Startup translator loading is present, but compiled `.qm` packaging and live retranslation remain
future work; language changes may require restart.

## 104. Open-Source Ecosystem Adoption Posture

Decision: Phase 41 establishes an adopt/adapt/optional/avoid posture for open-source AI assistant
ecosystem components, but does not authorize implementation or execution.

Reason: Sentinel needs a durable long-term reference before adding plugins, tools, agents, voice,
semantic retrieval, cloud providers, or developer automation. Ecosystem patterns are valuable, but
Sentinel must preserve local-first operation, explicit user authority, Qt/C++ architecture, and
metadata-only boundaries until later activation phases.

Adoption rules:

- Adopt only components that fit Sentinel's native local-first posture and pass license,
  packaging, security, and platform review.
- Adapt architectural patterns such as MCP-style capability declarations, graph state,
  checkpoints, human approval states, patch review UX, explainable context assembly, and audit
  trails.
- Keep Python-heavy agent/RAG frameworks optional or inspiration-only unless a later phase creates
  a separate integration boundary.
- Avoid autonomous defaults, hidden tool execution, hidden provider fallback, automatic cloud
  boundary crossing, hidden memory mutation, and default voice cloning.
- Review code licenses, model weights, voice models, sample assets, and plugin/server licenses
  separately before any future distribution.

Current status: `docs/OPEN_SOURCE_ECOSYSTEM_REVIEW.md` and
`docs/SENTINEL_LONG_TERM_STRATEGY.md` are the master references for this posture. No runtime
behavior changes are authorized by this decision.

## 105. Capability Roadmap Activation Policy

Decision: Sentinel's capability roadmap defaults to local-first operation, no hidden background
execution, permission-tiered activation, user-visible shell integration, opt-in cloud providers,
workspace/path-scoped filesystem access, and disabled-by-default voice activation.

Reason: Phase 42 converts ecosystem and strategy findings into a product roadmap. The roadmap must
make future capability order concrete without quietly granting authority to providers, tools,
agents, filesystems, cloud APIs, microphones, playback devices, or background workers.

Policy decisions:

- Local-first default: local Ollama and local data boundaries remain the default posture. Cloud
  providers are optional and must never become active through fallback, discovery, or implied
  configuration.
- No hidden background execution: background planning, hidden retries, filesystem observation,
  provider probing, autonomous agents, tool runs, microphone capture, playback, and cloud calls
  are not allowed unless a later phase explicitly implements visible lifecycle, permissions,
  cancellation, and audit.
- Permission-tiered feature activation: future capabilities use Disabled, Ask Every Time,
  Trusted, and Enabled states. Core policy must enforce these states; QML copy alone is not a
  security boundary.
- Companion surface restriction: menu bar/system tray companion integration is allowed only as a
  user-visible shell surface for status, navigation, quick capture, brief entry, and pause/setting
  controls. It must not become a hidden daemon or scheduler.
- Cloud/API opt-in only: cloud providers require explicit provider selection, secure credential
  storage, privacy/billing warnings, user-visible activation, execution gates, and audit. API key
  entry and provider calls remain out of scope until separately implemented.
- Filesystem workspace/path scope only: future filesystem access must be limited to user-approved
  workspaces or paths with explicit purpose, session, and revocation metadata. Broad scanning and
  implicit mutation are forbidden by default.
- Voice disabled by default: voice activation, microphone capture, STT, TTS, and playback remain
  disabled until explicit local-first voice phases add permission prompts, runtime gates, model
  and voice license review, visible session state, and tests.

Current status: `docs/SENTINEL_CAPABILITY_ROADMAP.md` is a roadmap reference only. Phase 42 adds
no runtime execution, tools/plugins, cloud/API calls, API-key input, filesystem scanning,
microphone/playback/STT/TTS activation, autonomous agents, background workers, dependencies, or
product behavior changes.

## 106. Workspace Foundation Remains Metadata-Only

Decision: Phase 44 introduces `WorkspaceService`, a persisted selected workspace id, Settings >
Workspace UX, Home workspace status, and Developer Mode boundary diagnostics as metadata-only
workspace foundation.

Reason: Sentinel needs a durable local-first project-context boundary before any future
filesystem access. The foundation must make the disabled state visible and prepare permission
language without accidentally becoming a file manager, scanner, indexer, or agent runtime.

Rules:

- The default workspace is a local placeholder, not a selected filesystem path.
- The selected workspace setting stores only an id such as `local-placeholder`.
- Workspace readiness summaries must state that workspace access is not enabled yet.
- QML receives only safe strings and string lists.
- No directory picker, recursive scanning, file reading, indexing, embeddings, subprocesses,
  tools/plugins, autonomous agents, background workers, hidden prompt-context injection, or
  workspace-derived memory writes are authorized by this phase.

Current status: Phase 44 is implemented as a local metadata boundary. Future workspace activation
requires explicit permission scopes, session ownership, revocation metadata, and core-enforced
policy before any filesystem access can be added.

## 107. Provider Roadmap Must Not Be Ollama-Only

Decision: Sentinel's provider roadmap must support multiple local runtime providers while keeping
Local Ollama as the current default and only currently active chat execution path.

Reason: Ollama is a strong default, but users may run local models through llama.cpp server,
LM Studio, OpenAI-compatible local servers, vLLM-compatible endpoints, Jan, or other compatible
local runtimes. The core and UI should model these providers generically now so later activation
does not require undoing Ollama-specific assumptions.

Provider rules:

- Provider metadata must separate provider id, endpoint, model-list discovery, selected model,
  readiness, streaming support, context-window metadata, local/cloud scope, API-key requirement,
  and capabilities.
- Required provider targets are Ollama, llama.cpp server, LM Studio local server,
  OpenAI-compatible local endpoints, vLLM-compatible endpoints, Jan / other safe local
  OpenAI-compatible runtimes, and future cloud providers such as OpenAI, Claude, Gemini, and
  OpenRouter.
- Any provider exposing an OpenAI-compatible API must use a shared OpenAI-compatible adapter
  where practical.
- LM Studio and llama.cpp begin as readiness/metadata-only unless safe chat execution is
  explicitly wired in a later phase.
- No automatic probing, background discovery daemon, hidden network call, filesystem scan,
  subprocess launch, automatic provider fallback, cloud call, or API-key storage is authorized by
  this roadmap.
- Local endpoints default to loopback only: `127.0.0.1`, `localhost`, and `::1`.
- Execution is allowed only when the provider is local or explicitly trusted, the endpoint is
  valid, a model is selected, chat is enabled, and permission policy allows it.

UI rules:

- Settings > Local AI should show a provider selector for Ollama, llama.cpp, LM Studio,
  OpenAI-compatible local, and Custom local endpoint.
- Provider readiness should expose Installed/Reachable, Endpoint, Models discovered, Streaming
  supported, and Context length when known.
- Home/Chat and AI Bridge should use generic selected-provider wording such as Local Provider
  instead of hardcoding Ollama-only labels.

Test expectations:

- Add provider registry coverage for Ollama, llama.cpp, LM Studio, and OpenAI-compatible targets.
- Add endpoint validation tests, selected provider persistence tests, UI/view-model exposure
  tests, and disabled-provider send refusal tests.
