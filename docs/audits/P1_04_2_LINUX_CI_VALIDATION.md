# P1-04.2 — Linux / CI IPC Regression Validation

Date: 2026-10-16. Finding F02; criterion AC-P1-04. Previous investigation: `docs/audits/P1_04_IPC_HANDSHAKE_INVESTIGATION.md` (macOS, final digest `a1b969…c2508`). Scope: Linux regression validation of the already-committed P1-04 IPC reader correction plus static review and CI-matrix coverage analysis. No transport redesign, no source changes, no commits/pushes, no acceptance-matrix edits, no remote GitHub writes.

## 1. Safety and Baseline

At entry the working tree was clean (`git status --porcelain` empty). During the session an unrelated, user-owned modification appeared externally:

| Exact path | Status | Action taken |
| --- | --- | --- |
| `core/src/runtime/RealToolExecutor.cpp` | `M` (external, user-owned) | Byte-preserved, untouched, excluded from all claims |

No reset, stash, stage, commit, push, or revert was performed. No real Sentinel user profiles or runtime sockets were used; all tests bind private process-scoped temporary Unix sockets or socket pairs.

## 2. Environment Identity

| Item | Value |
| --- | --- |
| Host OS | Fedora Linux 44, KDE Plasma Desktop Edition (x86_64) |
| Kernel | 7.2.9-200.fc44.x86_64 (SMP PREEMPT_DYNAMIC, 2026-10-03) |
| Architecture | x86_64 GNU/Linux |
| rustc (stable) | 1.99.1 (58cf8f9c3 2026-10-13) |
| cargo (stable) | 1.99.1 (5f94df478 2026-08-27) |
| Installed minimum toolchain | 1.94.0 (4a4ef493e 2026-03-02), rustfmt + clippy components, installed with user authorization |
| Git branch / HEAD | `main` / `0245e5fa94584a357a6b1de01344f69b577e0e41` (includes `16dea2c4 fix(ipc): resolve client reader defect …`) |
| Toolchain availability note | **No Rust toolchain existed on this machine at entry** (verified `rustc`/`cargo`/`rustup`, `~/.cargo`, `/usr/local`, `/opt` absent). Stable + 1.94.0 were installed into `~/.rustup`/`~/.cargo` after explicit user authorization; no system-wide package manager install, no global settings changed. |

**Tested source digest (exactly reproduced across every phase):**

| File | SHA-256 (sha256sum) |
| --- | --- |
| `cli/crates/sentinel-ipc/src/lib.rs` | `a1b969acbbce0b491681a3f679164311692a5a711f788412d3984bf9a1ec2508` |

This is byte-identical to the final candidate digest recorded in the P1-04 investigation (section 13). `git diff HEAD -- cli/crates/sentinel-ipc/src/lib.rs` is empty. The production correction is present in this checkout.

**Exact IPC regression test inventory (26):**

- `tests` (12): `command_reconnection_policy_preserves_business_errors`, `rejects_malformed`, `rejects_oversized`, `event_decoding`, `incompatible`, `exit_codes`, `partial_frame_survives_poll_timeout`, `expired_deadline_rejects_buffered_frame_without_consuming_it`, `request_timeout_closes_transport`, `successful_request_restores_polling_timeout`, `stable_error_categories`, `missing_daemon`
- `transport_tests` (2): `handshake_correlation_and_event_queue`, `typed_request_invalid_enum_or_fields`
- `generation_compatibility_tests` (1): `hello_generation_is_additive`
- `close_regression_tests` (11): `buffered_response_after_confirmed_peer_close`, `unbuffered_complete_response_after_confirmed_peer_close`, `incomplete_response_at_eof_is_not_success`, `handshake_event_order_and_immediate_close`, `fragmented_frame_writes_and_large_request`, `expired_write_deadline_sends_no_frame`, `sequential_requests_restore_both_timeouts`, `stale_id_and_wrong_name_are_rejected`, `blocked_writer_deadline_closes_transport`, `peer_closes_without_response_is_disconnected`, `client_drop_closes_all_socket_handles`
- Doc-tests: 0.
- Counting reconciliation (P1-04.3, 2026-10-16): this inventory, like the original P1-04 report, initially labeled `tests` as 11 and grouped `hello_generation_is_additive` under `transport_tests`, yielding an apparent 11+3+11 = 25 versus the suite's actual 26. Real `cargo test --locked -p sentinel-ipc -- --list` discovery (26 tests, 0 benchmarks; doc-tests 0) confirms the true per-module split is 12+2+1+11 = 26; no test has been deleted or added by this correction.

