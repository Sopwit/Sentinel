# P1-04 — Rust IPC Handshake Investigation and Scoped Fix

Date: 2026-10-16. Requested audit baseline: `b152a960c11fe6e13b086666752ea5954005797b`. **Actual investigation HEAD: `f106ab65b3907ab94d5a6c51fc606b231151f2f7`**, branch `feat/p4-desktop-live-integration`. Finding F02; criterion AC-P1-04. Scope: Rust Unix IPC reader and directly related tests only. No model/server/profile/credential/database access, runtime settings, dependency changes, commits, pushes or GitHub writes.

## 1. Executive Summary

**Reproduced and confirmed:** the original `transport_tests::handshake_correlation_and_event_queue` failed on the first isolated invocation with `Disconnected`; the initial 100-repeat stress had **86 passes / 14 failures**, all the same error. Synchronized socket-pair tests then proved a client reader defect: macOS rejected setting a read timeout after peer closure while a complete response was already buffered or queued. The reader returned Disconnected before examining that complete frame.

A minimal production correction preserves the absolute deadline, consumes already-buffered bytes without an unnecessary socket-option call, and, if setting a timeout fails, attempts only a **nonblocking drain** of available bytes. No data/partial EOF still fails; correlation, decoding, limits and timeout shutdown remain intact. Eleven narrow regression test functions were added; the original fixture was not kept artificially alive or otherwise changed to hide closure.

Final local macOS IPC-suite stress: **100/100 passed, 0 failed**, including all 26 IPC tests on each iteration. Full Rust workspace: **141 passed, 0 failed** (CLI 15, IPC 26, mascot 22, TUI 78). Formatting, Clippy and generated IPC/Desktop checks passed. Linux and exact macos-14 CI runner validation were unavailable. **AC-P1-04 remains open/partial; acceptance matrix status is unchanged.**

## 2. Baseline and Working-Tree Safety

Entry `git status --short` was empty, including staged changes. HEAD had advanced since the requested baseline: committed changes include the five previously concurrent Desktop dictation files and the six P0 audit/roadmap/acceptance documents. `git diff <requested-baseline> HEAD -- cli/crates/sentinel-ipc/src/lib.rs` was empty: the actual target was byte-identical to the audited implementation. This justifies scoped F02 comparison; it does not revalidate other architecture findings.

Personal-Brain `brain` was absent on PATH. The supplied checkout was verified directly. Root AGENTS.md and architecture/runtime/security/build/testing instructions, IPC/daemon docs, P0.1/P0.2, acceptance matrix and decision register were consulted. No sub-agent was used and no user-owned source was reset/stashed/formatted/staged/reverted.

Host: Darwin arm64, macOS **27.0.1**, rustc **1.98.1 (48a229cea 2026-09-01)**, Cargo **1.98.1 (797e8a9bc 2026-08-05)**. This is not certification of declared minimum Rust 1.94 or hosted CI. Preflight hashed all tracked/prior audit/roadmap files and guarded the Rust target before each edit; after each authored target change its expected digest was updated. No conflicting external target change was detected. Only `rustfmt --edition 2024 --config skip_children=true` on the target file was used to format edits; workspace fmt ran in check mode.

The five user-owned files were **clean at entry and byte-preserved**, with before/after hashes below. They were not part of this task's changes:

| Exact path | SHA-256 before | SHA-256 after |
| --- | --- | --- |
| apps/sentinel-desktop/include/sentinel/desktop/DesktopShellViewModel.h | 72eb506e0972f021c45f7b06b72a9cede809153744e1587835a287eed69d6ab7 | 72eb506e0972f021c45f7b06b72a9cede809153744e1587835a287eed69d6ab7 |
| apps/sentinel-desktop/include/sentinel/desktop/SystemDictationService.h | 509e242b962f6ea548bead6d91329e9bee26e729f30d2f858fa3f0851523ad9c | 509e242b962f6ea548bead6d91329e9bee26e729f30d2f858fa3f0851523ad9c |
| apps/sentinel-desktop/src/DesktopShellViewModel.cpp | df1d60637e32bab5156bf6473c252d10daa65c58c5fcbafa570ca83eab897309 | df1d60637e32bab5156bf6473c252d10daa65c58c5fcbafa570ca83eab897309 |
| apps/sentinel-desktop/src/MacSystemDictationService.mm | 533d61143d56dc2eef1f1e52be7b4314b9b2b2162f89622bf86dced1f92b8448 | 533d61143d56dc2eef1f1e52be7b4314b9b2b2162f89622bf86dced1f92b8448 |
| apps/sentinel-desktop/src/SystemDictationService.cpp | 8e5501b6e31867d84e413d520b7e96332b1c32f4b27afb0f918038c441d5151e | 8e5501b6e31867d84e413d520b7e96332b1c32f4b27afb0f918038c441d5151e |

