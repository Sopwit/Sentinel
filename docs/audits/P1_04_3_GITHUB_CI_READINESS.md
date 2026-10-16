# P1-04.3 — GitHub Integration & Hosted CI Readiness

Date: 2026-10-16. Finding F02; criterion AC-P1-04. Previous reports kept intact: `P1_04_IPC_HANDSHAKE_INVESTIGATION.md` (macOS, digest `a1b969…c2508`), `P1_04_2_LINUX_CI_VALIDATION.md` (Fedora local, 100/100). Scope: repository/PR/CI readiness of the already-committed IPC reader correction. **No source, workflow, or CI changes were made in this task**; the only edits are this new report plus a narrowly justified test-inventory count correction in the P1-04.2 report (§3.2). No commits, pushes, PRs, issues, or remote settings changes were performed.

## 1. Repository Preflight

| Item | Observed state |
| --- | --- |
| Branch / HEAD | `main` / `0245e5fa94584a357a6b1de01344f69b577e0e41` (merge of `codex/p2-cross-platform-ci-stabilization`) |
| Local vs `origin/main` | Identical (`origin/main` = `0245e5fa…`); `git fetch --dry-run` reports nothing to fetch; branch reported "up to date". Zero divergence. |
| Local correction commit | `16dea2c4 fix(ipc): resolve client reader defect causing premature disconnection on peer closure` — touches only `cli/crates/sentinel-ipc/src/lib.rs` (+300 lines, mostly tests) and `docs/audits/P1_04_IPC_HANDSHAKE_INVESTIGATION.md` (+191). |
| Correction on remote default branch | **Yes.** `16dea2c4` is the second parent of merge `0245e5fa`, which is `origin/main` HEAD. `git log origin/main` contains it directly. |
| Working tree modifications | `core/src/runtime/RealToolExecutor.cpp` — `M`, 5+/5−, unrelated user-owned shadowing cleanup (`active` → `entry`) inside `executeAsync` lambdas. Preserved byte-for-byte, excluded from all claims. |
| Untracked documentation | `docs/audits/P1_04_2_LINUX_CI_VALIDATION.md` (P1-04.2 report, not yet committed). No other pending docs. |
| Forbidden operations | No force push, reset, rebase, cherry-pick, amend, stash, stage or automatic commit was performed. |

**Source digest verified this session:** `sha256sum cli/crates/sentinel-ipc/src/lib.rs` → `a1b969acbbce0b491681a3f679164311692a5a711f788412d3984bf9a1ec2508` — byte-identical to the P1-04 / P1-04.2 final candidate, and `git diff HEAD` on that file is empty.

## 2. Test Inventory — 25-vs-26 Discrepancy Resolved

Real discovery on this machine (Rust toolchain installed locally, `--locked`):

```sh
$ cargo test --locked -p sentinel-ipc -- --list
…
26 tests, 0 benchmarks
   Doc-tests sentinel_ipc
0 tests, 0 benchmarks
```

Exact per-module split from source (`lib.rs`) + discovery:

| Module | Count | Tests |
| --- | --- | --- |
| `tests` | **12** | `command_reconnection_policy_preserves_business_errors`, `rejects_malformed`, `rejects_oversized`, `event_decoding`, `incompatible`, `exit_codes`, `partial_frame_survives_poll_timeout`, `expired_deadline_rejects_buffered_frame_without_consuming_it`, `request_timeout_closes_transport`, `successful_request_restores_polling_timeout`, `stable_error_categories`, `missing_daemon` |
| `transport_tests` | **2** | `handshake_correlation_and_event_queue` (the originally failing test), `typed_request_invalid_enum_or_fields` |
| `generation_compatibility_tests` | **1** | `hello_generation_is_additive` |
| `close_regression_tests` (new P1-04 regressions) | **11** | `buffered_response_after_confirmed_peer_close`, `unbuffered_complete_response_after_confirmed_peer_close`, `incomplete_response_at_eof_is_not_success`, `handshake_event_order_and_immediate_close`, `fragmented_frame_writes_and_large_request`, `expired_write_deadline_sends_no_frame`, `sequential_requests_restore_both_timeouts`, `stale_id_and_wrong_name_are_rejected`, `blocked_writer_deadline_closes_transport`, `peer_closes_without_response_is_disconnected`, `client_drop_closes_all_socket_handles` |
| Doc-tests | 0 | — |
| **Total** | **26** | |