## 3. Critical Code Review (`read_until`, production delta)

Confirmed by direct inspection of `Client::read_until` (lines 359–408), `request_until`, and `request`:

- **Preserved absolute deadlines:** `remaining_budget(deadline)?` is evaluated before every `fill_buf` iteration (including buffered data); the drain path never extends the deadline and the loop re-enters budget checking each iteration, so expiry terminates even under a repeatedly failing setter.
- **Complete buffered responses:** when `self.reader.buffer()` is non-empty, `set_read_timeout` is skipped entirely — already-buffered bytes are consumed with no socket-option call, which is the root fix.
- **Queued socket bytes after peer closure:** when the empty-buffer `set_read_timeout` fails (macOS EINVAL after peer close), the reader enters temporary nonblocking mode and performs a nonblocking `fill_buf`, draining kernel-queued bytes; EOF is only reported after buffered and queued bytes are exhausted.
- **Safe temporary nonblocking / reliable restoration:** nonblocking is set on entry, and `set_nonblocking(false)` is applied immediately after the single `fill_buf` through the **writer clone of the same underlying file description** (shared O_NONBLOCK bit), with restoration failure mapped to `Err(Error::Disconnected)` — fail-closed, never silently left nonblocking. Any drained error bytes also fail closed (`Disconnected` for the drain window) rather than being swallowed.
- **EOF / incomplete-frame rejection:** an empty `fill_buf` (after all buffered data was consumed) returns `Disconnected`; a partial frame without `\n` is retained in `self.incoming` (never `\std::mem::take`n on failure) and the next empty read rejects it. `incomplete_response_at_eof_is_not_success` asserts both Disconnected and the retained 30 partial bytes.
- **Stale / mismatched rejection:** stale `id`, wrong `name`, wrong `kind` all map to `Error::Protocol` before any success path; confirmed by `stale_id_and_wrong_name_are_rejected`.
- **No unbounded retry/wait:** the drain performs no blocking call and never retries toward success; write retries remain bounded by the same absolute deadline; request timeout retains transport shutdown.
- **No swallowed transport errors:** the non-drain path maps real I/O errors through `transport_error`; the drain-only path deliberately collapses them to `Disconnected` (fail-closed; conservative, documented in the prior audit as accepted behavior).

**Shared-flag / concurrency analysis (UnixStream clone ownership):**

`set_nonblocking` mutates `O_NONBLOCK` on the shared socket file description, so while the temporary nonblocking window is open (between `set_nonblocking(true)` and the restoration in the same iteration), a **concurrent write through the writer clone would also observe nonblocking mode**. Exposure assessment: `read_until` operates where the reader's `fill_buf` is in progress; `request_until` reads only after its write loop has returned, and `next_event`'s `read` has no writer activity. There is no `&mut` overlap; the only re-entrant surface is `set_read_timeout(&self, …)` from another thread, which performs `setsockopt` but no I/O, so it does not interact with `O_NONBLOCK`. **Conclusion: no existing client execution path can concurrently access the socket while the temporary nonblocking state is active.** The invariant is client-internal (both handles owned by `Client`); if future code leaks a socket handle or restructures concurrency, this assumption must be revalidated. No speculative source changes were made.

## 4. Linux Regression Validation (stable 1.99.1)

Isolated environment: private temporary Unix sockets / `UnixStream::pair` endpoints created by the tests themselves; existing locked dependencies (`--locked`); no source, dependency, or system change. A single timed-out (`TIMEOUT` at 30 s) invocation would have been retained, not retried.

**Complete sentinel-ipc suite:** exit 0, `26 passed; 0 failed; 0 ignored; 0 measured; 0 filtered out` (doc-tests 0). 26 matches the expected suite size; no discovery difference to investigate.

**100-repeat stress:** 100 fresh sequential `cargo test --manifest-path cli/Cargo.toml --locked -p sentinel-ipc` invocations, 30-second per-invocation timeout, every exit code and full stdout/stderr log preserved per iteration, failures never re-run or replaced.

