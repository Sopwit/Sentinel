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
optional `/api/tags` installed-model metadata. Phase 9.3 through Phase 9.5 add the first
controlled local-only inference boundary through `ILocalInferenceClient`: prompt execution is
allowed only after explicit controller routing through runtime permission and safety checks, and
the Ollama implementation is restricted to local non-streaming `/api/generate`. Chat provider
routing, streaming, downloads, subprocess launch, cloud calls, tools, plugins, and
filesystem/system actions remain disabled. Phase 9.6 through Phase 9.8 add selected local model
metadata, runtime inference UX state, and a disabled streaming skeleton. Model selection is
configuration metadata with safe discovered-model validation when available. Phase 10.0 through
Phase 10.2 add explicit chat-to-Ollama routing: chat uses local inference only when the persisted
opt-in setting is enabled, a valid local model is selected or resolved, the endpoint is loopback
HTTP, and permission/safety gates pass. Phase 10.3 through Phase 10.5 add guarded local-only chat
streaming: streaming remains disabled by default and activates only after the local chat inference
opt-in, streaming opt-in, valid model, loopback endpoint, and permission/safety gates pass. Phase
10.6 through Phase 10.8 add lightweight local model selection and runtime management UX over
discovered metadata without adding model-management actions. Phase 11.0 through Phase 11.2 add
lightweight local model management readiness metadata: deterministic recommendations, approximate
RAM/disk requirement summaries, and unavailable action results without downloads, deletes,
installs, scans, subprocesses, cloud calls, credentials, tools/plugins, or autonomous loops. Phase
11.3 through Phase 11.6 stabilize the local AI experience: safer refusal/error summaries, clearer
selected-model and discovered-model UX, streaming preview reset/finalization behavior, and
QML-safe runtime status exposure without adding new runtime authority. Phase 11.7 through Phase
11.9 checkpoint the local AI user flow and runtime QA posture with focused tests and
`docs/PHASE_11_CHECKPOINT.md`; they add no model management actions, cloud providers,
filesystem/system actions, tools/plugins, subprocess launch, autonomous behavior, or UI redesign.
Phase 12.0 through Phase 12.2 add a disabled voice boundary and TTS/STT readiness skeleton with
null providers only; no recording, playback, Whisper/Piper execution, subprocesses, downloads,
cloud calls, API keys, filesystem/system actions, or voice controls are enabled. Phase 12.3
through Phase 12.6 add metadata-only voice runtime planning and session orchestration with a
deterministic idle, preparing, awaiting-input, transcribing-placeholder, inference-placeholder,
synthesis-placeholder, completed, blocked, and error pipeline; no microphone, playback,
Piper/Whisper execution, subprocesses, downloads, cloud calls, API keys, filesystem/system actions,
or autonomous voice loops are enabled. Phase 12.7 through Phase 12.9 checkpoint the voice
architecture and document local Piper/Whisper integration prerequisites in
`docs/PHASE_12_CHECKPOINT.md`; they add no microphone access, playback, Piper/Whisper execution,
subprocesses, downloads, cloud calls, API keys, filesystem/system actions, voice controls, or
autonomous voice loops. Phase 13.0 through Phase 13.2 add metadata-only local voice runtime
environment ownership for future Piper/Whisper binaries, models, permissions, and safety
reporting; execution remains blocked and no microphone, playback, subprocess, filesystem-wide
scan, model loading, download, cloud call, API key, path picker, or voice control is enabled.
Phase 13.3 through Phase 13.5 add a Piper TTS adapter skeleton behind the text-to-speech boundary:
configuration, request/result, model descriptor, null client, and provider readiness metadata are
present, but audio playback, file-output synthesis, Piper subprocess execution, downloads, cloud
calls, API keys, filesystem-wide scans, model loading, speak controls, and path/model pickers
remain disabled. Phase 13.6 through Phase 13.8 add controlled local Piper file-output synthesis:
it remains disabled by default and can run only after explicit binary/model/output-directory,
local-only request, process-permission, and voice safety gates pass. Playback, microphone access,
downloads, cloud/API-key behavior, filesystem-wide scans, autonomous voice loops, and voice
controls remain out of scope. Phase 13.9 checkpoints the Voice/Piper architecture in
`docs/PHASE_13_CHECKPOINT.md`, confirms the current TTS path is `text -> Piper provider -> gated
file-output metadata`, and adds no runtime behavior. Phase 14.0 through Phase 14.3 add persisted
local Piper/Whisper path configuration and exact-path validation metadata only; no Piper/Whisper
execution, microphone access, playback, downloads, cloud/API keys, filesystem-wide scans, or
autonomous voice loops are enabled. Phase 14.4 through Phase 14.6 polish the voice configuration
UX and add safe hint-only path suggestions: binary hints inspect only fixed known Homebrew/local
paths, model hints inspect only configured paths, and no hint is applied automatically. Phase
14.7 through Phase 15.0 activate controlled local Ollama chat inference in the desktop app:
health/discovery and generate calls are loopback-only, selected-model settings are honored,
runtime state is exposed as unavailable/idle/inferencing/streaming/failed, and a narrow
local-only permission policy allows only explicit local inference while continuing to deny cloud
providers, API keys, autonomous agents, tools, shell execution, filesystem-wide actions, model
management actions, microphone access, playback, Piper execution, and Whisper execution.
Phase 15.1 through Phase 15.3 refine voice path setup and controlled Piper readiness preparation:
Piper/Whisper paths persist through settings, exact configured-path validation is exposed as
QML-safe metadata, Piper file-output TTS preparation reports Ready/Blocked/Missing with exact
blocked reasons, and Whisper remains future STT preparation metadata only. No Piper execution,
Whisper execution, microphone access, playback, downloads, filesystem-wide scans, cloud/API keys,
or autonomous voice loop is added. Phase 15.4 through Phase 15.6 enable controlled Piper
file-output execution behind a persisted opt-in and explicit Generate TTS File action: configured
Piper binary/model paths are reused, output is generated only inside an app-controlled cache/temp
directory, and status metadata reports disabled, blocked, missing, running, succeeded, failed, and
timeout states. Playback, microphone access, Whisper execution, downloads, cloud/API keys,
filesystem-wide scans, and autonomous voice loops remain out of scope. Phase 15.7 stabilizes
controlled local Ollama reliability before additional voice/STT work: health, discovery,
generation, and streaming requests carry timeout metadata; failures are categorized into
not-running, endpoint-unreachable, missing/invalid model, timeout, malformed response, interrupted
stream, permission/safety block, and duplicate busy request; busy/streaming cleanup is explicit;
and no new cloud, model-management, tool, filesystem/system, autonomous, playback, microphone,
Piper, or Whisper authority is added. Phase 15.8 moves real local generation and streaming behind
`ILocalInferenceWorker`: controller policy checks still happen first, callbacks are request-id
guarded, stale results after metadata cancellation are ignored, successful completions append one
assistant message, and failed streams clear preview text without persisting partial assistant
output. Phase 15.9 adds a concise conversation-runtime read model over that path: current
conversation state, request id, active model, active route, active streaming flag, last success
summary, last error/refusal summary, and last latency summary. Clear Chat resets that transient
state and persistent chat through the existing chat-history boundary, while restart loading keeps
persisted transcript rows stable without duplicating system or assistant messages. Phase 15.10
adds value-only persistent conversation UX metadata for the same single transcript: persisted
versus runtime-only status, message counts, last saved/restored status, and clear results. It does
not add multi-conversation storage, transcript browsing, export/import, encryption, pruning,
search, or new filesystem authority. Phase 15.11 through Phase 15.13 add transcript QA metadata
on that same boundary: literal case-insensitive in-memory search over the current `ChatSession`
only, clear-chat reset of search state, and disabled export readiness/result metadata for Plain
Text, Markdown, and JSON. Export requests remain metadata-only and write no files. No vector
search, semantic search, embeddings, SQLite full-text index, persisted search state, file picker,
export write, import, multi-conversation storage, cloud/API key, tool/plugin, or filesystem/system
action is added. Phase 15.14 through Phase 15.16 implement controlled current-transcript export:
Markdown and JSON writes are allowed only to the app-controlled export directory below Qt
`AppDataLocation`, filenames are sanitized/timestamped/unique, empty transcripts are refused, and
QML receives only safe filename/status/count/timestamp summaries. No import, arbitrary output path,
file picker, cloud sync, external process, tools/plugins, semantic/vector search, or
multi-conversation storage is added. Phase 15.17 through Phase 15.19 adds single-transcript
conversation-browser metadata (`ConversationDisplayTitle`, `ConversationListEntry`,
`ConversationListSummary`, `ConversationBrowserStatus`) so UI can expose one deterministic current
entry summary without introducing multi-conversation storage, migration, or browser controls. Phase
15.20 through Phase 15.22 adds metadata-only multi-conversation planning/readiness records
(`ConversationId`, `ConversationDescriptor`, `ConversationLifecycleStatus`,
`ConversationStorageMode`, `ConversationMigrationReadiness`, `ConversationSchemaPlan`) while keeping
storage behavior single-transcript and schema changes unapplied. Phase 15.23 through Phase 15.25
adds the real `IConversationStore` boundary with in-memory and SQLite implementations, but the
active chat path still uses `IChatHistoryStore`; no migration, import, cloud sync, tool/plugin
scope, or autonomous conversation orchestration is added. Phase 15.26 through Phase 15.29 activates
safe local session switching over `IConversationStore`: the controller owns a valid active
conversation id, loads selected conversation messages into `ChatSession`, resets live preview and
request metadata on switch, and relies on request-id guards so stale async results from a previous
conversation are ignored. The browser UI is compact and local-only; no delete UI, cloud sync,
import, semantic/vector memory, tools/plugins/system execution, or Ollama safety change is added.
Phase 15.30 through Phase 15.32 add compact Conversation Browser polish and safe delete-readiness
metadata: current/archived state summaries, empty-state copy, archived-send hinting,
`ConversationDeletePolicy`, `ConversationDeleteReadiness`, and `ConversationDeleteResult`.
Permanent delete remains disabled by default; delete requests refuse without storage mutation, and
archive/unarchive remain the supported local removal lifecycle.
Phase 15.33 through Phase 15.35 checkpoint that conversation runtime path: single-transcript
compatibility remains a startup source when the conversation store is empty, SQLite conversation
delete behavior is soft/non-destructive, archived conversations block sends, conversation switching
continues to invalidate request ids so stale async responses are ignored, and permanent delete
requests remain disabled and non-mutating. No semantic memory, embeddings/vector DB, cloud sync,
import/export change, permanent delete execution, UI redesign, model/voice/tool/plugin change, or
runtime authority expansion is added.
Phase 16.0 through Phase 16.6 add controlled semantic memory candidate metadata and an explicit
review lifecycle: candidates can be created from supplied conversation text metadata, default to
Pending Review, and can be approved, rejected, reset to pending, or archived as review metadata
only. `MemoryCandidateReviewResult` records the accepted/refused action, reviewed timestamp,
reviewer/source summary, and decision reason metadata. Approved means reviewed candidate metadata,
not committed long-term memory. The candidate store remains separate from `IMemoryStore`, the
static memory taxonomy, chat history, and conversation storage. No embeddings, vector DB, semantic
search, automatic memory writes, cloud sync, provider/model calls, tool/plugin authority,
filesystem/system authority, or automatic capture loop is added.
Phase 16.7 through Phase 16.9 add approved-memory commit planning metadata:
`MemoryCommitPlan`, `MemoryCommitTarget`, `MemoryCommitReadiness`, `MemoryCommitResult`, and
`MemoryCommitPolicy`. Approved candidates can produce deterministic key-value memory target
summaries, while pending/rejected/archived/missing candidates are blocked with explicit reasons.
The default policy disables actual commit, and commit requests refuse before mutating
`IMemoryStore`. Candidate, approved, planned, and committed are distinct states; committed memory
remains a future explicit phase gate. No embeddings, vector DB, semantic search, automatic memory
writes, cloud sync, provider/model calls, filesystem/system authority, tool/plugin authority, or
autonomous memory mutation is added.
Phase 16.10 through Phase 16.12 add that explicit memory commit gate: approved candidates can be
stored in the existing local key-value `IMemoryStore` only after an explicit user Commit action.
Commit keys are sanitized from candidate category, title, and id; committed values contain only the
reviewed candidate content; duplicate keys are refused by the default conflict policy; and
committed status/key/timestamp summaries are exposed through the controller/view-model boundary.
Approval still does not commit automatically. Pending, rejected, archived, missing,
store-unavailable, already-committed, and duplicate-key requests refuse safely. No embeddings,
vector DB, semantic search, provider/model calls, cloud/API keys, filesystem/system authority
beyond the existing memory store, tools/plugins, overwrite UI, or autonomous memory mutation is
added.
Phase 16.13 through Phase 16.15 add local memory recall metadata and surfacing:
`MemoryRecallQuery`, `MemoryRecallResult`, `MemoryRecallSummary`, `MemoryRecallStatus`, and
`MemoryRecallPolicy` describe deterministic read-only recall over committed key-value memory.
Recall reads `IMemoryStore::entries()` only, performs literal key/value matching, exposes
QML-safe strings/counts/lists, and never injects results into chat prompts. Empty queries report
Empty Query without mutation. No embeddings, vector DB, semantic search, provider/model calls,
cloud/API keys, tools/plugins, filesystem/system actions, or autonomous memory behavior is added.
Phase 16.16 through Phase 16.18 add context assembly planning metadata:
`ContextAssemblyRequest`, `ContextAssemblySource`, `ContextAssemblyResult`,
`ContextAssemblyStatus`, `ContextAssemblyPolicy`, and `ContextAssemblySummary` estimate future
participation for conversation context, committed memory context, runtime metadata context, and
orchestration context. Planning exposes source availability, deterministic source summaries,
candidate block counts, and simple character-size estimates through QML-safe controller/view-model
values. It does not assemble prompts, inject context into chat, attach context automatically, call
providers/models, run semantic ranking, build embeddings/vector indexes, execute tools/plugins, or
perform filesystem/system actions.
Phase 16.19 through Phase 16.21 add safe prompt context injection for local Ollama prompt
requests. `PromptContextBlock`, `PromptContextBundle`, `PromptContextInjectionPolicy`,
`PromptContextInjectionStatus`, and `PromptContextInjectionResult` describe bounded assembled
context and summary-only UI exposure. Injection is disabled by default, requires an explicit
setting, runs only after existing local inference gates pass, and prepends a clearly delimited
context block to the local prompt. Sources are limited to current conversation context, committed
key-value memory, runtime metadata summaries, and orchestration metadata summaries. Pending and
rejected memory candidates are excluded. The block is character-budgeted with deterministic
truncation. No embeddings, vector DB, semantic ranking/search, automatic memory writes, cloud/API
keys, tools/plugins, filesystem/system actions, voice/runtime scope changes, or raw prompt UI
exposure is added.
Phase 16.22 through Phase 16.24 add deterministic conversation window management for that local
prompt context path. `ConversationWindowPolicy`, `ConversationWindowSummary`,
`ConversationWindowResult`, `ConversationWindowStatus`, and `ConversationWindowBudget` describe a
recent-message-first character-budgeted history window. The selected messages are emitted in
chronological order, the current user prompt stays outside the history block, committed memory
remains a separate prompt-context source, and QML receives only budget/status/count summaries. No
semantic ranking/search, embeddings, vector DB, transcript summarization, provider/model change,
cloud/API-key behavior, tools/plugins, filesystem/system actions, broad UI redesign, or raw prompt
display is added.
Phase 16.25 through Phase 16.27 add deterministic conversation summary metadata for older history
omitted by that recent window. `ConversationSummaryPolicy`, `ConversationSummaryStatus`,
`ConversationSummaryResult`, `ConversationSummaryBlock`, `ConversationSummaryWindow`, and
`ConversationSummaryBudget` describe local heuristic compaction with original message indexes,
chronological order, role visibility, and bounded character budgets. Older summaries are injected
as a separate prompt-context block from recent conversation history, committed key-value memory,
runtime metadata, and orchestration metadata. Recent window priority is preserved, committed
memory remains separate, and summary planning/assembly does not mutate chat, memory, candidates,
runtime state, or orchestration metadata. No semantic summarization, embeddings/vector DB,
semantic ranking/search, provider/model calls, cloud/API keys, tools/plugins,
filesystem/system actions, automatic memory writes, broad UI redesign, or raw prompt display is
added.
Phase 16.28 through Phase 16.30 add deterministic retrieval planning over the existing local
context sources. `RetrievalPlanningPolicy`, `RetrievalPlanningStatus`,
`RetrievalPlanningResult`, `RetrievalCandidate`, `RetrievalSourcePriority`, `RetrievalBudget`, and
`RetrievalSelectionSummary` describe source participation, fixed priority, budget allocation,
truncation, selected/excluded counts, and source summaries. Priority is recent conversation,
deterministic older summaries, committed key-value memory, runtime metadata, then orchestration
metadata. Prompt context injection consumes selected retrieval candidates, but planning itself
does not mutate prompts, chat, memory, candidates, runtime state, or orchestration metadata.
Semantic/vector retrieval, embeddings, vector databases, semantic ranking/search, provider/model
calls, cloud/API keys, automatic memory writes, tools/plugins, filesystem/system actions, debug
console UI, broad redesign, raw prompt display, and private assembled payload display remain
future phase gates. Future vector/embedding compatibility must live behind a separate
retrieval/ranking boundary that preserves deterministic planning and source separation.
Phase 16.31 through Phase 16.33 add the embedding/vector abstraction foundation:
`EmbeddingVector`, `EmbeddingDocument`, `EmbeddingRequest`, `EmbeddingResult`,
`EmbeddingProviderStatus`, `EmbeddingProviderPolicy`, `VectorIndexStatus`, `VectorIndexPolicy`,
`VectorSearchQuery`, `VectorSearchResult`, `VectorSearchCandidate`,
`SemanticRetrievalStatus`, and `SemanticRetrievalPolicy`, plus `IEmbeddingProvider` and
`IVectorIndex`. The desktop runtime exposes only disabled semantic-readiness metadata and does not
configure a real embedding provider, real vector index, vector database, or semantic ranker.
`FakeEmbeddingProvider` and `FakeVectorIndex` are deterministic local test fakes only: stable
hash/token-count vectors, in-memory insert/search/remove, deterministic scoring, and no provider
calls, cloud/API keys, filesystem writes, downloads, plugins/tools, or system execution. Retrieval
planning and prompt assembly remain deterministic and unchanged; semantic metadata does not alter
source priority, prompt context, ranking, or injection.
Phase 16.34 through Phase 16.36 add semantic candidate orchestration metadata:
`SemanticCandidate`, `SemanticCandidateSource`, `SemanticCandidateSelection`,
`SemanticCandidateBudget`, `SemanticCandidateWindow`, `SemanticCandidateArbitration`,
`SemanticCandidateSummary`, `SemanticCandidateStatus`, `SemanticCandidatePolicy`,
`HybridRetrievalStatus`, and `HybridRetrievalPolicy`. Candidate orchestration derives
deterministic local metadata from recent conversation windows, deterministic summaries, committed
memory, runtime metadata, orchestration metadata, and a disabled future semantic/vector source.
Arbitration uses fixed ordering, deterministic budgeting, exclusion/truncation metadata, source
isolation, and existing chronology guarantees for conversation-derived sources. Hybrid readiness
reports deterministic retrieval as authoritative and semantic retrieval disabled. Prompt assembly
continues to consume only deterministic retrieval-planning selections; semantic candidates do not
mutate prompts, inject payloads, expose raw vectors/scores, call providers/models, activate vector
databases, run transformer inference, use cloud/API keys, execute plugins/tools, or expand
filesystem/system authority.
Phase 16.37 through Phase 16.39 checkpoint the full memory/context/retrieval architecture before
semantic activation. The checkpoint confirms candidate review/commit/recall boundaries, opt-in
prompt context injection, bounded conversation windows, deterministic summaries, deterministic
retrieval planning, embedding/vector interfaces, semantic candidate orchestration, hybrid
readiness, and QML-safe exposure remain separate. Deterministic retrieval remains authoritative;
semantic retrieval remains disabled; semantic candidates do not mutate prompts; and Phase 17
semantic activation requires a separate concrete provider/indexing/ranking scope with tests for
fallback behavior, privacy/safety gates, committed-memory authority, and QML non-exposure.
Phase 18.0 through Phase 18.3 add the agent task runtime foundation:
`AgentTask`, ids, type/status/priority/source enums, plans, steps, results, traces, safety policy,
runtime status, `IAgentTaskRuntime`, and `StaticAgentTaskRuntime`. The runtime creates
deterministic local metadata for tasks such as summarizing conversation, inspecting memory status,
planning responses, preparing retrieval context, preparing voice responses, and preparing export
actions. It refuses execution by design and does not call tools, plugins, providers, models,
filesystems, shell/subprocesses, background workers, cloud/API services, or autonomous loops.
Controller/view-model exposure remains QML-safe strings, counts, and lists only. Future tool
runtime activation must pass through a separate explicit phase with permission, safety, approval,
sandbox, and UI controls.
Phase 17.0 through Phase 17.3 add semantic provider planning and local selection metadata:
`SemanticProviderDescriptor`, `SemanticProviderSelection`, `SemanticProviderReadiness`,
`SemanticProviderHealth`, `SemanticProviderCapability`, `SemanticProviderPolicy`,
`SemanticActivationReadiness`, and `SemanticActivationResult`. Supported planned modes are
Disabled, Fake/InMemory test provider, Local Ollama embeddings provider, and Local file/vector
index. The desktop default remains Disabled, activation readiness refuses by default, and local
Ollama/file-index providers are planned-only. Deterministic retrieval remains authoritative and
prompt assembly is unchanged. No real embedding calls, vector DB writes, semantic ranking/search,
semantic prompt injection, cloud/API keys, downloads, filesystem scans, tools/plugins, system
actions, or prompt mutation are introduced.
Phase 17.4 through Phase 17.6 add safe hybrid retrieval arbitration simulation and embedding
runtime planning metadata: `SemanticArbitrationPolicy`, `SemanticArbitrationStatus`,
`SemanticArbitrationResult`, `SemanticCandidateScore`, `SemanticBudgetSummary`,
`EmbeddingRuntimePlan`, `EmbeddingRuntimeBudget`, and `EmbeddingRuntimeReadiness`. Simulated
scoring is deterministic metadata only, using fixed source weights, bounded size buckets, stable
source ordering, and candidate-id tie handling. Deterministic retrieval remains the final
prompt-context authority; simulated semantic rankings expose readiness/future-selection summaries
only and cannot mutate `RetrievalPlanningResult`, create prompt blocks, or alter prompt injection.
Embedding runtime planning estimates future local jobs/items and rough memory/storage cost while
reporting local-only requirements and disabled constraints. No embeddings, vector search/vector DB
writes, filesystem indexing, Ollama embedding calls, provider/model inference, cloud/API keys,
tools/plugins, autonomous actions, or prompt mutation are introduced.
Phase 17.7 through Phase 17.9 add an isolated local embedding runtime activation foundation:
`EmbeddingRuntimeStatus`, `EmbeddingRuntimeHealth`, `EmbeddingRuntimeSession`,
`EmbeddingGenerationResult`, `EmbeddingGenerationPolicy`, `EmbeddingGenerationReadiness`, and
`EmbeddingIsolationPolicy`. Isolated generation is permitted only when local-only mode, explicit
semantic readiness, local/fake provider scope, no cloud providers, no filesystem indexing, no
automatic prompt integration, no retrieval ranking mutation, no automatic memory writes, no vector
persistence, and no background indexing gates pass. Fake/InMemory generation is available for
deterministic readiness tests; Local Ollama embeddings remain bounded local-only readiness/runtime
planning and are not wired into semantic retrieval authority. Timeout, stale request, busy state,
provider failure, and refusal outcomes are explicit metadata. Deterministic retrieval and prompt
assembly remain unchanged, and no semantic retrieval, vector search, vector DB write, semantic
prompt injection, cloud/API key, provider download, autonomous action, tool/plugin behavior,
filesystem indexing, or background job is introduced.
Phase 17.10 through Phase 17.12 add local vector persistence lifecycle metadata:
`VectorPersistencePolicy`, `VectorPersistenceStatus`, `VectorPersistenceHealth`,
`VectorPersistenceReadiness`, `VectorPersistenceSession`, `VectorPersistenceBudget`,
`VectorPersistenceResult`, `VectorIndexLifecycle`, and `VectorIndexSnapshotSummary`.
`LocalVectorPersistenceIndex` provides explicit create/reset/clear lifecycle metadata and bounded
acceptance of successful isolated embedding runtime output metadata only. It is local-only,
deterministic, isolated, and disabled by default. Empty indexes, stale sessions, busy sessions,
bounded overflow, and stable snapshots are deterministic outcomes. The desktop runtime exposes
summary/count/readiness metadata only and does not enable automatic indexing, filesystem scanning,
background ingestion, semantic retrieval authority, ranking changes, prompt mutation, automatic
memory conversion, cloud/API/vector services, tools/plugins, autonomous actions, or semantic prompt
injection.
Phase 17.13 through Phase 17.15 add controlled local semantic search activation for readiness
validation and hybrid orchestration testing. `SemanticSearchPolicy`, `SemanticSearchStatus`,
`SemanticSearchResult`, `SemanticSearchCandidate`, `SemanticSearchBudget`,
`SemanticSearchSession`, `SemanticSearchReadiness`, and `SemanticSearchArbitrationSummary` model
a bounded local candidate-search lifecycle. Search reads only local vector persistence entries,
requires local-only deterministic policy gates and isolated embedding runtime output metadata, and
is bounded by candidate count, timeout metadata, similarity range, stale request protection,
busy-state refusal, and deterministic tie handling. Semantic results can produce candidate
metadata, bounded similarity matches, and hybrid arbitration summaries only. Deterministic
retrieval remains authoritative; semantic search does not mutate retrieval planning, prompt
context blocks, prompt assembly, prompt injection, or deterministic ranking. Empty indexes and
disabled persistence are safe fallbacks. Filesystem indexing, background ingestion, cloud/API or
external vector providers, provider downloads, autonomous actions, tools/plugins, semantic prompt
authority, raw vector UI, and debug payload dumps remain out of scope. Future authority activation
requires a separate phase with explicit indexing policy, privacy/safety review, fallback behavior,
prompt-authority tests, and QML non-exposure guarantees.
Phase 17.16 through Phase 17.18 add the bounded hybrid retrieval bridge foundation.
`HybridRetrievalBridgePolicy`, `HybridRetrievalBridgeStatus`, `HybridRetrievalBridgeResult`,
`HybridBridgeCandidate`, `HybridBridgeBudget`, `HybridBridgeReadiness`,
`HybridBridgeArbitration`, and `HybridBridgeSourceSummary` let orchestration read deterministic
retrieval planning output and semantic search candidates as metadata. The bridge produces bounded
merged candidate metadata, deterministic-first arbitration summaries, source participation
summaries, and deterministic fallback summaries only. Deterministic retrieval remains final
authority; semantic candidates are advisory, deterministic candidates win ties/conflicts, and
semantic candidates may only fill unused bounded bridge capacity. Disabled, empty, stale, busy,
timeout, and refused semantic sources keep deterministic retrieval fully functional. The bridge
does not mutate `RetrievalPlanningResult`, create or mutate `PromptContextBlock` values, inject
prompt content, override deterministic ranking, index filesystems, call cloud/API/vector
providers, execute tools/plugins, or expose raw vectors, prompt payloads, provider handles,
filesystem paths, or debug dumps. Future semantic-authority activation requires a separate phase
with indexing policy, privacy/safety gates, fallback tests, mutation tests, prompt-authority
review, and QML non-exposure guarantees.
Phase 17.19 through Phase 17.21 add deterministic semantic acceptance metadata:
`SemanticAcceptancePolicy`, `SemanticAcceptanceStatus`, `SemanticAcceptanceResult`,
`SemanticAcceptedCandidate`, `SemanticAcceptanceBudget`, `SemanticAcceptanceReadiness`,
`SemanticAcceptanceArbitration`, `SemanticAcceptanceFallback`, and
`SemanticAcceptanceSourceSummary`. Acceptance reads deterministic retrieval planning, hybrid
bridge metadata, and semantic search metadata, then approves only bounded semantic supplements that
pass deterministic gates. Deterministic retrieval remains primary and authoritative; semantic
supplements are explicitly marked semantic, local-only, non-authoritative, count bounded,
character-budgeted, and ordered after deterministic candidates. Disabled, empty, stale, busy,
timed-out, refused, errored, or capacity-exhausted semantic sources resolve to deterministic-only
fallback summaries. Acceptance does not mutate `RetrievalPlanningResult`, create or mutate
`PromptContextBlock` values, replace deterministic candidates, alter source priority, expose raw
vectors/prompt payloads, index filesystems, call cloud/API/vector providers, execute tools/plugins,
or grant semantic prompt authority. Future semantic-authority activation still requires a separate
phase with indexing policy, privacy/safety gates, prompt-authority review, fallback tests,
mutation tests, and QML non-exposure guarantees.
Phase 17.22 through Phase 17.24 add semantic supplement prompt assembly readiness metadata:
`SemanticSupplementBlock`, `SemanticSupplementBundle`, `SemanticSupplementAssemblyPolicy`,
`SemanticSupplementAssemblyStatus`, `SemanticSupplementAssemblyResult`,
`SemanticSupplementBudget`, `SemanticSupplementReadiness`, and
`SemanticSupplementSafetyReport`. Assembly reads accepted semantic supplements and can prepare a
separate bounded metadata bundle only through an explicit test-only policy gate. The desktop
default is disabled, live prompt inclusion remains blocked, and prompt assembly continues to use
deterministic retrieval context only. Supplement metadata is count bounded, character-budgeted,
deterministically ordered, deterministically truncated, non-authoritative, and separated from
deterministic context blocks. Disabled, empty, stale, busy, timed-out, and refused semantic states
fall back to unchanged deterministic prompt behavior. Assembly does not mutate
`RetrievalPlanningResult`, create or mutate `PromptContextBlock` values, replace or reorder
deterministic context, override conversation windows, override summaries, override committed
memory, override runtime metadata, expose raw prompt blocks/vectors/scores/provider handles/
filesystem paths/debug dumps, index filesystems, call cloud/API/vector providers, execute
tools/plugins, or grant semantic prompt authority. Future semantic prompt activation still
requires a separate phase with prompt-authority policy, privacy/safety gates, live prompt
inclusion tests, deterministic fallback tests, mutation tests, and QML non-exposure guarantees.
Phase 17.25 through Phase 17.27 add the semantic prompt authority policy foundation:
`SemanticPromptAuthorityPolicy`, `SemanticPromptAuthorityStatus`,
`SemanticPromptAuthorityResult`, `SemanticPromptAuthorityReadiness`,
`SemanticPromptAuthorityDecision`, `SemanticPromptAuthoritySafetyReport`,
`SemanticPromptAuthorityFallback`, and `SemanticPromptAuthorityAuditSummary`. The policy reads
`SemanticSupplementAssemblyResult` only and denies by default. A test-only "would include metadata"
decision requires local-only semantic search, deterministic semantic acceptance, a bounded
supplement bundle, explicit prompt-injection enablement, explicit authority-policy allow, and a
passing safety report. Disabled, stale, busy, timed-out, refused, unsafe, or unbounded states emit
deterministic-only fallback and audit summaries. Live prompt mutation remains blocked, default
prompt assembly is unchanged, and deterministic retrieval remains authoritative. QML receives only
status, decision, safety, readiness, fallback, audit, counts, and checks; it does not receive raw
prompts, supplement blocks, vectors, scores, provider handles, filesystem paths, or debug dumps.
Phase 17.28 through Phase 17.30 add controlled semantic prompt inclusion:
`SemanticPromptInclusionPolicy`, `SemanticPromptInclusionStatus`,
`SemanticPromptInclusionResult`, `SemanticPromptInclusionBudget`,
`SemanticPromptInclusionSafetyReport`, `SemanticPromptInclusionFallback`, and
`SemanticPromptInclusionAuditSummary`. Inclusion is disabled by default and can append semantic
supplements only after deterministic prompt context injection, only when semantic prompt authority
approves, supplement assembly is bounded and safe, local-only mode is active, and safety passes.
The semantic block is clearly delimited as supplemental/non-authoritative and sits after
deterministic context blocks before the user prompt. Deterministic retrieval remains final
authority; semantic supplements cannot replace or reorder deterministic context, override
committed memory, override summaries, override conversation windows, or override runtime metadata.
Disabled, denied, unsafe, empty, stale, busy, timed-out, and refused states fall back to the exact
deterministic-only prompt. QML receives only enabled/status, included count, budget, fallback,
audit, authority-preserved state, and checks; it does not receive raw prompts, supplement content,
vectors, scores, provider handles, filesystem paths, or debug dumps.

