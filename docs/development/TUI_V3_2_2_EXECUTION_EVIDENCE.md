# V3.2.2 execution evidence

## Proven execution chain

`cmake-gateway-before.log`, `cmake-absolute-gateway-before.log`, `process-fork-gateway.log`: real ToolExecutionGateway + RealToolExecutor + ProcessExecutor + macOS sandbox, using the existing test fixture approval/resource pipeline. These diagnostics are INTEGRATION evidence, not real-model autonomous success. They isolate executable access and child creation independently. The helper lives outside production and adds no Rust execution path.

`native-tail-before.log` and `context-tail-before.log`: deterministic failures before fixing loss of final diagnostics. `llm-tests.log`: passing focused test suite after the fix. Exact source changes are scoped to native result representation, process observation excerpts and regressions.

`live.json`: actual TUI-started tasks, typed IPC events, real tool identities/results, exact permission decisions, terminal states, session messages and authoritative workspace-change projection. Full before/after manifests and generated diffs record synthetic effects; failed tasks retain partial work. Source mutation alone never counts as build/test success.

`*-idle/approval/tool-running/tool-result/final.ansi`: raw actual TUI byte streams. PNG/TXT are deterministic VT replays, not native-host screenshots. A-read final replay was visually inspected and shows the three-step provider failure, preserved user task, connected header, Agent composer and failed footer. No private planner reasoning is exposed or saved.

`provider-metadata.json`: read-only loaded model/config snapshot. All actual requests stay on the same explicitly selected local provider/model. Native context budgeting remains a heuristic; no tokenizer or published-capacity assumption is used as proof of readiness.

Debug/release build, formatting, Rust test/Clippy and C++ regression logs are separate from real-model acceptance. Host-baseline files are explicitly outside the Agent gateway and never credited as Agent verification.

## Final evidence and provenance

- acceptance-summary.json is a bounded derivation of live.json, retaining run/session IDs, final states, elapsed times, exact approvals, tool-result summaries and actual diffs.
- byte-tail-before.log records the real failing UTF-8 byte-bound assertion. llm-tests.log is the final focused suite. The first attempted test invocation used a stale binary after overlapping autogen builds; that invalid attempt was replaced with a serial successful build and actual failing regression before production changes.
- D-recovery-final.ansi.png and B-edit-approval-0.ansi.png were rendered and visually inspected in addition to A-read. These are replays of captured release TUI output, not fabricated layouts or native-host screenshots. No new visual behavior is attributed to this phase.
- cpp-final-* logs validate final C++ source; earlier cpp-* and rust-* logs validate the initial changes and unchanged Rust client. The final upstream byte-bound fix postdates live acceptance. Do not conflate regression coverage with autonomous completion.

A/B/C/D/E all lacked accepted final completion. C/D reported context exhaustion. A reported empty model content/tool calls. B/E were cancelled via actual TUI after bounded observation. No usage/finish-reason metadata establishes why the provider returned an empty response. Full build cancellation stays BLOCKED by gateway confinement.

The fixture contains seven original files. builtin-tool-inventory.json records the 35 baseline built-in descriptors; schema validation and policy remain authoritative, and catalog membership does not grant execution. The private profiles add no extension catalog. D's host baseline configured/built with exit 0, then CTest exited 8 (expected 4, got -1). Those host results establish a reproducible failing fixture only.

The final restricted Codex-sandbox CTest attempt reported 105/122 passing and 17 failing, including localhost listen denials and sandbox_apply failures. This is retained separately; final permitted-host results must be read from cpp-final-ctest.log. Codex test-run permission is separate from Sentinel's runtime sandbox, which was never relaxed.

The first permitted final suite passed 121/122 and caught a preview-marker compatibility regression: AgentLoop's existing truncation test requires the word "omitted". The byte-bound marker was corrected to "middle omitted; byte limit" without changing truncation budgets or evidence. cpp-ctest-marker-regression.log preserves that real intermediate failure; it is not hidden as a platform issue.

Final permitted-host rerun after marker correction: **122/122 PASS**, 128.71s, cpp-final-ctest.log. Final focused suite: **34 PASS**. Final debug/release daemon logs and binary hashes correspond to the corrected source. Rust's unchanged client retains the recorded 73 passing tests, formatting/Clippy and debug/release build results.
