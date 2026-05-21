# Phase 17 Semantic Retrieval Checkpoint

Phase 17.31-17.33 completes the semantic retrieval and prompt inclusion checkpoint for the
current Desktop Alpha architecture.

## Completed Phase 17 Scope

- Added semantic provider planning and local provider selection metadata.
- Added isolated local embedding runtime readiness helpers for deterministic tests.
- Added disabled-by-default local vector persistence lifecycle metadata.
- Added bounded local semantic search over local vector persistence entries only.
- Added a deterministic-first hybrid retrieval bridge.
- Added deterministic semantic acceptance for supplemental semantic candidates.
- Added semantic supplement assembly as bounded metadata.
- Added semantic prompt authority policy and controlled prompt inclusion gates.
- Exposed compact QML-safe semantic statuses, summaries, counts, budgets, fallback summaries,
  audit summaries, and safety checks.

## Active Capabilities

- Deterministic retrieval planning remains active and authoritative for prompt context selection.
- Prompt context injection remains opt-in through the existing local context setting.
- Semantic prompt inclusion has a live code path, but it is disabled by default and requires
  explicit opt-in plus all policy gates.
- Semantic search can run only as bounded local metadata over local vector persistence entries in
  controlled/test paths.
- Fake/InMemory embedding and vector behavior remains deterministic test infrastructure.

## Inactive Capabilities

- No desktop default semantic provider is active.
- No filesystem indexing, background indexing, or background ingestion is active.
- No cloud/API embedding provider, external vector provider, API key path, provider download, or
  model-management action is active.
- No tools, plugins, autonomous actions, process launch, or system action is introduced by the
  semantic path.
- No semantic supplement can replace deterministic retrieval, reorder deterministic context, or
  override committed memory, conversation windows, deterministic summaries, or runtime metadata.

## Deterministic Authority Guarantees

- `RetrievalPlanningResult` remains the final authority for deterministic prompt context.
- Hybrid bridge arbitration fills bounded metadata capacity with deterministic candidates first.
- Semantic acceptance can approve only supplemental semantic candidates after deterministic
  candidates and within count/character budgets.
- Supplement assembly produces separate metadata bundles and does not mutate deterministic
  retrieval planning or `PromptContextBlock` values.
- Prompt inclusion appends semantic supplements only after deterministic context and before the
  user prompt, preserving deterministic context order.
- Disabled, denied, unsafe, empty, stale, busy, timed-out, and refused semantic states return the
  deterministic-only prompt.

## Semantic Inclusion Safety Gates

Semantic prompt inclusion requires all of the following:

- semantic inclusion explicitly enabled
- prompt context injection enabled
- local-only mode active
- semantic prompt authority approved
- bounded and safe supplement assembly
- deterministic retrieval authority preserved
- supplemental-only and clearly delimited semantic block
- deterministic context replacement and reordering blocked
- committed-memory, summary, conversation-window, and runtime-metadata overrides blocked
- raw prompt payload, vector, score, provider-handle, filesystem-path, and debug-dump exposure
  blocked

## Default Policy

- `AppSettings::semanticPromptInclusionEnabled()` defaults to false.
- `SemanticPromptInclusionPolicy::enabled` defaults to false.
- `SemanticPromptAuthorityPolicy::enabled` defaults to false.
- `SemanticSupplementAssemblyPolicy::enabled` defaults to false.
- Default local prompt assembly is deterministic-only.

## Privacy And QML Exposure Rules

- QML receives view-model strings, string lists, booleans, and counts only.
- Memory and Settings surfaces may show semantic status, readiness, budgets, counts, fallback,
  audit, and deterministic-authority-preserved summaries.
- QML does not receive raw prompts, raw semantic supplement content, raw vectors, raw scores,
  provider/index handles, filesystem paths, or debug payloads.
- Internal prompt strings inside controller/result records are not exposed as QML properties.

## Fallback Behavior

- When semantic inclusion is disabled, the prompt remains the deterministic prompt.
- When context injection is disabled, semantic inclusion also falls back to deterministic-only
  prompt behavior.
- When authority, assembly, search, bridge, or acceptance reports unsafe, stale, busy, timed-out,
  refused, empty, or denied states, semantic inclusion returns deterministic-only prompt output.
- Fallback metadata reports why semantic supplements were excluded without surfacing private
  prompt payloads.

## Known Limitations

- Semantic systems are still readiness/checkpoint infrastructure, not a full semantic retrieval
  product feature.
- Semantic search has no desktop default index to query.
- Vector persistence is local lifecycle metadata and controlled test infrastructure, not a durable
  production vector database.
- There is no indexing policy for app data beyond explicit local vector persistence test paths.
- Semantic supplements depend on deterministic acceptance and do not grant semantic authority over
  prompt construction.
- QML exposes compact status only; there is no semantic debug console.

## Phase 18 Readiness Criteria

Phase 18 may proceed only if it keeps these guarantees or explicitly scopes and tests any change:

- deterministic retrieval remains authoritative or any authority change is explicitly reviewed
- semantic inclusion remains disabled by default unless a later phase deliberately changes policy
- any real semantic provider stays local-only behind existing interfaces
- any indexing policy is explicit, bounded, user-visible, and app-owned
- deterministic-only fallback tests cover all semantic failure states
- QML non-exposure tests continue to block raw prompts, supplement content, vectors, scores,
  provider handles, filesystem paths, and debug dumps
- no cloud/API provider, filesystem indexing, tool/plugin execution, autonomous action, or runtime
  authority expansion is introduced without a separate phase gate

## Readiness Verdict

Phase 17 is ready to close. The semantic architecture is bounded, local-only, default-disabled,
policy-gated, and subordinate to deterministic retrieval. Phase 18 can build on this checkpoint
only by preserving deterministic fallback behavior and the QML/privacy boundaries above.
