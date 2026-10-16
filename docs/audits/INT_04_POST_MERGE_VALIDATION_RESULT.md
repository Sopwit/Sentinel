# INT-04 — Post-merge integration and cross-platform validation

Audit date: 2026-10-16 (Europe/Istanbul). Independent, read-only audit of the reported
integrated `main` plus a **local** product regression on the audit host. No commit, push,
merge, reset, stash, PR, workflow dispatch or CI trigger was performed.

Absolute workstation paths, the new audit worktree path and sensitive historical identifiers
are intentionally excluded or abstracted. This report does not rewrite any historical report,
acceptance matrix or decision register.

## 1. Audit identity and method

| Item | Value |
| --- | --- |
| Reported integrated main | `70e9fa072af9b4139d86f518c6d89f9c849c29b2` |
| Isolation | New worktree `int-04-post-merge-validation/Sentinel-sanitized`, branch `audit/int-04-post-merge-validation`, created from `origin/main` at that SHA |
| Local `main` | `70e9fa07…` |
| Stored `origin/main` | `70e9fa07…` |
| Live `git ls-remote origin main` | `70e9fa07…` |
| GitHub branch API `heads/main` | `70e9fa07…` (`protected: false`) |
| Baseline for progress delta | previous integrated main `619e989c419956393201a749da1cede386e83ba3` |

All four identity sources agree on `70e9fa07…`. Preflight read `AGENTS.md`, INT-01,
INT-02, the INT-03 reconciliation snapshot (uncommitted in its own worktree), the P1-01/
P1-01.7/P1-04/P2-05–08/P3-01–03 audit reports, the acceptance matrix and decision register.

Method: read-only Git/remote/API inspection, ancestry and merge-order analysis, file-presence
and SHA-256 comparison, a full CMake configure/build, complete CTest, focused QtTest runs,
Rust `fmt`/`clippy`/`test`, IPC generated-contract checks, repository personal-path scanner
and a changed-line `clang-format` gate. Hosted results are cited from GitHub Actions for the
exact candidate SHA. **Hosted jobs were not triggered or re-run.**

The audit host is one machine: macOS (Darwin arm64), AppleClang 21.0.0, Qt **6.12.0**,
CMake 4.4.4. Hosted CI uses Qt 6.11 on `ubuntu-22.04` / `macos-14` / `windows-2022` with the
CI-pinned toolchains. Local results are host-scoped and are not cross-platform certification.

## 2. Git integration integrity

### 2.1 Expected merged branches — all contained

Every expected P0/P1/P2/P3 integration head is an ancestor of `main`:

| Branch | HEAD | Disposition |
| --- | --- | --- |
| `test/p3-01-lifecycle-ownership` | `53d0032a` | merged at `70e9fa07` (and earlier `42b9d08e` → see 2.3) |
| `test/p2-01-provider-contracts` | `f87843f1` | merged at `67a80a0c` |
| `integration/p1-01-7-bounded-planner` | `9cdbd883` | merged at `a69a9f37` |
| `test/p1-01-agent-reliability` | `91640b24` | merged at `74ba91e2` |
| `review/int-02-sec-01-integration` | `467021b6` | merged at `1d7f4a5a` |
| `fix/sec-01-audit-privacy` | `58355635` | merged at `78a80d93` |
| `feat/p2-integration` | `ebad8152` | merged earlier (`8f6a8b87`) |
| `docs/p0-architecture-consistency` | `6ee75bb0` | merged earlier (`52a4e516`) |
| `docs/p0-03-decisions` | `fe82445b` | merged earlier |
| `ci/p1-04-ipc-stress` | `d4230063` | merged earlier |
| `feat/p4-desktop-live-integration` | `0245e5fa` | merged earlier |

**Not in `main`:** `fix/p1-05-legacy-cli-exits` = `43c7bef0` (one branch-only commit, merge base
`e71cca4e`); `fix/p2-cross-platform-ci-stabilization` (see 2.4). There is no registered local
P1-05 checkout.

