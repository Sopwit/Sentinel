# Phase 16 Memory And Context Checkpoint

Phase 16.37-16.39 completes the memory/context/retrieval activation-readiness checkpoint for the
current Desktop Alpha architecture.

## Completed Phase 16 Scope

- Added controlled memory candidates, review actions, and explicit commit into local key-value
  memory.
- Added literal read-only recall over committed memory.
- Added context assembly planning, opt-in prompt context injection, bounded conversation windows,
  deterministic older-conversation summaries, and deterministic retrieval planning.
- Added embedding/vector interfaces and deterministic fake test layers.
- Added semantic candidate orchestration and hybrid retrieval readiness metadata without enabling
  semantic retrieval.
- Exposed compact QML-safe statuses, counts, summaries, readiness checks, and disabled-state
  metadata.

## Architecture Boundaries

- `IMemoryStore` remains the only committed key-value memory persistence boundary.
- `IMemoryCandidateStore` owns reviewable candidate metadata separately from committed memory.
- `IEmbeddingProvider` and `IVectorIndex` are future semantic retrieval boundaries only; the
  desktop runtime does not configure real implementations.
- Deterministic retrieval planning remains the authoritative prompt-context source selector.
- Semantic candidate orchestration is readiness metadata and does not feed prompt assembly.
- QML receives view-model strings, string lists, booleans, and counts only. It does not receive raw
  prompts, raw vectors, vector scores, provider/index handles, or private candidate payloads.

## Memory Lifecycle

- Candidate creation produces Pending Review metadata from supplied conversation text.
- Approve, reject, reset, and archive update candidate review metadata only.
- Approved means reviewed, not committed.
- Commit is a separate explicit user action that writes only approved candidate content to
  `IMemoryStore`.
- Pending, rejected, archived, missing, unavailable-store, already-committed, and duplicate-key
  requests refuse before storage mutation.
- Local recall reads committed `IMemoryStore` entries only and performs literal key/value matching.
- Clearing chat does not clear approved candidate metadata or committed key-value memory.

## Context Assembly Lifecycle

- Context assembly planning estimates participation for conversation context, committed memory,
  runtime metadata, and orchestration metadata.
- Prompt context injection is disabled by default and requires the explicit "Use local
  memory/context in chat" setting.
- Injection runs only after local inference busy/model/endpoint/permission/safety gates pass.
- The assembled prompt context is delimited, character-budgeted, and source-separated.
- Pending, rejected, archived, and merely approved memory candidates are excluded from injection.

## Retrieval Planning Lifecycle

- Conversation history is bounded by a recent-message window.
- Older omitted messages can be represented by deterministic local summary blocks.
- Retrieval planning selects among recent conversation, deterministic summaries, committed memory,
  runtime metadata, and orchestration metadata in fixed priority order.
- Budgeting, truncation, selected/excluded counts, and source summaries are deterministic.
- Prompt context injection consumes selected deterministic retrieval candidates only.

## Semantic Disabled Guarantees

- `SemanticRetrievalPolicy::enabled` remains false in the desktop runtime.
- Embedding provider readiness is Not Configured.
- Vector index readiness is Not Configured and indexed item count is zero.
- Real embeddings, transformer inference, semantic ranking/search, vector database activation,
  provider/model calls, cloud/API keys, plugins/tools, filesystem/system actions, and semantic
  prompt injection remain out of scope.
- Fake embedding/vector implementations are deterministic test layers only.

## Prompt Injection Safety

- Semantic candidates do not mutate prompts.
- Hybrid readiness reports deterministic retrieval as authoritative.
- Semantic/vector candidates are represented only as disabled readiness metadata.
- Raw assembled prompts and private prompt payloads are not exposed to QML.
- Prompt context injection remains opt-in and local-only.

## Known Limitations

- Semantic retrieval is not active.
- Memory candidate storage is runtime/in-memory only unless a later phase adds durable candidate
  persistence.
- Recall is literal key/value matching only.
- Conversation summaries are deterministic local compaction, not model-generated summaries.
- Retrieval planning is character-budgeted deterministic source selection, not semantic ranking.
- There is no vector database, real embedding provider, semantic index maintenance, or semantic
  activation UI.

## Phase 17 Readiness Criteria

Phase 17 may begin real semantic activation only after a separate scope explicitly provides:

- A concrete local-only embedding provider implementation behind `IEmbeddingProvider`.
- A concrete vector index or vector database implementation behind `IVectorIndex`.
- Indexing policy for which committed local records may enter semantic retrieval.
- A migration/refresh strategy that keeps committed key-value memory authoritative.
- A deterministic fallback path where retrieval planning still works without semantic services.
- Permission, privacy, and safety checks before any semantic candidate can affect prompts.
- Tests proving semantic ranking cannot inject pending/rejected/uncommitted candidates.
- Tests proving raw vectors, scores, payloads, provider handles, and private prompts stay out of
  QML.
- Tests proving semantic activation does not call cloud providers, use API keys, execute tools, or
  expand filesystem/system authority.

## Readiness Verdict

Phase 16 is ready to close. The architecture is prepared for a future semantic activation phase,
but Phase 17 is not authorized to enable semantic retrieval until the criteria above are scoped,
implemented, and tested.