All pre-existing files outside the target matched their entry hashes, including historical audits, acceptance matrix, decision register, Cargo manifests/lockfile and generated contracts. Final intended changes are one modified Rust file and this new report. The entry target SHA-256 was `899a3e802d920139e42442557942e4a9327e233b454decec082d0cd5a96c4936`; the final target digest is recorded in section 13. No automatic baseline switch or acceptance status edit occurred.

## 3. Transport Architecture

Source: `cli/crates/sentinel-ipc/src/lib.rs`, `Client::{connect,request,request_until,read_until,next_event}`, `remaining_budget`, `transport_error`, `decode`, `Error::{requires_reconnect,exit_code}`; canonical fields/types in `protocol/ipc-v1.json` and generated `contract.rs`.

`Client::connect` connects a UnixStream to the explicitly supplied path, sets a ten-second default read timeout, clones the socket for writing, and sends a correlated hello request. The daemon owns socket binding/permissions in production; the original test binds a fresh process-specific temporary socket, and new tests use private UnixStream::pair endpoints. No real Sentinel endpoint is contacted.

Requests serialize one JSON/LF frame, increment the request sequence and write remaining bytes under one absolute deadline. Interrupted writes retry within that budget; zero writes/disconnects remain failures. Reading can receive partial frames; `incoming` retains their bytes. Complete envelopes are decoded/type/version validated. Events preceding a matching response are queued (limit 256); response ID/name/type must match. Request timeout closes the connection to prevent a late response poisoning another request. Successful requests restore polling/write timeouts. Disconnect never means cancel or replay. Reconnect/attach belongs to existing CLI/TUI callers; business versus transport error classification was not changed.

`BufReader` may read multiple frames in one syscall. Thus socket closure, kernel queued bytes, BufReader-prefetched bytes and parsed event queue are four distinct states. EOF is only known after queued/buffered bytes are drained. Closing the peer is not proof a preceding fully delivered response was invalid.

The original fixture reads hello and daemon.status, writes an output.delta before the status response, then drops both server socket handles. It deliberately checks the queued event and eventual EOF. Production daemons usually keep the connection open, but shutdown/disconnect can create the same closed-peer/queued-response boundary. This is a client transport defect exposed by a valid fixture lifetime, not evidence that the fixture needed a sleep or an extended keepalive.

## 4. Original Failure Analysis

The first isolated test failed at the status request unwrap (original source around line 613), not proof that initial hello itself failed. Sanitized trace:

```text
transport_tests::handshake_correlation_and_event_queue
called Result::unwrap() on an Err value: Disconnected
FAILED: 0 passed; 1 failed; 14 filtered out
```

Initial stress failure iterations: **3, 5, 6, 7, 10, 24, 26, 31, 33, 65, 67, 86, 95, 97**. All 14 exited 101 with Disconnected; no harness timeouts. The first standalone failure is retained separately from these 100 attempts. Passing attempts do not erase failures or establish statistical reliability.

## 5. Reproduction Method

The requested locked/offline command was verified against package/test names and run first:

```sh
cargo test --manifest-path cli/Cargo.toml --locked --offline -p sentinel-ipc handshake_correlation_and_event_queue -- --nocapture
```