**Discrepancy resolution:** the earlier reports labeled `tests` as 11 (it actually contains 12 functions) and grouped `hello_generation_is_additive` under `transport_tests` (it actually lives in its own `generation_compatibility_tests` module), so the arithmetic read 11+3+11 = 25 while actual discovery returns 26. True split: **12 + 2 + 1 + 11 = 26**. Counts reconcile by test names; no test was deleted or added.

**Narrow documentation correction applied (the only permitted edit):** `docs/audits/P1_04_2_LINUX_CI_VALIDATION.md` §2 inventory corrected (12 / 2 / 1 / 11) with a reconciliation note. Historical P1-04 wording ("eleven added regression functions") remains accurate as-is: 11 refers to `close_regression_tests`.

## 3. Acceptance Evidence Review

| Requirement (§3 of task) | Covered by | Status |
| --- | --- | --- |
| macOS local 100-repeat stress | P1_04 investigation §9 (final 100/100, results.json `1b3eef9d…`) | ✅ |
| Fedora local 100-repeat stress | P1_04_2 §4 (100/100, results.json `3c61121b…`) | ✅ |
| Rust 1.94.0 workspace validation | P1_04_2 §5 (141/141 incl. IPC 26/26; fmt, clippy, contract checks) | ✅ |
| Identical production source digest | Both reports record `a1b969…c2508`; re-hashed again this session | ✅ |
| Original regression reproduction | P1_04 §4 (first isolated failure 86/14, deterministic `EINVAL` pre-fix evidence) | ✅ |
| Negative timeout / EOF / correlation cases | P1_04 §8 + P1_04_2 §3 (`expired_deadline_rejects…`, `incomplete_response_at_eof…`, `stale_id_and_wrong_name…`, `blocked_writer_deadline_closes_transport`, `peer_closes_without_response…`) | ✅ |
| Working-tree preservation | P1_04_2 §1, P1_04 §2, and §1 of this report | ✅ |

**Remaining hosted-CI requirement exactly:** the acceptance matrix (`docs/roadmaps/ACCEPTANCE_MATRIX.md` line 136) requires "100 repetitions per supported Unix CI OS" as the isolated Unix handshake repeat policy. Local host runs do not substitute hosted `ubuntu-latest`/`macos-14` runners. A hosted run executing the IPC suite **once** is not evidence of that requirement.

**Update from this session (remote inspection only):** hosted workflow `ci-rust.yml` run **38872661627** (push to `main`, 2026-10-16 08:17Z, headSha `0245e5fa…` — the merge containing `16dea2c4`) completed **success on both matrix jobs** (`rust (ubuntu-latest)` job `116880190161`, `rust (macos-14)` job `116880189939`), including all steps: `rustup toolchain install 1.94.0`, `python3 ../tools/ipc/generate.py --check`, `cargo +1.94.0 fmt --all --check`, `cargo +1.94.0 clippy --locked --all-targets -- -D warnings`, `cargo +1.94.0 test --locked`. This is genuine hosted verification of format/Clippy/contracts/full-workspace-tests under 1.94.0 on both required OSes at the exact candidate commit — **but it is a one-time normal run and does not satisfy the 100-repeat requirement.**

## 4. Existing CI Coverage vs Requirements

`.github/workflows/ci-rust.yml` (unchanged):

| Requirement | Present? | Detail |
| --- | --- | --- |
| Ubuntu and macOS | ✅ | matrix `os: [ubuntu-latest, macos-14]` |
| Rust 1.94.0 | ✅ | `rustup toolchain install 1.94.0 --profile minimal --component rustfmt --component clippy`; every step `+1.94.0` |
| Full Rust workspace | ✅ | `cargo +1.94.0 test --locked` (whole `cli` workspace; IPC 26, CLI 15, mascot 22, TUI 78) |
| Formatting and Clippy | ✅ | `fmt --all --check`, `clippy --locked --all-targets -- -D warnings` |
| New IPC tests | ✅ | part of the workspace run; 11 `close_regression_tests` included |
| **100 full IPC-suite repetitions per required OS** | ❌ **missing** | single `cargo test` invocation; nothing repeats the suite |

Hosted run 38872661627 confirms points 1–5 in reality, not just in workflow text.

### 4.1 Proposed smallest bounded addition (for maintainer review — NOT applied)

