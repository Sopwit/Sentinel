# V3.2.2 isolated acceptance evidence — 2026-10-10

Baseline b5e6933 plus scoped uncommitted output-tail fixes. macOS arm64, actual already-loaded LM Studio nemotron-3-nano-4b, context 8192. No personal Sentinel profile/workspace or running personal daemon was a target. No commit/push/merge, model download or silent provider/model switch.

- `live.json`: real Rust TUI sends tasks via typed IPC. Exact public tool/run events and single-use path/digest approval decisions; synthetic before/after full file manifests and content. Match events by run_id. Observer-timeout is not successful completion.
- `*.diff`: actual source text before/after, not assistant claims or invented Git history.
- `*-host-baseline.json`: host-run synthetic configure/build/test baseline only; never Agent acceptance. D's baseline-test.log contains the actual observed failing test.
- `*.ansi`: actual release TUI byte streams, 120×40. PNG/TXT are bounded VT replays, not native Ghostty/macOS Terminal screenshots.
- `provider-metadata.json`: read-only local loaded-instance metadata; catalog presence alone is not readiness.
- `cmake-gateway-before.log`, `cmake-absolute-gateway-before.log`, `process-fork-gateway.log`: real gateway/executor/sandbox diagnostics, separate from model-driven tests. `process-probe.cpp` derives the existing integration helper and prints only the synthetic tool result, not model reasoning. It grants the known diagnostic plan in a test fixture; it is not a new production tool path or a user permission bypass.
- `native-tail-before.log`, `context-tail-before.log`: failing regressions before fixes; `llm-tests.log`: passing final focused suite (34 tests).
- `cpp-*`, `rust-*`: configure/build/test/lint results, separate from E2E acceptance. Full C++ suite 122/122; Rust 73 tests.

The harness imports its local support module and must run from the repository root with existing built binaries/local provider. It creates private temporary profiles and fresh workspaces. It never points Agent edits at the Sentinel checkout. Approval scopes are exact calc.cpp/test.cpp files; named commands each at most once. The task model chooses all calls. No private planner reasoning or raw provider prompts are captured. Direct endpoint probes provide no token/finish-reason telemetry; do not invent it.

F build cancellation remains BLOCKED by proven executable/fork confinement. Historical sleep cancellation does not certify a CMake build. Native hosts, Linux/Windows build/cancellation and physical modified keys remain NOT VALIDATED.

Final byte-bound regression: byte-tail-before.log failed before ToolOutputTruncator changes; final llm-tests.log passes 34 QtTest cases including setup/cleanup. binary-sha256.json distinguishes the live copied daemon (first two fixes) from final release daemon (third UTF-8 fix); acceptance-summary.json provides concise per-run evidence and model-request event counts. No full provider prompts or reasoning are exported.

Visual replay inspection: A-read final, B-edit approval-0, D-recovery final. Approval uses actual y/n scope; D shows retained historical approval prose and context failure. These existing presentation limitations remain documented. A read-only owned-process check after the harness found no surviving isolated daemon/TUI; private fixture files and partial changes remain for evidence.

Final source validation: cpp-final-ctest.log **122/122 PASS** (128.71s); llm-tests.log **34 PASS**; cpp-final-debug-build.log and cpp-final-release-build.log PASS. cpp-final-ctest-restricted.log and cpp-ctest-marker-regression.log preserve intermediate failures and are not final outcomes. Rust **73 PASS**, fmt/Clippy/debug/release PASS. No live journey is promoted to accepted on this basis.
