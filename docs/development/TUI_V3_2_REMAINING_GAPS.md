# TUI V3.2 remaining gaps

Scope: reliability of the existing daemon Agent loop. This backlog does not enable new commands or permissions.

| Gap | Required work / acceptance |
|---|---|
| Runtime context capacity — PARTIAL | V3.2.1 now discovers the actual loaded 8192-token window, bounds native result excerpts, reserves native output and preflights serialized request size. A measured two-file task completes; a similar-sized request still receives provider context exhaustion. Reliable tokenizer/accounting, loaded-limit refresh and authoritative compaction remain deferred. No model/server settings changed. |
| Natural multi-file coding autonomy — NOT ACCEPTED | Repeat the recorded synthetic levels across models; distinguish observed evidence from a useful explanation. A small local model completing one file read does not establish general coding capability. |
| Evidence-derived filesystem final presentation | Current policy replaces filesystem prose with scoped facts/excerpts. This prevents unsupported claims but can obscure multi-file explanations. A future typed claim/evidence design is needed; do not relax grounding based on fluent prose. |
| Native repair live acceptance | New native-call repair paths have deterministic tests. The live run did not deliberately exercise malformed/unknown native calls, so those repair tests are not live provider acceptance. Catalog capability flags alone do not prove a protocol-repair path ran. |
| Large extension tool catalogs | The compact tool index is bounded. Built-in catalog coverage is tested; arbitrarily large plugin/MCP catalogs still need paged authoritative discovery/schema retrieval. |
| Interrupted mutating operations — DEFERRED | No new automatic retry added. V3.2.1 direct D requested the failed configure command twice with fresh approvals; repeated model-initiated commands across observations still need outcome-aware guarding. Add durable operation IDs, partial-effect evidence and explicit review before supporting resumable writes/processes. |
| Detailed safe execution inspection — PARTIAL | Existing IPC omits raw arguments/results and reconnect does not replay the full historical timeline. Add a bounded redacted projection rather than reading private runtime stores in Rust. |
| Cancellation acceptance — PARTIAL | V3.2.1 observes the real owned /bin/sleep PID, sends Ctrl+C through release TUI, receives run.cancelled and observes no descendant afterward. Pending-approval and post-tool cancellation also pass live. General child trees, native Agent token-stream abort and mid-read preemption remain unvalidated; cmake launch failed separately. |
| Cancellation/session isolation — FIXED + VERIFIED | Late cancelled text entering a newly created conversation was reproduced and fixed; deterministic regression passes in 122/122 C++ suite. Active Agent runs cannot create/switch conversations. Stale cancellation-waiting notice text remains a presentation gap. |
| Physical terminal keys | Shift+Enter on Ghostty/macOS Terminal/iTerm2/Linux/tmux/SSH remains NOT VALIDATED. PTY input and deterministic routing do not substitute for physical host observation. |
| Performance/token comparison | No equivalent Codex/OpenCode benchmark or reliable per-turn token telemetry was available. Do not claim performance or autonomy parity. |

See [reliability report](TUI_V3_2_AGENT_RELIABILITY_REPORT.md), [test matrix](TUI_V3_2_AGENT_TEST_MATRIX.md), and [execution failures](TUI_V3_2_EXECUTION_FAILURES.md). No V3.3 work was started.

V3.2.1 closure evidence: [final report](TUI_V3_2_1_FINAL_REPORT.md), [context](TUI_V3_2_1_CONTEXT_RELIABILITY.md), [multi-file acceptance](TUI_V3_2_1_MULTIFILE_ACCEPTANCE.md), [cancellation](TUI_V3_2_1_CANCELLATION_ACCEPTANCE.md). Do not infer full acceptance from the passing regression suite.

## V3.2.2 measured update

[Multi-file final report](TUI_V3_2_2_MULTIFILE_FINAL_REPORT.md), [acceptance matrix](TUI_V3_2_2_AGENT_ACCEPTANCE_MATRIX.md), [execution evidence](TUI_V3_2_2_EXECUTION_EVIDENCE.md).

- Output-tail preservation fixed at native continuation, planning process history and UTF-8 byte-bound preview layers; original evidence remains unchanged.
- Natural multi-file coding remains NOT ACCEPTED: B/D changed calc.cpp but not test.cpp; no complete edit/review/build/test/accepted-final chain. C/D exhausted loaded context; A returned empty provider content; E failed repeated discovery and was cancelled. No automatic mutating retry added.
- macOS safe build execution is BLOCKED: real gateway denies Homebrew CMake execution and process forks under required process-tree policy. PATH changes cannot resolve this. Requires authoritative platform process supervision/confinement design, not a TUI shell shortcut or relaxed permission.
- Long raw approval JSON, stale historical approval prose and clipped notices remain presentation gaps. Physical hosts and other platforms remain NOT VALIDATED.

## V3.2.3 measured update

See [execution environment](TUI_V3_2_3_EXECUTION_ENVIRONMENT.md), [provider diagnostics](TUI_V3_2_3_PROVIDER_DIAGNOSTICS.md), [Agent acceptance](TUI_V3_2_3_AGENT_ACCEPTANCE.md), and [final report](TUI_V3_2_3_FINAL_REPORT.md).

