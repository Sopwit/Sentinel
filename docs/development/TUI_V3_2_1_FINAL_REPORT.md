# Sentinel V3.2.1 Agent execution reliability closure

## Verdict: IMPROVED — FURTHER FIXES REQUIRED

Context budgeting, file evidence identity and cancellation/session isolation improved with regression coverage. A real two-file context task completed. Running-process cancellation is confirmed through the actual Rust TUI, typed IPC, authoritative Agent runtime and owned OS PID cleanup. Full natural multi-file modification plus verification acceptance remains incomplete. No V3.3 work was started.

## Baseline and reproducible defects

Baseline: clean working tree at 18bc2a9. Personal-Brain CLI was unavailable. Architecture/runtime/security/build/testing and all four V3.2 reports were reviewed. Earlier source modification was real; its final context failure was not a completed verified task.

1. LM Studio's published model maximum 1048576 was used instead of actual loaded context 8192. Native schema/result overhead was outside the prompt estimate, and native generation reservation was unspecified.
2. Built-in read-file resolved relative paths against the authorized workspace, while evidence could reinterpret the original relative argument against daemon cwd. Grounded output therefore risked rejecting valid scoped observations.
3. Actual cancellation followed by session creation exposed queued previous-run terminal text in the new conversation. The regression failed before the fix and passes afterward.

## Implemented fixes

| Source | Change | Validation |
|---|---|---|
| ModelLibrary.cpp | Bound catalog context to minimum positive loaded instance window | UNIT PASS |
| ModelService.cpp | Bound native descriptions, reserve <=1024 output tokens, preflight serialized native request including schemas/results | UNIT PASS; measured actual native provider usage; capacity negotiation still PARTIAL |
| LlmAgentRuntime.cpp | Explicit 2000-character native result excerpts; retain original evidence; stop on known context exhaustion without mutation retry | UNIT PASS; LIVE failure handled accurately |
| ObservationPolicy.cpp | Built-in read evidence uses the authoritative absolute resolved path; external tools cannot rebind it | UNIT PASS; LIVE E2E two-file facts |
| ApplicationController.cpp | Block conversation create/switch during active Agent execution; retire idle old projection identity | INTEGRATION PASS; failing-before/passing-after session race reproduction |

Rust TUI remains unchanged in this phase. Existing Shift+Enter/fallback editing, commands and keyboard routing are preserved. No Qt/QML presentation redesign, IPC schema fork, new tool authority or unrestricted shell path was added.

## Test verification

- **INTEGRATION PASS:** final CMake tests preset debug configure/build and CTest: 122/122, 153.74 s, QT_QPA_PLATFORM=offscreen.
- **UNIT / INTEGRATION PASS:** focused LlmAgentRuntime 32/32; ObservationPolicy 35/35; final suite includes new daemon projection race test.
- **PASS:** Rust 73 tests (48 TUI + 11 IPC + 14 CLI), formatting check, Clippy workspace/all targets with warnings denied, debug and release builds.
- **PASS:** C++ release daemon configure/build; changed C++ ranges clang-formatted; git diff --check.
- Sandbox-local socket/process fixtures initially require permitted host execution; verified runs used that permission. Ccache disabled for builds because its configured external volume is not writable in this workspace. The verified host test runs pass; the initial sandbox-restricted attempts are not acceptance evidence.

Commands: `cmake --preset tests`, `cmake --build --preset tests`, `ctest --preset tests --output-on-failure` (CCACHE_DISABLE=1 for build/configure); release preset `cmake --build --preset release --target sentinel-daemon`; Rust `cargo fmt --all -- --check`, `cargo clippy --workspace --all-targets -- -D warnings`, `cargo test --workspace`, `cargo build --workspace`, `cargo build --workspace --release` from cli. Logs are in the evidence directory.

## Live acceptance boundaries

Provider/model: LM Studio / nvidia/nemotron-3-nano-4b, actual loaded window 8192, local endpoint 127.0.0.1:1234. Only that already-loaded model was selected. Synthetic C++20/CMake fixtures and private portable profiles isolate all effects. Approvals match exact file paths or command digests, not broad workspace write/execute grants.

A completed with two native reads and grounded actual markers. Native input usage 5395/5774/5992 and output 310/93/193; output reservation 1024 each. Similar-sized D request still failed context capacity; the precise residual provider cause is unresolved. Initial B/C/E exceeded bounded observation periods; C made no observed edits. These are PARTIAL/FAIL, not completed coding journeys.

A direct release TUI Ctrl+C test observed owned /bin/sleep PID 72613 before cancellation and none afterward; run.cancelled was authoritative. Pending-approval cancellation and post-tool continuation cancellation also have actual events. Cmake's separate command launch failed before a live PID could be observed; it is not cleanup acceptance. PTY VT replay was inspected at 80×24. Native terminal hosts and physical modified keys are NOT VALIDATED.