A temporary Python subprocess driver invoked that exact command 100 times, sequentially, with a 30-second harness deadline per invocation. Every stdout/stderr/exit was retained; there was no retry-until-success logic. Subsequent stress invoked all IPC tests once per iteration. Test subprocesses used isolated temporary Unix sockets/socket pairs and disposable fixture data only. Local execution needed the socket-capable execution permission because the ordinary sandbox previously rejected Unix socket binding; no production policy/sandbox was disabled.

New synchronized tests prefetch a full frame, close the peer synchronously, then invoke the reader. This establishes causal ordering without a timing sleep. Channel barriers hold the sequential-response server alive until timeout restoration assertions finish. Short real timeout tests exercise expiry, rather than sleeping to make a closure race disappear.

## 6. Root-Cause Evidence or Remaining Hypotheses

**Confirmed on the local macOS host:**

1. Server delivers a complete event/response frame and closes its handles.
2. Client's BufReader already holds a full response, or the disconnected socket still has queued bytes.
3. Old `read_until(Some(deadline))` calls `set_read_timeout` before `fill_buf`, even for already-buffered bytes.
4. On this host, the closed-peer setter returns `EINVAL` (OS code 22 / InvalidInput).
5. Old `.map_err(|_| Error::Disconnected)?` exits before decoding the available response. Scheduling determines whether the option call precedes peer close, explaining intermittent outcomes.

Deterministic pre-fix evidence:

```text
closed peer timeout setter:
Err(Os { code: 22, kind: InvalidInput, message: "Invalid argument" })
buffered bytes=156
buffered_response_after_confirmed_peer_close: Err(Disconnected)
unbuffered_complete_response_after_confirmed_peer_close: Err(Disconnected)
```

The buffered fixture asserts that its complete frame is present before closing. The unbuffered fixture uses a 16-byte reader capacity and synchronously writes/closes before reading, so it forces queued-frame draining over multiple fills. Both failed before the production correction and passed afterward. The same pre-fix suite also masked stale response correlation as Disconnected and failed to consume partial EOF bytes before its setter error. This is direct source/syscall/regression evidence, not timing-only speculation.

No Linux EINVAL behavior, live production outage frequency or universal OS error code is inferred. The fix does not branch on EINVAL: any failed deadline option update permits only a bounded nonblocking attempt to consume available data, then normal strict parsing or failure. It never waits indefinitely after the setter failure, suppresses read EOF, fabricates success or accepts a mismatched response.

## 7. Changes Made and Rationale

**Modified:** `cli/crates/sentinel-ipc/src/lib.rs` only.

- Always check `remaining_budget` before processing a deadline-bound frame, including buffered data.
- If BufReader already has bytes, avoid setting a socket timeout that cannot affect reading those bytes.
- If an empty-buffer read cannot set its timeout, temporarily enable nonblocking mode, call fill_buf once, then restore blocking mode through the existing writer clone (same underlying socket flags). Available bytes still traverse frame limits/decode/correlation; no bytes, EOF, restoration failure or drain read error yields Disconnected.
- Keep request write loop/deadline, request timeout shutdown, timeout restoration, schema, version, event queue, IDs, reconnect behavior and protocol unchanged.
- Add eleven focused test functions; preserve the original fixture's immediate closure behavior.

Production delta is confined to read_until; test code accounts for most added lines. No dependencies, CI/build/C++/Desktop files or historical documentation changed. No blanket timeout increase or sleep-based race mitigation was introduced.

## 8. Deterministic Regression Tests

All new symbols are in `close_regression_tests` in the same Rust file. Existing deadline/partial-read tests remain in `tests`.