- **IMPLEMENTED + VERIFIED:** allowlisted opt-in native diagnostics; successful native HTTP status propagation; output-limit rejection distinct from empty/malformed replies; context vs unrelated schema HTTP rejection classification; text-file and workspace-root tool guidance. Local HTTP and registry regressions pass. No new write retries or permission grants.
- **BLOCKED:** macOS safe CMake execution and cancellation of an actual build. Separate executable/dependency confinement and fork prohibition were reproduced. Requires a reviewed authoritative process ownership/containment capability; no sandbox relaxation is justified by a passing host build.
- **NOT VALIDATED:** Linux comparison, because no usable Linux/Docker runner is available. Native terminal-host acceptance remains unobserved.
- **NOT ACCEPTED:** complete multi-file Agent autonomy. Compare the actual model/fixture results in the phase acceptance report; unit tests do not establish edits, build/test or grounded final completion.
- **PARTIAL:** native context preflight uses an explicit heuristic, not a tokenizer. Its refusal is not measured provider exhaustion. Native observations/usage/finish reasons are now recorded, but plain intent-classification calls still lack equivalent production metadata and a per-request output bound in the local OpenAI-compatible body. Investigate that path before attributing an early model stall to tool continuation or model capability.
- **PARTIAL:** the temporary non-streaming diagnostic relay is not proof of upstream inference cancellation. Direct endpoint controls and bounded response metadata are required when interpreting timeout/cancel observations. Do not compare elapsed timings under different engine formats or concurrent development loads as a controlled performance result.
- **BLOCKED QA gate:** the macOS test_upgrade encryption case waits in SecItemCopyMatching on the user's Keychain. The test was terminated rather than granting secret access or changing Keychain permissions/data. The other 121 CTest executables pass; its two non-Keychain migration/compatibility subtests pass separately. A hermetic test credential backend or an explicitly authorized isolated test Keychain is required for this gate; do not bypass production encryption.

- **ISOLATED, NOT CLOSED:** final-source direct Qwen A waits in ObservationIntentPolicy::classify through plain local infer, before any tool. Fresh direct bounded transport probes succeed on both models. This is not evidence of a native tool-schema/continuation failure; the inference delay's underlying cause/finish reason remains unknown. Direct Nemotron A reaches an accepted observation final, but the receipt/excerpts do not satisfy the requested explanation. Preserve grounding and introduce a typed evidence-backed explanatory projection rather than accepting unsupported prose.

## V3.2.4 measured update

See [classification reliability](TUI_V3_2_4_CLASSIFICATION_RELIABILITY.md), [grounded completion](TUI_V3_2_4_GROUNDED_COMPLETION.md), [acceptance matrix](TUI_V3_2_4_ACCEPTANCE_MATRIX.md) and [final report](TUI_V3_2_4_FINAL_REPORT.md).

- **FIXED + VERIFIED — finite classification client lifetime:** explicit 512-token reservation and nominal 30-second deadline, cancellation propagation, one bounded transport attempt, output-limit rejection, truthful metadata and fail-closed Agent entry. Unsupported provider implementations reject the bounded capability. Nine live requests terminate safely at the bound, with no tools. Upstream cancellation remains unconfirmed.
- **PARTIAL — useful classification:** Nemotron's normal classification succeeds with actual HTTP200/stop and measured usage. Both models' read-task classifications time out; Qwen's normal case also times out. Safety is improved; read-task usefulness is not accepted. No model-capability ranking or delay cause is inferred.
- **PARTIAL — grounded explanation projection:** typed interpretations with current-run IDs/exact quotes and daemon-derived source paths pass deterministic resolver and real read-gateway tests. Strict symbolic claims/mutations remain separate. The generic receipt path and response-purpose relevance remain a gap. Live grounded A/B/E/F are not accepted because no reads occurred.
- **PARTIAL — repeated reads/context:** bounded latest-success inventory and existing correlation/truncation/doom-loop protection remain. Live recovery and pressure completion were not reached; no compaction/tokenizer subsystem was introduced.
- **FIXED IN HERMETIC REGRESSION — upgrade Keychain QA:** injectable key source exercises real macOS AES using a test key, without personal Keychain access. Final CTest is 122/122 PASS. OS Keychain integration coverage remains separate and unvalidated.
- **SCOPED PRESENTATION:** terminal cancellation replaces stale waiting notice; known failure Markdown wrappers display plainly; long notice previews show ellipsis and /notices access. Late wrapper/ellipsis fixes have deterministic tests, not a new live-model replay. Safe raw approval arguments and physical terminal-host validation remain open.
- **UNCHANGED BLOCKERS:** macOS safe child-tree build execution remains BLOCKED; containment is SPECIFIED ONLY. Linux independent execution, full edit/build/test acceptance, and complete deterministic real-TUI stall/cancel acceptance remain unvalidated. No V3.3 work was started.