### 2.2 Progress delta and file preservation

Range `619e989c..70e9fa07`: **39 files changed; 26 files added; 0 files deleted.**
All 26 added paths are present in `HEAD` (`missing = 0`). No P1/P2/P3 source/test/report file is
missing. Working tree and index are clean (only ignored `build/` output exists).

Verified present with the expected content:

- P1-01.7 planner streaming markers in `core/src/agent/LlmAgentRuntime.cpp`
  (`kMaxPlannerStreamedChars`, `requireBoundedStreaming`, `supportsBoundedStreaming()`,
  options-carrying `sendMessageStreaming`, "Planner streamed output exceeded" fail-closed),
  and **no** planner delta is forwarded to `streamObserver_`.
- P2-05 bounded-streaming contract (`IChatProvider` overload, `test_bounded_streaming_contract`).
- P2-06/P2-07 native/vendor fixtures and the `LocalInferenceTransport.h` injection seam.
- P2-08 endpoint security guards (`isLoopbackHost`, `LMStudioConfig::isSecureTransportEndpoint`,
  `isAllowedEndpoint`, `NetworkPolicyService`), with `e33db6de` an ancestor.
- P3-03 destructor correction committed as `09060055` (13-line diff: disconnect + fence + abort
  the owned probe without a borrowed callback).
- SEC-01 remediated `docs/audits/P1_04_2_LINUX_CI_VALIDATION.md` and receipt.

### 2.3 Ancestry and merge order

First-parent chain:

```
619e989c -> 0183f154 -> 78a80d93(SEC) -> 1d7f4a5a(INT-02) -> 42b9d08e(P3 early)
         -> 74ba91e2(P1) -> a69a9f37(P1-01.7 planner) -> 67a80a0c(P2) -> 70e9fa07(P3 final)
```

`test/p3-01-lifecycle-ownership` is merged **twice**: `42b9d08e` imported `5d8ce5ba`, then
`70e9fa07` imported `09060055`/`53d0032a` (destructor correction + P3-03 report). This is a normal
re-merge of an advanced branch, not duplicated content. Merge commits record containment, **not**
review or acceptance.

### 2.4 `fix/p2-cross-platform-ci-stabilization` status

- Remote branch `origin/fix/p2-cross-platform-ci-stabilization` = `55190cfb` (merge of PR #18).
- Local branch = `0f4d314f` (one extra `Merge branch 'main'` commit), 2 commits ahead of / 47
  commits behind current `main` — stale.
- **PR #17** is OPEN and DRAFT against `main` (base `b53ed0f5`, head `55190cfb`, `mergeable: true`,
  last updated 2026-10-15). Not merged. It is an old-base branch and is unrelated to the
  integrated PM merges; it also lacks the current `test_llm_agent_runtime.cpp`.
- Related dependency PRs #8 and #12–16 remain OPEN.

### 2.5 Test-target registration, duplicates and generated files

- `tests/CMakeLists.txt` is the single registration point; `add_sentinel_core_test` and
  `add_test(NAME …)` names are unique across all `CMakeLists.txt`.
- `ctest -N` enumerates **136 unique** test names (no duplicate target).
- IPC generated-contract checks pass: `tools/ipc/generate.py --check` → 0,
  `tools/ipc/generate_desktop.py --check` → 0 (no generated change).
- Tracked `*.generated.h`/`*.generated.inc` are canonical inputs, not stray output; no unexpected
  untracked generated file exists (`aqtinstall.log` is untracked/ignored, not in Git).

### 2.6 Code whose acceptance/approval is still pending

The P1/P2/P3 changes reached `main` through **direct maintainer merges with no pull request and no
independent reviewer** (PR list contains no PR for `test/p1-01…`, `test/p2-01…`, `test/p3-01…`,
`integration/p1-01-7…`, `fix/sec-01…`, `review/int-02…`). Containment is therefore not approval.
All remain subject to the open acceptance criteria in §6.