Phase 17.31 through Phase 17.33 checkpoint the full Phase 17 semantic architecture. The audit
confirms deterministic retrieval remains final prompt authority, semantic inclusion remains
disabled by default and explicit opt-in, semantic supplements are bounded/local-only/policy-gated
and clearly delimited, and all disabled/denied/unsafe/empty/stale/busy/timed-out/refused semantic
states fall back to deterministic-only prompts. QML exposure remains compact metadata only, with no
raw prompts, supplement content, vectors, scores, provider handles, filesystem paths, or debug
dumps. The checkpoint does not add filesystem indexing, cloud/API/vector provider activation,
provider downloads, tools/plugins, autonomous actions, or runtime authority expansion.

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
- Local inference client: explicit prompt-execution boundary for local runtimes. The current
  Ollama implementation is loopback-only, non-streaming, injectable, and permission/safety-gated by
  `ApplicationController`.
- Local model selection metadata: persisted selected model name, safe effective-model fallback from
  installed-model discovery when available, selected-model summary, and active runtime/model badge.
  This is not model management and cannot download, pull, delete, launch, or install models.
- Local inference streaming boundary: stream chunk/status/error/cancellation/malformed-chunk
  metadata, ordered chunk callbacks, accumulated assistant text, and a loopback-only Ollama
  `/api/generate` stream client. Streaming is disabled by default and guarded by explicit settings
  plus existing model, endpoint, permission, and safety checks.
