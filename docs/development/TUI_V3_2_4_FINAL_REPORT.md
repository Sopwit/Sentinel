# SENTINEL — V3.2.4 FINAL REPORT

Verdict: **IMPROVED — FURTHER FIXES REQUIRED**. Classification now fails finitely and safely; live grounded read-task completion remains unaccepted. No V3.3 work or full edit/build/test acceptance is claimed.

## 1. Root causes

The classifier used a plain request with no serialized local output limit and a zero default deadline. The base provider interface could silently discard unsupported options. Separately, filesystem final canonicalization replaced explanations with receipts before authoritative acceptance. Repeated reads were observed historically, but tool-result loss is not established as their sole cause.

## 2. Classification changes

Classification requests explicitly reserve 512 output tokens and a 30000-ms deadline. Selected endpoint/Ollama providers carry those bounds; unsupported base providers fail closed. Plain local requests serialize max_tokens, retain actual HTTP status, reject length-finished responses and expose bounded allowlisted metadata. Bounded JSON requests make one attempt, preventing retry lifetime expansion. Indeterminate classification terminates both AgentLoop entry modes before tools. Cancellation retains the actual terminal category; client reply abort does not prove upstream generation stopped.

## 3. Provider behavior

Actual task-owned instances report 8192 context/four slots. Qwen's five real TUI journeys fail classification at roughly 31 seconds, safely, with zero tools. Nemotron's normal classification succeeds in 8.506 seconds with HTTP200/stop and measured usage 392/110 tokens; the run answers `4` and completes at 58.11 seconds. Its four read-task classifications time out before reads. Underlying provider delay and upstream cancellation remain unknown. No silent fallback, model download or user provider-setting change occurred; initially unloaded state was restored.

## 4. Grounded final-answer behavior

A bounded typed projection preserves source-based interpretations linked to exact quotes and current-run call IDs. The daemon derives paths, rejects invented/denied/stale references, and publishes labelled interpretations with observed provenance only after AgentLoop acceptance. Symbolic claims and mutations retain stricter existing policy; receipts remain supported. Citation validation is not a semantic success evaluator. The deterministic real read gateway passes; live read-task explanation acceptance is not established because classification fails first. Generic receipt conversion can still obscure task relevance outside the typed path.

## 5. Repeated-read recovery

A bounded inventory lists latest unique successful reads and repeated-read count, guiding continuation toward missing evidence. Existing full observations, correlated native results, head/tail truncation, context preflight, doom-loop limits and bounded repair remain. Legitimate rereads are not globally suppressed and writes are not automatically replayed. Real recovery after multiple reads is NOT VALIDATED in this batch.

## 6. Real Agent acceptance

Ten actual TUI journeys across two real models: one normal conversational completion, nine safe classification failures, zero tool results, zero approvals/edits, all fixture hashes unchanged. A/B/E/F are not accepted source-analysis journeys. Deterministic fixtures are recorded separately. Physical terminal-host acceptance and complete real-TUI deterministic stall/cancel acceptance remain NOT VALIDATED.

## 7. Keychain test status

The upgrade test's fixed master-key lookup entered SecItemCopyMatching on the personal Keychain. An injectable key source now lets its macOS test exercise real AES encryption, nonplaintext ciphertext and roundtrip with a disposable test key. Production still defaults to the OS key source. test_upgrade and the complete suite pass. This is not OS Keychain integration acceptance; personal data, Keychain permissions and credential prompts were not changed or approved.

## 8. Remaining macOS execution blocker

Safe build execution and cancellation of compiler descendants remain BLOCKED by existing executable/dependency confinement and deny-fork protections. V3.2.3 containment design remains SPECIFIED ONLY. No sandbox weakening, alternate Rust shell path or Linux-success inference was introduced.

## 9. Regression tests and presentation

Final C++/IPC suite: **122/122 PASS**. Focused six-executable regression set passes. Rust: **75 tests PASS**, formatting, warnings-denied Clippy and debug/release builds pass. C++ tests/release builds and checked formatting pass. Initial fixture/socket/cache failures are retained in evidence; they are not reported as green runs.

Limited TUI corrections replace stale cancellation-waiting text on terminal events, normalize the known daemon failure Markdown wrapper while retaining literal code, and ellipsize long notice previews with /notices access. Keyboard contract, themes, sessions and layout remain. Raw approval argument presentation was not expanded because the authoritative IPC projection does not supply that safe capability. Late wrapper/ellipsis fixes have deterministic coverage only.

## 10. Final verdict and provenance

**IMPROVED — FURTHER FIXES REQUIRED**. Finite classification safety and scoped deterministic grounded projection improved; successful live read classification, explanation quality, repeated-read recovery and context-pressure completion remain open. This phase is not ACCEPTED.

Uncommitted V3.2.3 work was preserved. External commits appeared in the shared checkout during execution (ending at observed 144ef090); this agent issued no commit, push or merge commands and did not undo them. Both live profiles used the same frozen release daemon hash, matching the release build recorded in evidence. Late changes after freezing concern regression fixtures and TUI presentation, not daemon inference behavior.

## 11. Recommended next phase

Continue this reliability work before V3.3: investigate classifier latency under the recorded actual limits; choose and verify a usable finite classification contract without provider switching or heuristic authorization; then repeat grounded A/B/E/F and deterministic TUI stall/cancel journeys. Strengthen explicit response-purpose/relevance handling so receipts cannot stand in for explanations. Separately review macOS descendant containment and obtain an explicitly authorized Linux runner for independent execution acceptance. Keep Keychain integration testing separate from hermetic crypto regression coverage.

[Classification reliability](TUI_V3_2_4_CLASSIFICATION_RELIABILITY.md) · [Grounded completion](TUI_V3_2_4_GROUNDED_COMPLETION.md) · [Acceptance matrix](TUI_V3_2_4_ACCEPTANCE_MATRIX.md) · [Evidence](../reviews/agent-v324-2026-10-10/README.md)