## 3. Local regression (audit host)

### 3.1 Build and complete CTest

| Check | Observed result |
| --- | --- |
| `cmake --preset tests` | Configure OK (Qt 6.12.0, Ninja, AppleClang 21, ccache) |
| `cmake --build --preset tests` | **Build OK**, 1513 build steps, exit 0 |
| `QT_QPA_PLATFORM=offscreen ctest --preset tests --output-on-failure` | **136/136 passed**, 206.76 s, 0 failures |
| `ctest -N` target discovery | 136 targets |

The reported **136/136 local total is reproducible on this host.** It is a single-machine result.

### 3.2 Focused suites (actual QtTest counts on this host)

| Target | Result |
| --- | --- |
| `test_agent_reliability_harness` (P1) | 97 passed, 0 failed, **1 skipped** (native tool-assembly row) |
| `test_agent_reliability_ipc` (P1 COR-17/18) | 5 passed, 0 failed, 0 skipped |
| `test_llm_agent_runtime` | 49 passed, 0 failed, 0 skipped |
| `test_bounded_streaming_contract` (P2-05) | 18 passed, 0 failed, 0 skipped |
| `test_provider_contracts` (P2) | 205 passed, 0 failed |
| `test_native_provider_contract` (P2-06) | 76 passed, 0 failed |
| `test_vendor_cloud_contract` (P2-07) | 340 passed, 0 failed |
| `test_endpoint_security_capability` (P2-08) | 248 passed, 0 failed |
| `test_model_lifecycle_ownership` (P3-01) | 13 passed, 0 failed, 0 skipped |
| `test_local_model_runtime` (P3-02/03) | 49 passed, 0 failed, **1 skipped** (D6 in-use deletion) |

These match the individual workstream reports' focused totals. The two remaining skips are the
explicitly documented, asserted gaps (native incremental tool assembly; D6 residency lease).

### 3.3 Rust IPC gates (from `cli/`, toolchain `+1.94.0`)

| Check | Result |
| --- | --- |
| `cargo +1.94.0 fmt --all --check` | exit 0 |
| `cargo +1.94.0 clippy --locked --all-targets -- -D warnings` | exit 0, no warnings |
| `cargo +1.94.0 test --locked` | exit 0 — sentinel-cli 15 + sentinel-ipc 26 + sentinel-mascot 22 + sentinel-tui 78 = **141 passed, 0 failed**; doc-tests 0 |
| `cargo +1.94.0 test --locked -p sentinel-ipc` | 26 passed, 0 failed |
| `python3 tools/ipc/generate.py --check` | exit 0 |

### 3.4 Formatting, static analysis, privacy

| Gate | Result |
| --- | --- |
| Repository personal-path scan (`tools/analysis/check_personal_paths.py`) | **PASS** — 0 files flagged |
| `git diff --check 619e989c..HEAD` | clean (exit 0) |
| Changed-line `clang-format` (CI-pinned **18.1.8**) over the **full integrated delta** `git diff -U0 619e989c..70e9fa07 -- apps core integrations plugins tests \| clang-format-diff.py -p1` | **FAIL — 1 violation**: `core/src/runtime/OllamaRuntime.cpp` include ordering (`ProviderRequestRuntime.h` vs `LocalInference.h`/`LocalInferenceTransport.h`) |
| Same gate limited to the hosted base (`67a80a0c..70e9fa07`) | clean — this is why the hosted "Check C++ formatting" step passes |
| Scoped `clang-tidy` | **Not reproducible**: host `clang-tidy` is LLVM 23; the build PCH was produced by AppleClang 21 and is rejected ("older PCH format"). CI-pinned clang-tidy 18 on Linux GCC cannot be exercised on this host. |
| QML lint (`qmllint`) | not run locally (Linux quality job covers it) |

**Observation:** the merged delta contains one real clang-format violation that the hosted
`HEAD^`-relative gate cannot see because the integrating pushes cancelled one another (see §4.3).