- Async local inference worker: injectable worker boundary that runs real Ollama non-streaming and
  streaming calls away from the controller/UI thread, posts QML-safe metadata updates back through
  request-id guarded callbacks, and ignores stale results after metadata cancellation.
- Conversation runtime state: controller-owned session read model for current request id, active
  model/route, streaming activity, last success, last error/refusal, and latency. It is UI metadata
  only and does not own chat persistence, route selection, provider execution, tools, or runtime
  workers.
- Conversation history UX metadata: value-only summary of the active single transcript, including
  persistence status, message counts, saved/restored status, and clear result. It does not add
  multiple conversations, transcript browsing, export/import, pruning, encryption, search, or
  storage access outside the existing `IChatHistoryStore`.
- Embedding provider boundary: future local/provider-compatible embedding generation behind
  `IEmbeddingProvider`. The current runtime has no configured provider; the deterministic fake is
  test-only.
- Vector index boundary: future in-memory or vector database-compatible indexing behind
  `IVectorIndex`. The current runtime has no configured vector index and semantic retrieval remains
  disabled.
- Conversation transcript QA and export metadata: literal in-memory search query/results over the
  active single transcript plus controlled Markdown/JSON export result records. Search does not
  mutate history or query storage. Export writes only to the app-controlled export directory and
  does not support import, arbitrary paths, file pickers, or multi-conversation workflows.