| Requested boundary | Regression evidence |
| --- | --- |
| Hello response then immediate close | handshake_event_order_and_immediate_close: response-only variant, complete typed acknowledgement then EOF |
| Event before hello response | Same function: before-event variant queues then drains event |
| Response before event | Same function: after-event variant drains event after acknowledgement even with peer close |
| Partial frames/reads/writes | fragmented_frame_writes_and_large_request: 20,000-byte request, 16-byte readers, server writes response in 3-byte fragments; existing partial_frame_survives_poll_timeout retains prefix across polling timeout |
| Complete response then EOF | buffered_response_after_confirmed_peer_close and unbuffered_complete_response_after_confirmed_peer_close; queued/buffered delivery validated |
| Incomplete response then EOF | incomplete_response_at_eof_is_not_success asserts Disconnected and 30 partial bytes, never success |
| Expired read/write deadline | Existing expired_deadline_rejects_buffered_frame_without_consuming_it; new expired_write_deadline_sends_no_frame; blocked_writer_deadline_closes_transport fills actual send queue to WouldBlock, then validates timeout/closure |
| Sequential timeout restoration | sequential_requests_restore_both_timeouts: two request IDs, nondefault read/write timeouts, channel-controlled server lifetime |
| Stale/mismatched response | stale_id_and_wrong_name_are_rejected: stale ID and valid typed wrong-name payload return Protocol, not success |
| Peer/client shutdown closure | peer_closes_without_response_is_disconnected; client_drop_closes_all_socket_handles checks peer EOF after both cloned client handles drop |

Pre-fix red suite: **4 passed / 5 failed** across the first nine new functions. Failures: buffered close, unbuffered close, immediate-close ordering, stale/wrong correlation, and incomplete-EOF byte-retention assertion (its Disconnected outcome itself was already correct). Post-fix suite first passed 24 tests; two additional negative deadline/no-response tests raised the final IPC count to 26. The final handshake-order test includes response-only plus both event orders. The final 100-repeat suite includes every variant. No test failure was removed or ignored.

Coverage limit: the fragmented-write fixture deterministically fragments **server application writes** and bounds reader fills; the large request exercises real client writes but does not instrument an OS short-write/Interrupted count. Arbitrary kernel short-write scheduling and injected option-restoration failures remain additional fault-injection opportunities, not acceptance claims from this test. Absolute deadline/write/EOF negative cases are covered as above.

## 9. Stress Test Results

| Candidate / run | Iterations | Passed | Failed | Meaning |
| --- | --- | --- | --- | --- |
| Original production reader, original target | 100 | 86 | 14 | Reproduced intermittent Disconnected; every failure retained |
| Corrected reader + 26 IPC tests before response-only test variant | 100 | 100 | 0 | First post-fix suite stress on local macOS; source digest differs from final test-order expansion |
| Final reader/tests, all 26 IPC tests including original target | 100 | 100 | 0 | **Final local acceptance subset**; 2,600 IPC test executions, not independent statistical reliability samples |

The first post-fix stress was launched after adding both negative tests; final source later expanded one test to add response-only ordering. The production reader was unchanged between those two successful stress runs. Final workspace ran again after that test-only expansion. None of these macOS results substitutes for Linux or exact hosted CI evidence.

## 10. Cross-platform Coverage

| Environment | Result |
| --- | --- |
| Local macOS 27.0.1 arm64 / Rust 1.98.1 | Original failure + deterministic red/green + final 100 suite iterations + full workspace/fmt/Clippy/contract checks |
| Ubuntu Linux Rust CI | **Unavailable / pending**; not run remotely or through a local Linux environment |
| macos-14 hosted Rust CI, declared minimum Rust 1.94 | **Pending**; local newer host/toolchain is not this exact runner |
| Windows | Not a supported Unix stress cell; existing Rust named-pipe limitation remains, no Windows claim |

`.github/workflows/ci-rust.yml` defines Ubuntu/macOS targets and Rust 1.94. No workflow or remote CI run was created/changed. No native Desktop/audio or AI acceptance was performed.

## 11. Remaining Risks

- Linux and exact CI/minimum-toolchain behavior must be verified before closing AC-P1-04. Local 100-repeat success is bounded evidence only.
- Nonblocking flags are shared by cloned socket handles; the client owns both and mutable request access serializes this path. Future concurrent socket ownership must preserve that assumption.
- Restoration failure remains fail-closed; no arbitrary socket-option failure injector was added.
- Original fixture can leave its process-specific temporary directory when an assertion aborts before cleanup. Failed logs/state were retained; no broad temp cleanup or personal directory access occurred.
- Raw local Cargo logs contain build paths; keep them local/private and use sanitized excerpts for public evidence. Temporary storage is not durable archival or permission to publish it.
- No hard provider shutdown, agent reliability, release-security or unrelated Desktop voice finding is closed by this fix.