Principles: no duplicate workflow, no new dependencies, no always-on expensive job, PR-only opt-in trigger, per-iteration evidence retained, clear failure summary, failure does not retry.

```yaml
# In the existing .github/workflows/ci-rust.yml — three additions, nothing else changes:

# (1) Add one trigger key to the existing `on:` block (label-gated PR / manual dispatch):
on:
  push:
    branches: [main]
  pull_request:
  workflow_dispatch:

# (2) Add one job after the existing `rust` job (same workflow, same file):
  ipc-stress:
    if: >-
      github.event_name == 'workflow_dispatch' ||
      (github.event_name == 'pull_request' &&
       contains(github.event.pull_request.labels.*.name, 'ci:ipc-stress'))
    timeout-minutes: 30
    strategy:
      matrix:
        os: [ubuntu-latest, macos-14]
    runs-on: ${{ matrix.os }}
    defaults:
      run:
        working-directory: cli
    steps:
      - uses: actions/checkout@3d3c42e5aac5ba805825da76410c181273ba90b1
        with:
          persist-credentials: false
      - run: rustup toolchain install 1.94.0 --profile minimal
      - name: 100-repeat sentinel-ipc suite (no retry, no pass-on-failure)
        run: |
          set -u
          : > iteration-results.txt
          failed=""
          for i in $(seq 100); do
            if cargo +1.94.0 test --locked -q -p sentinel-ipc > "iter-$i.log" 2>&1; then
              printf '%s pass\n' "$i" >> iteration-results.txt
            else
              printf '%s fail\n' "$i" >> iteration-results.txt
              failed="$failed $i"
            fi
          done
          {
            echo "### sentinel-ipc 100-repeat stress (${{ matrix.os }})"
            echo "| iteration | result |"; echo "| --- | --- |"
            awk '{print "| " $1 " | " $2 " |"}' iteration-results.txt
            echo ""
            if [ -n "$failed" ]; then
              echo "FAILED iterations:${failed}" | tee /dev/stderr
              grep -h -E "FAILED|panicked" iter-*.log || true
              exit 1
            fi
            echo "100/100 iterations passed"
          } >> "$GITHUB_STEP_SUMMARY"
      - name: Retain per-iteration logs
        if: always()
        uses: actions/upload-artifact@v4
        with:
          name: ipc-stress-${{ matrix.os }}
          path: |
            cli/iteration-results.txt
            cli/iter-*.log
          retention-days: 14
```

Notes for the reviewer:

* The loop body is exactly `cargo +1.94.0 test --locked -q -p sentinel-ipc`: the **whole** 26-test suite runs once per iteration, sequentially, 100 times, and each iteration's outcome is recorded independently.
* Cost bound: the existing jobs execute the suite in well under two minutes including build; the compiled-test-only loop adds ~2–5 min per OS, once, only when labeled. `concurrency` cancellation already applies to the whole workflow.
* Trigger surface: `workflow_dispatch` for one-off hosted validation (the immediate P1-04 need), plus per-PR label `ci:ipc-stress` so it never runs on ordinary pushes/PRs. No schedule, no push path, so no always-on expense.
* Contract compliance: sequential fresh iterations, no retry, every outcome retained, one unexplained failure fails the job (`exit 1`); matches ACCEPTANCE_MATRIX repeat policy and "no retry-until-green".

## 5. GitHub PR Readiness

| Question | Answer |
| --- | --- |
| Correction on remote default branch? | **Yes** — `16dea2c4` is inside `origin/main` (`0245e5fa`). Nothing to PR for the source itself. |
| Existing PR? | **#17** (draft, `fix/p2-cross-platform-ci-stabilization` → `main`) whose branch content was merged into `main` at `0245e5fa` while the draft remains OPEN; plus six unrelated dependabot PRs. Recommend maintainers merge/close #17 (its head is already integrated; a hosted stress run on a fresh "Sync" would be re-run only if desired). |
| Can a new PR contain only the correction + audit evidence? | **Not as originally assumed — the source correction cannot be PR'd against main because it is already in main.** What remains PR-able is a **pure-documentation branch** containing the untracked audit reports and this report; zero source delta. |
| Local main ahead of origin/main? | No — `0245e5fa…` both sides; only untracked docs and the unrelated user-owned `RealToolExecutor.cpp` modification, neither of which belongs to any PR. |
| Pending documentation file to commit? | `docs/audits/P1_04_2_LINUX_CI_VALIDATION.md` (untracked) plus this report and its narrow correction; all are P1-04 evidence, safe to publish (§11 of P1-04.2: raw local logs stay local; reports contain no personal paths). |