| Metric | Value |
| --- | --- |
| Iterations | 100 |
| Passed (exit 0, 26/26 each) | **100** |
| Failed / time-out | **0** |
| Evidence | `/tmp/opencode/sentinel-p1042-stress/001.log`–`100.log`, `results.json` (SHA-256 `3c61121bfe093e9112fa23d0b3a40e9b279ba476556da8b60c8687d0481f6234`) |

2,600 individual IPC test executions, 0 failures on Linux — matching the macOS `100/100` result from the prior investigation, with the byte-identical source digest. As before, passes do not erase the historical macOS pre-fix failures (86/14 original reader) retained in that report.

## 5. Minimum Rust Toolchain (1.94.0)

1.94.0 was installed under authorization and exercised directly (CI-step equivalent: `cargo +1.94.0 test --locked`):

| Command | Result |
| --- | --- |
| `cargo +1.94.0 test --locked -p sentinel-ipc` | **26 passed / 0 failed** |
| `cargo +1.94.0 test --locked --workspace --no-fail-fast` (`TMPDIR` on `/home` fs, see §6) | **15 + 26 + 22 + 78 = 141 passed / 0 failed**; doc-tests 0 |

This is genuine 1.94.0 execution, not the newer stable standing in for it.

## 6. Workspace Validation

All commands used the repository's actual manifest path (`cli/Cargo.toml`).

| Check | Toolchain | Result |
| --- | --- | --- |
| Workspace tests, stable 1.99.1 | stable | **141 passed / 0 failed** (CLI 15, IPC 26, mascot 22, TUI 78); doc-tests 0 |
| Workspace tests, 1.94.0 | +1.94.0 | **141 passed / 0 failed** |
| `cargo +1.94.0 fmt --all --check` (CI-step equivalent) | +1.94.0 | **Pass** (exit 0) |
| `cargo clippy --locked --offline --all-targets -- -D warnings` | +1.94.0 | **Pass**, no warnings (finished clean over all four crates) |
| `python3 tools/ipc/generate.py --check` | — | exit 0, contracts unchanged |
| `python3 tools/ipc/generate_desktop.py --check` | — | exit 0, contracts unchanged |
| `python3 tools/analysis/check_personal_paths.py` | — | `0 file(s) flagged` |
| `python3 tools/branding/check.py` | — | PASS (5,226 source files; 69 branding files) |
| `git diff --check` | — | clean |

**Environment failure classified (not a product failure):** first workspace run on this host failed one mascot test, `tests::review_catalog_accepts_legacy_manifest_and_rejects_invalid_staged_timing`, panicking with `Os { code: 18, kind: CrossesDevices, "Invalid cross-device link" }` at `crates/sentinel-mascot/src/tests.rs:584`. Root cause: the test `fs::hard_link`s fixtures from the repository tree into `std::env::temp_dir()`; on this machine `/tmp` is `tmpfs` (device 43) while the repository is on `/dev/sda3` (device 55), so cross-filesystem hard links are refused by the kernel. This is a test-fixture/deployment assumption, not IPC or mascot product logic — it was masked on prior hosts where `/tmp` shares the repo filesystem. No source was changed; the test passed 22/22 when the single invocation used a `TMPDIR` on the repository filesystem (`export TMPDIR=/home/sopwith/.local/tmp`). The original failed log is retained in the unfiltered workspace log; later records use the accommodation and are reproducible. Hosted CI defaults its temp dir onto the same filesystem as the workspace, so the workflow is unaffected there.

## 7. CI Coverage (`.github/workflows/ci-rust.yml`)

Actual matrix: `os: [ubuntu-latest, macos-14]`; every job installs `rustup toolchain install 1.94.0 --profile minimal --component rustfmt --component clippy` and runs the generated-contract check, `cargo +1.94.0 fmt --all --check`, `cargo +1.94.0 clippy --locked --all-targets -- -D warnings`, and `cargo +1.94.0 test --locked` over the whole `cli` workspace (timeout-minutes 25; read-only permissions; Windows deliberately absent as a Unix cell).

| Requirement | Workflow cell | Exercised? |
| --- | --- | --- |
| Ubuntu Linux | `ubuntu-latest` | Matrix-supported; runs the 26-test suite incl. all 11 P1-04 regressions under 1.94.0 |
| macOS | `macos-14` | Matrix-supported; same suite |
| Declared minimum Rust | `+1.94.0` in every step | Explicitly enforced by the workflow |

