# V3.2.4 grounded completion

Status: **PARTIAL — deterministic explanatory projection passes; live read-and-explain acceptance is NOT VALIDATED.**

## Root cause

V3.2.3 Nemotron successfully observed calc.h, calc.cpp and test.cpp, then reached run.completed with receipts/excerpts instead of the requested explanation. Source inspection identifies an authoritative workflow cause: `LlmAgentRuntime` replaced filesystem model finals with `ClaimGroundingResolver::filesystemFinalAnswer`, and AgentLoop required equality with that scoped representation. A fluent explanation could therefore be discarded regardless of provider quality. Provider behavior and repeated reads remain separate contributors; the historical traces do not establish loss of every tool result.

Native continuation already returns correlated tool results with status and bounded beginning/end excerpts, while full structured observations stay in run evidence. Doom-loop and two-final-rejection limits already exist. They remain unchanged.

## Typed explanatory projection

For read-only file-content analysis, a final can use:

```json
{"explanations":[{"interpretation":"add subtracts its second argument from the first.","evidence":[{"call_id":"current-run-call-id","quote":"return a - b;"}]}]}
```

The resolver requires a current-run successful FileContent observation, nonempty exact source quotation and matching call ID. It derives the source path from actual structured evidence; a supplied source must match. Superseded observations, denied/failed calls, invented quotes and foreign IDs are rejected. Runtime canonicalization preserves the interpretation and verified source references. Only AgentLoop's accepted final renders `Interpretation: …` followed by labelled observed quotes and provenance.

This validates provenance, **not arbitrary semantic truth or task success**. An explanation is explicitly an interpretation of source. It cannot certify test outcomes, execution or broad absence. Symbolic existence/absence claims and operation/mutation evidence retain the existing stricter representation; this path cannot bypass them. Receipt requests remain supported by the unchanged receipt path.

Bounds: input envelope 16384 characters; up to eight interpretations of at most 1500 characters; one to four quotations per interpretation, each at most 512 characters. Invalid attempted explanation envelopes enter the existing bounded repair path rather than silently becoming receipts. No extra independent synthesis agent, compaction subsystem or unbounded tool retry was added. Native serialization preflight includes the additional prompt material and retains its existing conservative byte estimate/output reservation.

## Continuation and repeated reads

Continuation prompts now include up to eight latest unique successful reads with their current-run IDs, scoped resources and step indices, plus a repeated-read count. The instructions distinguish answering the task from listing accessed files, direct the model to missing evidence, and permit a specific freshness/narrower-range reread. This inventory is model context, not permission or evidence by itself; source strings are explicitly data. No global read suppression, mutating retry or doom-loop weakening was introduced.

The inventory cannot restore omitted content or make a heuristic context estimate exact. Existing head/tail result truncation and full evidence retention remain. General fallback-provider context accounting and native request wall-time limits are not newly solved by this projection.

## Verification and limits

Deterministic tests reject invented/empty quotes, foreign IDs, denied reads and superseded evidence, and verify canonical roundtripping. A synthetic real read gateway followed by a scripted typed final reaches authoritative completion and displays the actual subtraction explanation/source quote. That is workflow validation, not live-model reasoning.

The first gateway fixture failed because macOS canonicalized the temporary path; the fixture was corrected to use the actual canonical source, preserving strict production validation. The full C++ suite passes afterward.

The live A/B/E/F journeys produced zero reads on both models because classification timed out first. Therefore neither grounded explanation quality nor repeated-read recovery is live-accepted. The normal Nemotron arithmetic answer does not substitute for source-based explanation acceptance. The old generic filesystem-final path can still produce receipts if the model does not choose the explanatory contract; request-relevance acceptance needs further work. No success evaluator was added to hide that limitation.