- Semantic memory candidates: reviewable metadata records derived from supplied conversation text
  metadata. They default to Pending Review, expose a guarded approve/reject/reset/archive review
  lifecycle, and remain separate from key-value memory, memory taxonomy, embeddings, vector
  storage, semantic search, provider/model execution, and autonomous memory writes.
- Memory commit boundary: approved-candidate to key-value-memory plan/readiness metadata plus an
  explicit user Commit action. The request path writes reviewed candidate content to `IMemoryStore`
  only after review, policy, availability, and duplicate-key checks pass; otherwise it refuses with
  QML-safe result metadata. Future phases may define overwrite controls, durable candidates, and
  mutation tests.
- Local memory recall: deterministic literal key/value matching over committed `IMemoryStore`
  entries. Current recall is read-only UI metadata and is not semantic recall or prompt injection.
  Future phases may define semantic recall, ranking, embeddings/vector indexes, and controlled
  context assembly.
- Context assembly planning: deterministic readiness metadata for future conversation, committed
  memory, runtime metadata, and orchestration context participation. Current planning only counts
  and summarizes candidate blocks and does not build prompts, inject context, attach context
  automatically, call providers/models, rank semantically, or use embeddings/vector indexes.
- Explicit local chat inference routing: persisted opt-in that lets chat use the local inference
  boundary only after model, endpoint, permission, and safety checks. Disabled remains the default.