**Proposed PR (documentation-only).** Branch example `docs/p1-04-audit-evidence`, containing exactly: `P1_04_IPC_HANDSHAKE_INVESTIGATION.md` (already in main via 16dea2c4 — unchanged), `P1_04_2_LINUX_CI_VALIDATION.md` (new), `P1_04_3_GITHUB_CI_READINESS.md` (new, with the narrow §2 correction of P1_04.2).

*Title:* `docs(audit): record P1-04 IPC correction evidence (macOS + Fedora validation) and hosted-CI readiness`

*Description skeleton:*
> Closes the documentation side of [P1-04] / finding F02 / AC-P1-04. No source changes — the reader correction is already in `main` (16dea2c4 within 0245e5fa, verified by SHA-256 `a1b969…c2508` in both audits).
> - P1-04.1: macOS investigation; original failure 86/14 → deterministic root cause (`set_read_timeout` EINVAL after peer close discards a complete buffered response) → 100/100 local stress.
> - P1-04.2: Fedora 100/100 local stress, Rust 1.94.0 workspace 141/141, fmt/clippy/contracts green; CI gap analysis.
> - P1-04.3 (this): GitHub/PR/CI readiness; hosted `ci-rust.yml` run 38872661627 green at 0245e5fa (one-time); propose label-gated `ipc-stress` job for the required 100-repeat hosted validation.
> Test inventory reconciled at exactly 26 tests (12/2/1/11) via `cargo test -- --list`.
> Remaining to close AC-P1-04: one hosted 100-repeat dispatch (both OSes) and issue lifecycle — see checklist.

*Test evidence:* the three reports + hosted run link 38872661627 + per-iteration stress digests.
*Closing criteria:* both hosted `ipc-stress` jobs 100/100 (or failure classified) recorded in the acceptance matrix; [P1-04] issue closed with DoD complete.

No PR may be created from local `main` for the correction (nothing to push), and none is created here without explicit maintainer authorization.

## 6. Proposed GitHub Issue — `[P1-04] Investigate intermittent Rust IPC handshake disconnect`