## 12. Acceptance Criterion AC-P1-04 Assessment

**Confirmed root cause, scoped production fix and local-host subset passed; overall criterion remains OPEN / PARTIAL.** The existing matrix's Ready for Validation status was not edited. Root-cause/negative-regression evidence and local macOS 100-repeat requirement are satisfied for the recorded candidate diff; required Linux (and exact supported CI/toolchain) evidence is unavailable. F02 is not silently closed, and AC-P5-05/whole P1/platform/release acceptance is not claimed.

## 13. Commands and Evidence

All Cargo commands used locked/offline cached dependencies. Socket tests ran with private temporary socket execution allowed; ordinary source/document reads remained sandboxed. No global Cargo/system configuration was changed.

| Command / step | Outcome / retained local evidence |
| --- | --- |
| git branch/HEAD/status + baseline target diff | Branch/actual HEAD above, clean entry, unchanged target versus requested baseline |
| rustc --version; cargo --version; uname; sw_vers | Host/toolchain above |
| Original targeted cargo test | Exit 101; `/tmp/sentinel-p104-original.log` |
| 100 original-target subprocess invocations, 30-second cap each | 86/14; `/tmp/sentinel-p104-before-stress/001.log`…`100.log` and results.json |
| First new close_regression_tests against unchanged reader | Exit 101, 4/5; `/tmp/sentinel-p104-red.log` |
| Initial fmt check on newly authored unformatted tests | Exit 1, formatting diff only; `/tmp/sentinel-p104-initial-format.log`; corrected with scoped rustfmt, no product defect inferred |
| First fixed sentinel-ipc suite | Exit 0, 24 tests; `/tmp/sentinel-p104-green.log` |
| First corrected 100-suite stress | 100/0; `/tmp/sentinel-p104-after-stress/001.log`…`100.log` and results.json |
| Final 100-suite stress | 100/0; `/tmp/sentinel-p104-final-stress/001.log`…`100.log` and results.json |
| cargo test --workspace --no-fail-fast --locked --offline --manifest-path cli/Cargo.toml | Final exit 0, **141 passed / 0 failed**, zero doc-tests; `/tmp/sentinel-p104-final-workspace.log`; earlier workspace log also retained |
| cargo fmt --all --check --manifest-path cli/Cargo.toml | Final exit 0 |
| cargo clippy --locked --offline --manifest-path cli/Cargo.toml --all-targets -- -D warnings | Final exit 0; no warnings; `/tmp/sentinel-p104-final-clippy.log` |
| python3 tools/ipc/generate.py --check; python3 tools/ipc/generate_desktop.py --check | Exit 0; generated contracts unchanged |
| git diff --check; focused personal-path scan; preservation hashes | Exit 0; 0 files flagged by focused path scan; every pre-existing file except intended target byte-identical to entry, all five Desktop hashes preserved; no concurrent Rust conflict detected |
| CMake/native/model/release tests | Not run; no C++/QML/build/contract changes and outside this scoped Rust transport task |

Per-iteration files retain the command output and exit; results.json stores the command and every attempt. Sanitized distinct failures are recorded in sections 4/6/8. Evidence digests:

- before stress results.json SHA-256: `18c4b2e9fab9bbd1a6fba201c1c0cfd7e5f070d24e7d708006aee4100818b5a7`.
- after stress results.json SHA-256: `1b3eef9d56832f091b18352d69612ef6e1579318a115d47ee2a0091313a97d8c`.
- final stress results.json SHA-256: `1b3eef9d56832f091b18352d69612ef6e1579318a115d47ee2a0091313a97d8c`.
- Final target source SHA-256: `a1b969acbbce0b491681a3f679164311692a5a711f788412d3984bf9a1ec2508`.

## 14. Recommended Next Task

Run the same original/negative regression cases and 100-repeat final IPC suite on the supported Linux environment and exact CI toolchain/runner subset. Preserve all outcomes and source/config digests, then reassess AC-P1-04 without overwriting historical failures. If those pass, review this small transport diff and its tests before any separately authorized commit or integration; do not promote P1/model/native/release acceptance from this result.