- Local model management readiness: deterministic metadata for recommended local models,
  approximate RAM/disk requirements, and unavailable pull/delete/install actions. This does not own
  installed-model discovery, downloads, deletion, installation, subprocesses, filesystem/system
  scans, cloud calls, API keys, tools/plugins, or autonomous loops.
- Voice provider boundary: future TTS/STT provider interfaces plus disabled null providers and
  readiness metadata. Piper should enter only through `ITextToSpeechProvider`, and Whisper should
  enter only through `ISpeechToTextProvider`, after a later explicit phase defines audio
  permissions, local model ownership, playback/capture lifecycle, cancellation, and safety checks.
- Voice runtime coordinator: future voice session/pipeline ownership surface. The current
  implementation reports runtime unavailable, TTS/STT unavailable, microphone disabled, playback
  disabled, local-only policy active, and process execution disabled while emitting deterministic
  metadata-only pipeline traces.
- Voice checkpoint and local integration plan: Phase 12 closes with a documented readiness gate for
  future Piper and Whisper work. Piper must remain behind `ITextToSpeechProvider`; Whisper must
  remain behind `ISpeechToTextProvider`; both require explicit permission, lifecycle,
  cancellation, local binary/model ownership, and safety decisions before execution.
- Voice runtime environment: metadata-only ownership for expected future Piper/Whisper binary
  paths, voice model paths, runtime permissions, and safety checks. Current null/static
  implementations report missing/not-configured binaries and models, denied permissions, and
  blocked execution without launching processes, scanning the filesystem broadly, loading models,
  opening audio devices, downloading assets, or calling cloud services.