## 4. Hosted cross-platform CI (exact candidate `70e9fa07`)

### 4.1 Per-workflow / per-job status (observed 2026-10-16 ~20:05 UTC)

| Workflow (run) | Job | Status | Conclusion |
| --- | --- | --- | --- |
| Rust IPC clients ([38957943863](https://github.com/sentinel-projects/sentinel/actions/runs/38957943863)) | rust (ubuntu-latest) | completed | **success** |
| | rust (macos-14) | completed | **success** |
| | `ipc-stress` | completed | **skipped** (dispatch-only) |
| macOS CI ([38957943786](https://github.com/sentinel-projects/sentinel/actions/runs/38957943786)) | Build and test (macOS) | completed | **failure** (Test step) |
| Linux CI ([38957943894](https://github.com/sentinel-projects/sentinel/actions/runs/38957943894)) | Build and test | completed | **failure** (Test step) |
| | AddressSanitizer build and test | in_progress | — |
| | Format, lint, and static analysis | in_progress | — |
| Windows CI ([38957943785](https://github.com/sentinel-projects/sentinel/actions/runs/38957943785)) | Build and test (Windows x64) | in_progress | — |
| | Build (Windows ARM64, experimental) | completed | **skipped** (nonblocking) |
| CodeQL ([38957944070](https://github.com/sentinel-projects/sentinel/actions/runs/38957944070)) | CodeQL Analyze (C++) | in_progress | — |

**Platform distinction requested:** Linux x86_64 (**failed**), macOS arm64 (**failed**), Windows x64
(**in progress**), Rust IPC Unix matrix (**passed** for both cells; stress **never ran**),
packaging/release (**never ran** — `release.yml` is tag-triggered on `v*` and no tag exists).

`Reject personal paths in public files` passed on Linux, macOS and Windows — the SEC-01 current-tree
fix is effective in CI.

### 4.2 Confirmed hosted test failures (both 135/136)

- **macOS `Build and test`** — `99% tests passed, 1 tests failed out of 136`:
  `#106 test_real_tool_executor_tools` →
  `RealToolExecutorToolsTest::gitInspectionUsesFixedSandboxedArguments()`, actual
  `ToolExecutionStatus` **6 (`Blocked`)** vs expected `2 (Succeeded)` at
  `test_real_tool_executor_tools.cpp(343)`. The test only `QSKIP`s when
  `status == Blocked && summary.contains("Sandbox")`; on the hosted runner the `Blocked` result is
  returned with a summary that does not satisfy the guard (e.g. the executor's
  "Process sandbox plan is missing." path), so the assertion fires instead of skipping.
- **Linux `Build and test`** — `99% tests passed, 1 tests failed out of 136`:
  `#104 test_llm_agent_runtime` →
  `LlmAgentRuntimeTest::allExposedDescriptorsResolveWithoutDanglingMetadata()`, actual
  `decision.kind` **2 (`GiveUp`)** vs expected `0 (`ToolCall`)` at `test_llm_agent_runtime.cpp(702)`.
  Root cause: the test iterates **exposed+enabled** descriptors and never skips
  platform-unsupported ones, while `LlmAgentRuntime::availableTools()` (line 214) filters with
  `toolSupportedOnCurrentPlatform`. `app-launch` is `platforms.linux = Unsupported`
  (`BuiltInToolProvider.cpp:786`), so on Linux the synthetic `app-launch` call becomes an unknown/
  unsupported tool → `GiveUp`; on macOS `app-launch` is supported → `ToolCall` → pass.

Both are **environment/platform-sensitive test defects**, not caused by the PM merge content:
neither failing test file nor the tool-provider logic is in the `619e989c..70e9fa07` delta. They
surface now only because the SEC-01 privacy fix lets Linux/macOS CI proceed past the scan step
that previously failed the jobs at 619e989c.

### 4.3 CI-gating gap in the integrated chain

Intermediate integrated merges were pushed in rapid succession, so:

- `619e989c` — Linux/macOS/Windows all **failed** (privacy scan).
- `74ba91e2` — **no runs**.
- `a69a9f37` — 5 runs, all **cancelled** by `concurrency: cancel-in-progress`.
- `67a80a0c` — **no runs**.
- `70e9fa07` — the only surviving substantive run (in progress / macOS failed).

Consequently the merged P2/P1 deltas were never independently gated by a surviving run, and the
final quality job's format check only diffs `67a80a0c..70e9fa07` (two files). This is the reason the
full-delta formatting violation and the Linux/macOS test defects were not caught before merge.

## 5. P1-04 hosted IPC stress

- `ci-rust.yml` has the `workflow_dispatch`-only `ipc-stress` matrix (ubuntu-latest, macos-14), 100
  sequential `cargo +1.94.0 test --locked -p sentinel-ipc` iterations, no retry, per-iteration
  logs/JSON.
- GitHub `event=workflow_dispatch` query for that workflow returns **total_count = 0**. The ordinary
  push runs report `ipc-stress → skipped`. **There is no qualifying hosted 100-repeat evidence on
  either supported Unix cell.**
- **AC-P1-04 remains Ready-for-Validation / not Passed.** Local IPC suite is green (26 passed), but
  that is not hosted stress.

Dispatch procedure to be executed only after explicit authorization (not performed here):

1. Confirm the reviewed immutable candidate SHA and that `ci-rust.yml` exists on the default branch.
2. Actions → **Rust IPC clients** → **Run workflow**, select the exact ref/SHA.
3. Required evidence: both `ubuntu-latest` and `macos-14` cells each report `Completed: 100/100;
   failed: 0`; run URLs; `github_sha`/`checkout_sha`; runner + `rustc +1.94.0 -Vv`; every
   `iteration-NNN.log`/`.json` and `outcome.json`. Any timeout, failure or missing iteration keeps
   AC-P1-04 open (no retry-to-green).

## 6. Security and product acceptance

### 6.1 Current-tree privacy (SEC-01) — corrected

- Scanner on the audit baseline: **0 files flagged**; the whitelisted `/home/linuxbrew/...`
  system prefix is the only `/home/...` token in the tree.
- `docs/audits/P1_04_2_LINUX_CI_VALIDATION.md` SHA-256 `0ab8f740…` and
  `docs/audits/SEC_01_HISTORICAL_PRIVACY_REMEDIATION_RESULT.md` SHA-256 `3260c7e8…` match INT-02.
- Both SEC histories (`58355635` and the INT-02 duplicate) are ancestors of `main`.

### 6.2 Historical Git exposure — still open (not resolved by SEC-01)

The unredacted blob (`f00a8bcd…`) introduced by `d4704e39` remains reachable in `origin/main`
history (verified: the historical blob still contains exactly one `/home/<id>/` match; content not
printed). Redaction does not recall clones/forks/caches. A separate maintainer/security decision is
required; none is made here.

### 6.3 Acceptance criteria that must stay open

| Criterion | Matrix state | Why still open after this audit |
| --- | --- | --- |
| AC-P1-01 native tool assembly + live evidence | Unverified | `cor11 native-tool-assembly` row is an asserted skip; `test_agent_runtime` has 4 macOS no-fork skips; COR-06 capabilities are injected metadata, not live Git/browser/Whisper/model |
| AC-P2-01 provider API/capability | Unverified | Mock/scripted transports only; no real backend/live acceptance |
| AC-P2-03 real backend parity | Blocked | No real Ollama/llama/LM Studio/cloud acceptance |
| AC-P3-02 ownership/lifecycle | Blocked | D4/D5b XFAIL + D6 skip remain; no residency/lease/unload |
| AC-P5-04 shutdown/legacy authority | Blocked | Depends on PD-D05/PD-D10 ownership and shutdown budgets |
| PD-D10 provider ownership / hard-stop | Pending Decision | Not adopted by any commit subject |
| PD-D11 named humans / review capacity | Pending Decision | Single CODEOWNER `@Sopwit`; no independent reviewer assigned |

The decision register still lists **14 Pending Decisions** (PD-D01…PD-D14); the acceptance matrix and
decision register are unchanged across `619e989c..70e9fa07`. Merged source plus green local targets
does **not** mark any of the above passed.

## 7. Integration problems and production defects

1. **Hosted CI is red at the integrated candidate.** macOS fails
   `test_real_tool_executor_tools::gitInspectionUsesFixedSandboxedArguments` (Blocked sandbox;
   skip-guard mismatch); Linux fails
   `test_llm_agent_runtime::allExposedDescriptorsResolveWithoutDanglingMetadata` (platform-unsupported
   `app-launch` not skipped by the test). Both are real cross-platform gaps that local macOS passes.
2. **Formatting violation in merged source.** `core/src/runtime/OllamaRuntime.cpp` fails the
   repository's clang-format 18.1.8 gate over the integrated delta; the hosted gate misses it because
   the merge pushes cancelled one another.
3. **Negative CI evidence for the integrated chain.** Only the final push has a surviving run;
   intermediate merges (`74ba91e2`, `a69a9f37`, `67a80a0c`) had no completed CI, so the merged
   provider/agent/lifecycle content was not independently gated.
4. **No hosted P1-04 stress evidence** (0 `workflow_dispatch` runs).
5. **Uncommitted INT-03 reconciliation** exists in another worktree and is not in `main` (context,
   not a defect).

No memory-safety/ownership regression was observed locally (ASan is still running hosted). The
current-tree secret/personal-path scanner is clean but is only a bounded home-path pattern.

## 8. Recommended next integration task

1. **Fix the two hosted test defects on a new `main`-relative branch** (do not rewrite `main`):
   make `allExposedDescriptorsResolveWithoutDanglingMetadata` skip/adjust platform-unsupported
   descriptors (or assert their `GiveUp`) so Linux is deterministic; make
   `gitInspectionUsesFixedSandboxedArguments` skip on the hosted `Blocked` status regardless of the
   summary casing, or return the sandbox availability deterministically.
2. **Apply the single clang-format 18.1.8 correction** to `OllamaRuntime.cpp` include order and add a
   full-delta format check for integration candidates.
3. **Re-run the full candidate-specific CI on the fixed SHA** and require Linux, macOS, Windows,
   Rust and CodeQL all green before treating `main` as an integration baseline; avoid rapid
   cancelling pushes on integration days.
4. **Separately authorize hosted P1-04 IPC stress** on the reviewed SHA (procedure in §5); keep
   AC-P1-04 open until both Unix cells pass.
5. **Proceed with reviewer/ownership decisions** (PD-D10, PD-D11) and live/native evidence planning;
   none of AC-P1-01/AC-P2-01/AC-P2-03/AC-P3-02/AC-P5-04 can close on merged source plus local green
   totals. If a new code PR is wanted, the only identified unmerged scope is the nine-file
   `fix/p1-05-legacy-cli-exits` change, which needs fresh current-main validation.

## 9. Readiness and scope limits

- **Identity:** verified at `70e9fa07…`, all four sources agreeing.
- **Local:** reproduced 136/136 CTest, 141 Rust tests green, IPC contracts and privacy gate green.
- **Hosted:** Linux and macOS **failed** at this SHA; Windows/CodeQL/ASan/quality still running at
  observation; release/packaging never ran; hosted stress never ran.
- **Not done / not claimed:** GUI/native Fedora KDE, live-model, cloud or Windows runtime acceptance;
  ASan result on Linux; history rewrite or exposure recall; acceptance-criterion closure.

This report is an uncommitted audit artifact written in the isolated audit worktree. No commit, push,
merge, reset, stash, PR or workflow dispatch was performed.