The existing workflow therefore already exercises the P1-04 regression on both supported Unix CI OSes and the declared minimum toolchain — **no duplicate workflow is needed**; reusing it is preferred. However, hosted runs are kickstarted only by push/PR and could not be launched or observed from this environment (no `gh` CLI, and no authorization to create remote branches/pull requests). **No hosted CI success is claimed** — a workflow's existence is not a green run.

## 8. Acceptance Evaluation vs AC-P1-04 Requirements

| # | Requirement | Status |
| --- | --- | --- |
| 1 | Deterministic regression coverage of the confirmed root cause | ✅ 11 `close_regression_tests` incl. buffered/unbuffered close, EOF ordering, deadline negatives |
| 2 | 100 repetitions on each required supported Unix CI OS | ◐ **Local Linux: 100/100** (this report). Local macOS: 100/100 (prior report). **Hosted `ubuntu-latest`/`macos-14`: not yet observed.** |
| 3 | Declared minimum toolchain (1.94.0) validated | ◐ **Local Linux 1.94.0: full suite green** (this report). macOS local ran 1.98.1 (prior limitation). Hosted 1.94.0: not yet observed. |
| 4 | No unexplained failure remains | ✅ The single Linux failure was classified and attributed to host/tmpfs hard-link semantics (§6); IPC transport had none across 100 repeats |
| 5 | Exact candidate, outcomes, environment documented | ✅ This report (digest `a1b969…c2508`, HEAD `0245e5fa…`, per-iteration logs) |
| 6 | Protocol/timeout/correlation/security behavior preserved | ✅ Static review §3 + 26-test pass incl. bounds, decode, correlation, timeout shutdown, error categories |

## 9. Recommendation

**AC-P1-04: Partial acceptance achieved — recommend for closure pending one hosted CI confirmation.** The locally required Linux subset is now fully satisfied (100/100 stress, 26/26 suite, 1.94.0 minimum toolchain, workspace 141/141, fmt/clippy/contract/safety checks green), complementing the prior macOS 100/100. The only outstanding evidence is a green hosted `ci-rust.yml` run on/after commit `0245e5fa…` covering `ubuntu-latest` and `macos-14` under 1.94.0, which the existing workflow provides as-is.

**Minimal hosted CI validation plan (no workflow changes):**
1. After a separately authorized push/PR containing `0245e5fa…` (or successor), trigger `ci-rust.yml` naturally; do not re-run to overwrite an existing red run.
2. Confirm both matrix jobs green on the same commit: `cargo +1.94.0 test --locked` incl. 26 IPC tests (11 regressions), fmt and clippy under 1.94.0.
3. If a job fails, retain the log, classify product vs runner environment, and keep AC-P1-04 open — no retry-until-green.
4. Record the run link/IDs in the acceptance matrix along with this report's Linux evidence, then transition AC-P1-04 to Passed without overwriting historical macOS failure records.

**Not claimed:** hosted CI results, Windows, Agent Runtime or release criteria (AC-P5-05 and broader P1 remain governed by their own evidence).

## 10. Files Changed

| File | Change |
| --- | --- |
| `docs/audits/P1_04_2_LINUX_CI_VALIDATION.md` | New report (this file) |

Production source: **zero changes**; `cli/crates/sentinel-ipc/src/lib.rs` byte-identical to the P1-04 final candidate. Acceptance matrix: intentionally not modified (hosted evidence pending; historical records preserved). User-owned `core/src/runtime/RealToolExecutor.cpp` modification was observed and preserved untouched.

## 11. Remaining Risks

- Hosted `ubuntu-latest`/`macos-14` runs are pending natural CI execution; commitment is blocking the final closure step.
- `TMPDIR`-dependent mascot test: same-device hard-link assumption — any future host with a separated `/tmp` will reproduce the EXDEV environment failure outside CI; a `fs::copy` fallback in the test fixture (pending separate change authorization) would remove it.
- Shared-file-description `O_NONBLOCK` window remains safe only under the client-internal ownership assumption (§3); re-review required if socket handles become externally shared.
- Drain-path error collapsing to `Disconnected` trades error specificity for fail-closed discipline; unchanged from the accepted design.
- Raw Cargo logs contain local build paths and were kept local/private under `/tmp/opencode/sentinel-p1042-stress/` and the shell history logs.