- Piper TTS adapter boundary: typed Piper TTS configuration, voice model descriptor, request,
  result, status, client, null client, process client, and provider metadata. The current TTS path
  is `text -> Piper provider -> gated file-output metadata`. The current provider is disabled/not
  configured by default until the persisted Piper file-output opt-in and explicit user action are
  both present, refuses missing or invalid binary/model metadata deterministically, and permits
  local file output only inside an app-controlled output directory after explicit
  process-permission and safety gates pass.
- Local voice configuration UX: persisted Piper binary/model and Whisper binary/model path strings,
  exact configured-path metadata checks, fixed-location binary hints, configured-path model hints,
  and Settings visibility. This is configuration only; it does not run Piper or Whisper, load
  models, open microphones, play audio, download assets, scan directories recursively, apply hints
  automatically, use API keys, or start autonomous voice behavior.
- Voice path readiness refinement: explicit Apply Paths persistence, exact validation rows, and
  Ready/Blocked/Missing preparation status for Piper file-output TTS and future Whisper STT. Piper
  readiness here means configured local paths are suitable for a later controlled file-output
  phase; it does not bypass the existing provider/client permission and safety gates.
- Controlled Piper file-output execution: persisted disabled-by-default opt-in, explicit Generate
  TTS File action, controlled cache/temp output path, timeout/failure/status metadata, and
  generated path summary only. Playback, microphone capture, Whisper execution, arbitrary output
  paths, downloads, cloud/API keys, filesystem-wide scans, and autonomous loops remain excluded.

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