Direct B exceeded 420 s after 11 tool results; C failed after 284.87 s under the unchanged-call duplicate guard, without an edit approval/diff. Direct D failed after 262.72 s: exact configure requested/approved twice, both executions failed, then an unlisted command was denied. Build/test completion remains unverified; no build directory or source diff was present. Model-initiated repeated process requests across observations remain a resumability gap despite fresh approvals. Chat streaming cancellation is NOT VALIDATED: no public delta within 90 s. An isolated closed-endpoint configuration cancelled immediately with no tools; restored state is not resumed-inference acceptance. Some independent probes overlap against the loaded local model; durations are test observations, not a controlled throughput/performance comparison. The metadata proxy cannot abort blocked upstream reads, so only direct-endpoint probes support backend cancellation conclusions.

## Recovery and security

Existing deterministic coverage passes for native argument repair and unknown-call repair, bounded invalid-call failure, repeated read planning repair without reexecution, mutation duplicate rejection, missing-file evidence, denied permission, ProcessExecutor timeout/cancellation/shutdown, IPC reconnect/disconnect and no late completion. These are fixtures unless a live row explicitly says otherwise. No blanket retry, uncertain mutation replay, implicit summarization, provider fallback or automatic Git action was added. Full evidence stays authoritative even where model-facing excerpts are truncated.

Required next closure work: complete real two-file edits plus approved verification and accepted grounded final; expose bounded redacted launch diagnostics; diagnose remaining native context errors; verify general owned child-tree cancellation and native-host input separately. Architectural tokenizers/compaction/resumable mutation design is deferred; V3.3 is not started automatically.

See [context reliability](TUI_V3_2_1_CONTEXT_RELIABILITY.md), [multi-file acceptance](TUI_V3_2_1_MULTIFILE_ACCEPTANCE.md), [cancellation](TUI_V3_2_1_CANCELLATION_ACCEPTANCE.md), [remaining gaps](TUI_V3_2_REMAINING_GAPS.md), and [raw evidence](../reviews/agent-v321-2026-10-10/README.md).

## Explicit remaining validation

| Case | Status | Boundary |
|---|---|---|
| Invalid/unknown native tool arguments | UNIT PASS | Scripted provider repair tests; live malformed-call injection not attempted |
| Repeated read / mutation duplicates | UNIT PASS; LIVE safe rejection | Direct C rejected repeated unchanged call; no edit applied |
| Missing file / permission denial | INTEGRATION PASS | Existing gateway/Agent fixtures; V3.2 denial live evidence is historical, not a new V3.2.1 pass |
| Tool timeout / shutdown | INTEGRATION PASS | ProcessExecutor owned fixture; not a real-model timeout recovery journey |
| Context exhaustion | UNIT PASS; LIVE failure handled | Failure preserved, no accepted success or automatic mutation retry |
| Client reconnect during Agent | PARTIAL | Actual TUI attaches to an active run; durable full timeline replay / post-disconnect mutation dedup not established |
| Mid-read cancellation | NOT VALIDATED | Atomic read completed too quickly to establish preemption |
| Native Agent streaming abort | NOT VALIDATED | No public private-planning stream; separate Chat probe had no delta in bound |
| Running owned process | LIVE E2E PASS | Actual /bin/sleep PID disappears after TUI Ctrl+C; general child trees unvalidated |
| Pending approval / post-tool cancellation | LIVE E2E PASS | Correct cancelled state; no granted operation after pending-approval cancellation |
| Unavailable provider cancellation | PARTIAL | Closed endpoint configured; cancellation before proof of an in-flight network failure |
| Native Ghostty/macOS Terminal/iTerm2/Linux/Windows | NOT VALIDATED | PTY and platform unit fixtures are distinct evidence |

## Final task acceptance

| Task | Final status |
|---|---|
| A: two-file context | LIVE E2E PASS |
| B: multi-file explanation | PARTIAL — 180 s initial and 420 s direct bounds; no accepted final |
| C: function + test edits | FAIL / PARTIAL — initial timeout; direct duplicate-call rejection; no edits |
| D: build/test verification | FAIL — initial context rejection; direct failed configure and denied unlisted follow-up |
| E: near-capacity multistep | PARTIAL — bounded cancellation; not completed |
| F: running process cancellation | LIVE E2E PASS for direct /bin/sleep; cmake launch probe PARTIAL; general child trees NOT VALIDATED |

Recommended next batch is focused closure of the existing multi-file/verification workflow, process-launch diagnostics and outcome-aware repeated process handling. Do not start V3.3 or claim competitor parity from these results. No commit, push or merge was performed.