> Parent: `EP-P1` · Finding: F02 · Acceptance: AC-P1-04, AC-P5-05 · Repository: sentinel · Labels: `type:defect`, `area:ipc` — per GITHUB_ISSUE_BACKLOG.md.
>
> ### Root cause (confirmed on macOS host)
> The original `Client::read_until` called `set_read_timeout` before every `fill_buf`, **even when BufReader already held a complete response**. After a peer closes its handles the macOS setter fails with `EINVAL` (OS code 22); the old code mapped that straight to `Error::Disconnected`, discarding the complete delivered response. Scheduling decided whether the option call preceded peer close — hence the intermittent `Disconnected` in `transport_tests::handshake_correlation_and_event_queue`.
>
> ### Reproduction: before / after
> | Candidate | Iterations | Result |
> | --- | --- | --- |
> | Original reader (macOS, arm64, Rust 1.98.1) | 100 | **86 pass / 14 fail** — every failure `Disconnected`, iterations 3,5,6,7,10,24,26,31,33,65,67,86,95,97 retained |
> | Corrected reader, ingress-first (macOS) | 100 | **100 / 0** |
> | Corrected reader (Fedora 44, x86_64, Rust 1.99.1 and toolchain 1.94.0) | 100 | **100 / 0** per toolchain |
> Hosted CI (ubuntu-latest + macos-14, Rust 1.94.0): run 38872661627 green at commit 0245e5fa (one-time suite run). Pre-fix red suite with the new regressions: 4 passed / 5 failed against the unchanged reader; passes never erased the historical failures.
>
> ### Source change
> Single production delta in `Client::read_until` (`cli/crates/sentinel-ipc/src/lib.rs`, commit `16dea2c4`, digest `a1b969ac…c2508`): check `remaining_budget` before every fill (incl. buffered data); skip the socket-option call when bytes are already buffered; if the setter fails on an empty buffer, attempt exactly one bounded nonblocking drain, restore blocking mode through the writer clone of the same file description (restoration failure ⇒ `Disconnected`, fail-closed); EOF/incomplete frames still fail; correlation, limits, timeout shutdown, protocol unchanged.
>
> ### Regression tests
> 11 new `close_regression_tests`: buffered/unbuffered-complete-response-after-peer-close, incomplete-at-EOF (never success), handshake event-order (response-only + both event variants), fragmented writes + 20k-byte request, expired write deadline sends no frame, sequential timeout restoration, stale-ID/wrong-name rejection, blocked-writer deadline closes transport, peer-close-without-response, client-drop closes handles. Suite totals exactly **26 tests** (12 `tests` + 2 `transport` + 1 `generation` + 11 `close_regression`; doc-tests 0).
>
> ### Cross-platform evidence
> macOS local: red→green + 100/100. Fedora local: 100/100; workspace 141/141 on 1.94.0; fmt/clippy/contract checks green. Hosted `ubuntu-latest` + `macos-14` @ 1.94.0: full workflow green once (run 38872661627). Windows out of scope (named-pipe transport unchanged).
>
> ### Remaining CI requirements (DoD gap)
> - [ ] Hosted **100 full IPC-suite repetitions** per required OS (`ubuntu-latest`, `macos-14`) with per-iteration outcomes retained — a one-time `cargo test` does not satisfy this.
> - [ ] Proposed label-gated `ipc-stress` job in `ci-rust.yml` (see P1_04_3 §4.1) awaiting maintainer review; run once via `workflow_dispatch` or `ci:ipc-stress` label.
> - [ ] Record hosted run IDs in ACCEPTANCE_MATRIX / re-assess AC-P1-04 without overwriting historical 86/14 records.
>
> ### Definition of Done
> Deterministic regression coverage of the confirmed root cause; 100 repetitions on each supported Unix CI OS with zero unexplained failures (hosted, per matrix policy); declared minimum toolchain 1.94.0 exercised; no unexplained failure remains; request deadline / transport invalidation / late-reply fencing / no-replay contracts stay intact; exact-candidate evidence linked (digests, run IDs); historical failure records preserved.
>
> Out of scope: retry-to-green, timeout increases without explanation, fixture keep-alive hacks, Windows implementation. See **Finding F02**, acceptance criterion **AC-P1-04** (implementation evidence also touches AC-P5-05 contract checks).

Not posted; creation requires a separate explicit instruction.

## 7. Independent Follow-up — sentinel-mascot EXDEV

Candidate backlog entry (low priority, **separate** task; P1-04 touches neither mascot source nor tests):

> **sentinel-mascot test portability: fixture hard-link assumes same filesystem.**
> `crates/sentinel-mascot/src/tests.rs` (`review_catalog_accepts_legacy_manifest_and_rejects_invalid_staged_timing`, ~line 584) `fs::hard_link`s fixtures from the repository tree into `std::env::temp_dir()`. On hosts where `/tmp` is tmpfs and the repository is on another device the link fails with `EXDEV` (`Os { code: 18, kind: CrossesDevices }`) — observed once on Fedora 44 and classified as environment, not product, failure in P1_04_2 §6. Proposed minimal remedy: `fs::copy` fallback in the test fixture (or `tempfile::tempdir_in(workspace)`), test-only, no product code. Low priority because default CI temp dirs share the workspace filesystem. Label: `type:validation`, `area:automation`.

## 8. Files Changed in This Task

| File | Change |
| --- | --- |
| `docs/audits/P1_04_3_GITHUB_CI_READINESS.md` | New report (this file) |
| `docs/audits/P1_04_2_LINUX_CI_VALIDATION.md` | §2 inventory counts corrected 11/3/11 → 12/2/1/11 with reconciliation note (narrowly justified; reclassified doc tracking as untracked working-tree item, not committed) |

No source, workflow, manifest, or acceptance-matrix files changed. `RealToolExecutor.cpp` preserved untouched.

## 9. AC-P1-04 Closure Assessment

**Not yet closable — one bounded gap remains.** Local requirements (root cause, regressions, 100/100 on both local hosts, 1.94.0 toolchain, negative cases, documentation) are complete; hosted one-time full workflow is now verified green at the exact candidate commit. The single outstanding requirement is hosted **100-repeat** validation per required OS, which needs the `ipc-stress` job addition (or an instruction to treat the green hosted workspace runs as an alternative evidence base — a matrix-policy decision, not this agent's to make). Historical 86/14 macOS record remains intact.