Current Phase 15.35 runtime activates controlled local Ollama chat inference while keeping the
larger orchestration system bounded. Sentinel allows loopback-only Ollama health/discovery,
selected-model metadata, explicit opt-in chat-to-Ollama routing, guarded local-only streaming,
action-light local model selection UX, metadata-only model-management readiness, metadata-only
voice provider/session/runtime boundaries, metadata-only local voice runtime environment ownership,
controlled Piper file-output synthesis behind persisted opt-in and explicit action, and local
Piper/Whisper path configuration UX with hint-only fixed-location suggestions plus explicit
Ready/Blocked/Missing path preparation metadata. The Ollama path reports bounded timeout/error
metadata, runs real generate/stream work through an async worker boundary, and clears failed
streaming previews without persisting partial assistant output. Conversation runtime summaries are
request-id guarded and reset on Clear Chat or conversation switch alongside active transcript
loading. Conversation history UX metadata now has a compact local conversation browser over
`IConversationStore`, with create/switch/rename/archive/unarchive actions and controlled
Markdown/JSON export still scoped to the currently visible transcript only. Permanent delete
remains disabled and non-mutating, archived conversations remain send-blocked until unarchived,
and stale async completions after switching remain ignored. Phase 15.35 still adds no audio
playback, microphone access, autonomous voice loop, cloud voice calls, API keys, model downloads,
Whisper execution, autonomous agents, tool execution, shell execution, filesystem-wide actions,
vector search, embeddings, semantic search, SQLite FTS, file picker, import, permanent-delete
execution, or arbitrary export paths:

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
- `IRuntimePermissionPolicy` owns runtime permission decisions. `StaticRuntimePermissionPolicy`
  still denies execution-level runtime permissions by default for metadata-only/test scenarios.
  `LocalOnlyRuntimePermissionPolicy` is used by the desktop app to allow only explicit local
  inference and to deny provider invocation, tool invocation, external process, filesystem access,
  broader network access, and plugin invocation.
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
- `ILocalInferenceClient` owns the controlled local inference boundary. `NullLocalInferenceClient`
  refuses deterministically, and `OllamaLocalInferenceClient` may call only local loopback
  non-streaming `/api/generate` after the controller has evaluated runtime permission and safety
  metadata. It rejects blank prompts, missing or unavailable models, invalid endpoints, streaming
  requests, and cancellation metadata safely.
- Local model selection remains metadata/configuration only. `ApplicationController` resolves an
  explicit model first, then the selected local model, then a safe discovered-model fallback when
  model metadata is available. Known invalid selections are rejected before inference. No downloads,
  pulls, deletes, or model-management actions exist.
- Settings can display discovered local model names and metadata summaries, select one discovered
  model, persist that selection, and show Missing, Fallback, Available, Unverified, or Invalid
  state. Model summaries include name, size when available, modified date when available, and Local
  Only status. Recommendation summaries may appear near selection, but they do not trigger
  downloads, installs, pulls, deletes, or scans.
- Local chat inference routing is disabled by default. When enabled, `ApplicationController`
  appends the user message, calls `runLocalInference` only if the effective model is valid, the
  Ollama endpoint is loopback HTTP, and runtime permission/safety checks pass, then appends exactly
  one assistant response from the inference result or a safe refusal/error. Refusal/error summaries
  are user-readable and technical enough for diagnosis, but do not expose stack traces, secrets,
  filesystem paths, raw internal objects, provider credentials, or broad endpoint details.
- `ILocalInferenceStreamClient` owns local streaming shape and ordered chunk callbacks.
  `NullLocalInferenceStreamClient` reports deterministic disabled behavior, and
  `OllamaLocalInferenceStreamClient` may call only loopback HTTP `/api/generate` streaming with no
  redirects, cloud endpoints, API keys, downloads/pulls/deletes, subprocess launch, tools, or
  plugins.
- Local chat streaming is disabled by default. When enabled with local chat inference, successful
  chunks update QML-safe live response text only while streaming is active. Completion clears the
  live preview and persists exactly one final assistant message through chat history; malformed
  chunks, cancellation, timeout, refusal, and errors are summarized safely.
- `IModelManagementService` owns future model-management readiness metadata only.
  `StaticModelManagementService` returns deterministic recommendations and approximate descriptive
  requirement summaries. Pull, delete, and install requests return not implemented/unavailable
  results and perform no actions.
- `ITextToSpeechProvider` and `ISpeechToTextProvider` own future voice provider boundaries only.
  `NullTextToSpeechProvider` and `NullSpeechToTextProvider` report disabled metadata and return
  safe refusals. No microphone, playback, Piper, Whisper, subprocess, filesystem/system action,
  download, cloud call, API key, or voice UI control is active.
- `IVoiceRuntimeCoordinator` owns future voice runtime/session orchestration metadata only.
  `StaticVoiceRuntimeCoordinator` emits deterministic session status, runtime summaries, and
  pipeline traces for idle, preparing, awaiting-input, transcribing-placeholder,
  inference-placeholder, synthesis-placeholder, completed, blocked, and error states. It does not
  open microphones, play audio, execute Piper or Whisper, launch subprocesses, touch filesystems,
  call cloud providers, use API keys, or run autonomous voice loops.
- `docs/PHASE_12_CHECKPOINT.md` records the Phase 12 voice architecture review and Phase 13
  readiness criteria. Future Piper/Whisper integration must stay local-first, injectable,
  permission/safety-gated, and separate from chat providers, local inference clients, tools,
  plugins, memory, and model management.
- `IVoiceRuntimeEnvironment` owns future local voice runtime environment metadata only.
  `NullVoiceRuntimeEnvironment` and `StaticVoiceRuntimeEnvironment` describe Piper/Whisper
  binary/model readiness, denied runtime permissions, and blocked safety posture. They do not
  execute Piper or Whisper, launch subprocesses, load models, scan the filesystem broadly, open
  microphones, play audio, download assets, call cloud providers, use API keys, or add voice
  controls.
- `PiperTextToSpeechProvider` owns the current Piper TTS adapter skeleton behind
  `ITextToSpeechProvider`. `NullPiperTtsClient` refuses synthesis deterministically, and missing
  or invalid Piper binary/model metadata is rejected before any client boundary.
  `ProcessPiperTtsClient` may launch Piper only for explicit local file output after the persisted
  opt-in is enabled and the provider accepts enabled configuration, executable binary, readable
  model, controlled output path, local-only request, process permission, and safety gates. The
  adapter does not play audio, open microphones, scan broadly, download assets, call cloud
  providers, use API keys, expose playback controls, or add path/model pickers.
- Voice Configuration in Settings persists Piper binary path, Piper model path, Whisper binary
  path, and Whisper model directory/path as strings. The controller validates only those exact
  paths for exists/missing, readable/unreadable, and executable/non-executable binary metadata.
  The controller also exposes read-only hints from only `/opt/homebrew/bin/piper`,
  `/usr/local/bin/piper`, `/opt/homebrew/bin/whisper`, `/usr/local/bin/whisper`, and already
  configured model paths. The validation and hints do not execute binaries, load models, inspect
  model contents, recurse through directories, auto-write settings, download assets, open audio
  devices, call cloud services, or start background work.
- `docs/PHASE_13_CHECKPOINT.md` records the Phase 13 Voice/Piper review, confirms the TTS path as
  `text -> Piper provider -> gated file-output metadata`, and marks Phase 14 ready only for
  explicit planning or configuration-readiness work unless a later phase separately authorizes
  playback, microphone capture, Whisper execution, downloads, cloud/API-key behavior, or
  autonomous voice loops.
- `AppSettings` persists the routing mode and normalized Ollama endpoint through
  `JsonSettingsStore`; it also persists the selected local model name, local chat inference
  opt-in, and local streaming opt-in. It does not store provider credentials or API keys.
- Tool planning, approval, sandbox, and execution boundaries remain non-operational.
- `NullAgentRuntime` and `NullToolExecutor` still perform no real AI/model/tool execution.

Cloud routing, credentials, model downloads, model pulls/deletes/installs, autonomous agent
runtime, semantic/vector memory, actionable model-management operations, and automatic routing
policy automation remain future work.

## Phase 10 Direction

Phase 7.0 starts with a local runtime boundary skeleton, not full provider/model execution. Phase
7.2 adds capability negotiation vocabulary. Phase 7.3 through Phase 7.5 add metadata-only
permission, request pipeline, and safety policy boundaries while keeping execution blocked by
default. Phase 7.6 checkpoints the runtime architecture in `docs/PHASE_7_CHECKPOINT.md`. Phase
8.0 through Phase 8.2 add execution lifecycle/session coordination metadata only. Phase 8.3 through
Phase 8.5 add adapter, bridge, and pre-integration readiness metadata only. Phase 9.0 through Phase
9.2 add Ollama local health/discovery only. Phase 9.3 through Phase 9.5 add the first controlled
local-only inference boundary. Phase 9.6 through Phase 9.8 add selected-model metadata, runtime UX
state, and a disabled streaming skeleton. Phase 10.0 through Phase 10.2 add explicit opt-in
chat-to-Ollama routing. Phase 10.3 through Phase 10.5 add guarded local-only streaming chat and
live response UX. Phase 10.6 through Phase 10.8 add local model selection and runtime management
UX while keeping cloud provider integration, credentials, downloads, model management operations,
tool execution, plugins, vector memory, and autonomous behavior out of scope.
