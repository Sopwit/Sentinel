# P1-05 — Legacy C++ CLI failure and timeout exit semantics (result)

**Task:** Correct legacy C++ CLI failure and timeout exit contracts (backlog P1-05, AC-P1-07, PD-D05).
**Worktree/branch:** `/home/sopwith/Desktop/sentinel-p1-05` on `fix/p1-05-legacy-cli-exits`, based on `e71cca4e`
("docs(audit): add P1-04.3 GitHub CI readiness report"). No commits, pushes, or PRs were created;
all changes are uncommitted in the worktree.

**Scope honored:** only legacy `apps/sentinel-cli` command handlers, its CMake target, the new test,
test registration, `docs/reference/CLI_REFERENCE.md`, and this document changed. No AgentRuntime/AgentLoop
changes, no Rust IPC or Rust TUI changes, no GitHub CI changes, no modifications to P1-01/P1-04 documents,
no new Agent entry path, no real model downloads or cloud calls (all provider traffic goes to loopback
fixtures), and no changes to the CLI's retention policy (PD-D05 remains a pending maintainer decision).

---

## 1. Defects confirmed with baseline evidence

Baseline = unmodified binary at `e71cca4e`, snapshotted before any edits. Evidence produced by an external
harness (`/tmp/opencode/p1-05-exit-harness.py`, 18 scenarios, loopback HTTP fixture for
`/api/version`, `/api/tags`, `/api/generate`, `/api/pull`); raw records in
`/tmp/opencode/p1-05-evidence-baseline.jsonl` and `/tmp/opencode/p1-05-evidence-after.jsonl`.

| # | Defect | Baseline evidence |
| --- | --- | --- |
| D1 | Chat response timeout returns **0**. `ChatCommand` waited the 30 s deadline, printed `[Timeout waiting for response]`, then fell through to `return 0`. | S17: exit **0** after 29,929 ms with the timeout marker on stdout. |
| D2 | Chat provider/request failure returns **0**. The handler printed the failure message from `lastAgentResponse()` but never inspected `chatSendLifecycleState()`/`chatErrorCategory()`. | S13 (no configured model), S14 (provider unavailable), S15, S16: exit **0** with failure text. |
| D3 | Model pull failure returns **0**. `pullFinished(model, false)` printed `Pull failed: …` but the handler returned 0. | S10 (connection refused), S11 (HTTP 500): exit **0**. |
| D4 | Blank model pull name waits indefinitely. `model pull "   "` reached `OllamaModelPuller::pull()`, which trims to an empty id and returns without emitting `pullFinished`; the command then waited in `QEventLoop::exec()` forever. | S5: no exit; harness killed the process at 15 s (recorded exit −9). |
| D5 | Stalled pull waits indefinitely. No inactivity/deadline existed in the pull loop. | S12: no exit; killed at 15 s (would hang forever; production default is unbounded). |
| D6 | Chat success was unreachable in a fresh process. The chat gate evaluates `OllamaChatProvider::status()` against the model-discovery snapshot, which a fresh process holds as `Pending` (discovery is asynchronous), so the provider was always `Unavailable` and the request failed before ever reaching the endpoint. | S15 against a healthy fixture: exit 0 but with "provider is not ready", `generate=0` — the fixture chat endpoint was never contacted. |
| D7 | Model list failure returned **1** (usage/argument class) although it is an operation failure. Not a zero-exit defect, but inconsistent with any failure taxonomy. | S8: exit **1** with `Model discovery failed: …` on stderr. |

Signals: baseline SIGINT died by signal (S18, exit −2); this behavior is preserved, not changed.

## 2. Root causes

- `ChatCommand` and `ModelCommand` used literal `return 0` after printing failure conditions; exit code
  was independent of the observed outcome.
- Outcome state already existed in core (`ApplicationController::chatSendLifecycleState()`,
  `chatErrorCategory()`, `chatSendLifecycleSummary()`, `OllamaModelPuller::pullFinished(success)`) but was
  never read by the CLI.
- The pull event loop had no deadline; the chat deadline existed but its result was discarded (marker on
  stdout, then `return 0`).
- The chat flow never waited for the initial model-discovery round to settle, so both the success path and
  a meaningful response deadline were unreachable in a fresh process.

## 3. Changes

| File | Change |
| --- | --- |
| `apps/sentinel-cli/commands/ExitCodes.h` (new) | Single definition of `kExitSuccess/kExitUsage/kExitFailure/kExitTimeout` with the contract comment. |
| `apps/sentinel-cli/commands/ChatCommand.cpp` | Bounded discovery wait before sending (resolves D6); exit classification from `chatSendLifecycleState()==completed && chatErrorCategory()==None`; refused requests exit 2 immediately; response deadline exits 3 with stdout marker preserved and a stderr diagnostic; deadline decision is terminal — a late async reply is ignored, and `stopChatGeneration()` is invoked on timeout so teardown joins an aborted worker instead of a stuck one; blank prompt → usage 1; env deadline seam. |
| `apps/sentinel-cli/commands/ModelCommand.cpp` | Pull: trimmed/blank model id → usage 1 (D4); `pullFinished(success)` decides 0/2 (D3); inactivity deadline derived from the existing progress timer → cancel + exit 3 (D5); stderr diagnostics `sentinel-cli: model pull failed/stalled`. List: discovery failure → 2 (D7), 5 s discovery deadline → 3 (was: unbounded wait). |
| `apps/sentinel-cli/main.cpp` | Unknown command returns `kExitUsage` (same value 1, now contract-named). |
| `apps/sentinel-cli/CMakeLists.txt` | Registers `ExitCodes.h`. |
| `tests/integration/test_legacy_cli_exit_codes.cpp` (new) | Deterministic QtTest suite, 18 test slots (20 including init/cleanup), running the real binary as a subprocess against a loopback `QTcpServer` Ollama fixture with per-test isolated `HOME`/XDG profiles. |
| `tests/CMakeLists.txt` | Registers the suite with `SENTINEL_CLI_BINARY="$<TARGET_FILE:sentinel-cli>"`, include path to `commands/ExitCodes.h`, dependency on the CLI target, 300 s test timeout. |
| `docs/reference/CLI_REFERENCE.md` | Exit-code contract, environment deadline seams, and an explicit note that this table belongs only to the retained legacy `sentinel-cli` (separate from the canonical daemon-backed `sentinel` table in `docs/user/CLI.md`); the retention disposition stays open under PD-D05. |

No production code outside `apps/sentinel-cli` was modified.

## 4. Exit-code contract (also documented in `docs/reference/CLI_REFERENCE.md`)

| Code | Meaning |
| --- | --- |
| 0 | Success, or usage printed because no command was supplied (help). |
| 1 | Usage/argument error: unknown command/subcommand, unknown configuration key, missing/blank required argument (unchanged class; blank chat prompt and blank model id are now included). |
| 2 | Operation failure: provider failure, refused request, model discovery failure, model pull failure. |
| 3 | Deadline timeout: chat response deadline, model discovery deadline, or model pull inactivity deadline. |

Rules:

- Failures never exit 0; help/success remain 0.
- Exactly one terminal decision: the outcome is classified once from the state that settled the wait;
  asynchronous callbacks arriving after a deadline decision cannot convert the run
  (guarded in both chat and pull paths).
- All waits are bounded (chat response deadline, model discovery deadlines, pull inactivity deadline).
- Signals are not caught: an interrupted run dies by signal (shell convention 130 for SIGINT on Unix);
  no signal path was converted into an exit code.
- No new Agent entry path: everything stays in the existing command handlers; `AgentRuntime`/`AgentLoop`
  are untouched.

### Test seams (environment, documented)

| Variable | Default | Use |
| --- | --- | --- |
| `SENTINEL_CLI_CHAT_TIMEOUT_MS` | 30000 | Chat response deadline (min 100 ms). |
| `SENTINEL_CLI_PULL_IDLE_TIMEOUT_MS` | 60000 | Pull inactivity deadline (min 100 ms); measures time without progress/status movement, so a healthy streaming download is not cut off. |

The test suite removes ambient values of both variables in its environment so production defaults apply
unless a case sets them explicitly.

## 5. Before/after evidence (18 scenarios, identical harness)

| Scenario | Baseline exit | Fixed exit | Notes |
| --- | --- | --- | --- |
| S1 usage, no args | 0 | 0 | Help unchanged. |
| S2 unknown command | 1 | 1 | Unchanged. |
| S3 chat, missing prompt | 1 | 1 | Unchanged. |
| S4 config, unknown key | 1 | 1 | Unchanged. |
| S5 pull, blank name | **hang (killed @15 s)** | **1** | Usage error, 364 ms. |
| S6a status, no config | 0 | 0 | Unchanged. |
| S6b config get, no config | 0 | 0 | Unchanged. |
| S7 model list, success | 0 | 0 | Unchanged. |
| S8 model list, refused | **1** | **2** | Deliberate reclassification (operation failure, not usage). |
| S9 model pull, success | 0 | 0 | Unchanged. |
| S10 model pull, refused | **0** | **2** | D3 fixed. |
| S11 model pull, HTTP 500 | **0** | **2** | D3 fixed. |
| S12 model pull, stalled | **hang (killed @15 s)** | **3** | D5 fixed; exits 2.2 s with `model pull stalled` on stderr. |
| S13 chat, no configuration | **0** | **2** | D2 fixed; `chat failed (state 'failed', …)` on stderr. |
| S14 chat, provider failure | **0** | **2** | D2 fixed. |
| S15 chat, success | 0 (but failure text, `generate=0`) | **0** | D6 fixed: `Sentinel: Hello from the fixture.` with `generate=1`. |
| S16 chat, stalled response | **0** (`generate=0`, gate failure) | **3** | D1/D6 fixed: deadline hit at 1.8 s, marker + `chat timed out` on stderr, `generate=1`. |
| S17 chat, unusable conversation store | **0 @29,929 ms with timeout marker** | **2** | D1 fixed: refusal settles in 826 ms with `chat request refused` on stderr, no timeout marker. |
| S18 interrupted pull (SIGINT) | −2 (signal) | −2 (signal) | Signal-death convention preserved. |

Environment noise: the fixture runs print unrelated `VDPAU`/`MESA-INTEL` loader warnings from the host
Qt/graphics stack on stderr; they are environment-caused, occur in baseline and fixed runs alike, and do
not affect exit codes or assertions.

## 6. Validation

- **Build:** `cmake --preset tests` configure + `cmake --build --preset tests` — success.
- **CLI exit-contract suite:** `ctest --test-dir build/tests -R test_legacy_cli_exit_codes
  --output-on-failure` — **passed**; binary run directly: **20 passed, 0 failed, 0 skipped (11.0 s)**.
  Covers: success (usage/help, status/config, list, pull, chat), chat timeout → 3, chat provider failure
  → 2, chat refusal → 2, model pull failure → 2 (refused and HTTP 500), pull stall → 3, discovery
  deadline → 3, invalid arguments → 1 (unknown command/key, missing/blank prompt, blank model id),
  missing configuration → 0/2 as specified, interrupted operation → signal death. No real pulls or cloud
  calls: everything runs against the loopback fixture in-process.
- **Full suite:** `ctest --preset tests --output-on-failure` — see §8 for the result.
- **Formatting:** `clang-format --dry-run --Werror` on all five changed/new C++ files — clean.
- **Static analysis / other linters:** none configured for these targets beyond clang-format in this
  workflow; no new warnings appeared in the build log.

Test-design note: the suite waits for the subprocess through the Qt event loop
(`QTest::qWaitFor`) rather than `QProcess::waitForFinished()`, because a blocking wait starves the
in-process fixture's event loop and would make every fixture interaction time out for reasons unrelated
to the CLI.

## 7. Compatibility risks

- **Model list failure exit 1 → 2 (S8).** Scripts keying on exactly 1 for "discovery failed" would see
  the new value. Documented in the CLI reference; 1 is now reserved for usage/argument errors.
- **New exit code 3** for deadline timeouts (chat response, discovery, pull inactivity). Previously these
  paths either exited 0 or hung. Scripts treating any nonzero as failure are unaffected.
- **New stderr diagnostics** (`sentinel-cli: chat failed…`, `chat request refused`, `chat timed out`,
  `model pull failed/stalled`) and slightly different chat output ordering (discovery wait precedes
  sending). The stdout `[Timeout waiting for response]` marker is preserved for existing parsers.
- **Blank arguments now fail fast**: `chat "   "` and `model pull "   "` exit 1 instead of hanging or
  proceeding.
- **Two new environment variables.** They only change deadlines; unset production behavior keeps the
  documented 30 s / 60 s defaults. Documented as the only env inputs of this CLI.
- The legacy CLI's separate authority disposition (retain vs retire) is unchanged; this task implements
  PD-D05's "correct failure exits" step only and does not close PD-D05.

## 8. Validation summary

- Build (tests preset), before and after the final formatting pass: **pass**.
- `test_legacy_cli_exit_codes`: **20/20 pass** on the final build (11.2 s).
- Full `ctest --preset tests --output-on-failure`: **127/128 passed, 1 failed**.
  The single failure is `test_llm_agent_runtime::allExposedDescriptorsResolveWithoutDanglingMetadata`
  (assertion at `tests/core/test_llm_agent_runtime.cpp:702`, planner decision `GiveUp` instead of
  `ToolCall`). It is **pre-existing at the branch base and independent of this change**, verified as
  follows: (a) `ninja -t commands test_llm_agent_runtime` contains no input from this diff (no
  `apps/sentinel-cli/*`, no `ExitCodes.h`, no new test files; the target links only core/Qt), (b) no
  `core/` file was modified, (c) it reproduces deterministically in an isolated rerun. It is tracked in
  §10 as a remaining issue for a separate agent-runtime task, not an environment-dependent failure and
  not caused by P1-05.
- `clang-format --dry-run --Werror` on all five changed/new C++ files: **clean**.
- Before/after harness on the final binary: 18/18 scenarios match the contract (§5).

## 9. AC-P1-07 status

AC-P1-07 row: *"Timeout/provider/pull failure must be nonzero; supported success remains zero"* —
baseline gap *"ChatCommand timeout and pull failure can return 0"*, method *"injected/subprocess fixture,
no real pulls"*, environment *"ENV-HOST + ENV-HEADLESS"*, criteria *"documented nonzero failure/timeout
in sync/async paths, one terminal, bounded wait; help/success zero; no new Agent entry path"*, evidence
*"EV-01 plus case outcomes, independent verification and all attempts"*, dependency PD-D05.

| Criterion | Status |
| --- | --- |
| Timeout/provider/pull failure nonzero, success/help zero | **Met** — contract §4; evidence §5 (S10–S17) and 20/20 suite. |
| Baseline gap (chat timeout and pull failure return 0) closed | **Met** — D1/D3 fixed; baseline vs fixed in §5. |
| Documented nonzero failure/timeout | **Met** — `docs/reference/CLI_REFERENCE.md` exit table. |
| Sync and async paths | **Met** — synchronous settlement (`responded` before the loop) and async loop settlement both classified; refusal path returns immediately. |
| One terminal decision | **Met** — single classification point; late callbacks explicitly ignored after a deadline decision (chat and pull). |
| Bounded wait | **Met** — chat response, discovery (chat and list) and pull inactivity deadlines; no indefinite wait remains (S5/S12). |
| No new Agent entry path | **Met** — changes confined to `apps/sentinel-cli`; AgentRuntime/AgentLoop untouched. |
| Injected/subprocess fixture, no real pulls | **Met** — subprocess CLI against loopback `QTcpServer` fixture; harness uses a Python loopback fixture; no downloads or cloud calls. |
| ENV-HOST + ENV-HEADLESS evidence | **Met on ENV-HOST** (this Fedora worktree, `ctest` + harness). The suite is headless-safe (`QTEST_GUILESS_MAIN`, offscreen platform env, no GUI dependency) and runs unchanged under `ctest` in headless CI once P1-04 adopts CI execution; this task does not claim CI-run evidence. |
| Independent verification | **Partially met** — the external harness re-executes the built binary outside the test code and agrees with the suite on all 18 scenarios; a verifier independent of this change (separate owner/session) is still required by the AC evidence rule. |
| PD-D05 dependency | **Open** — PD-D05 remains "Pending Decision" (maintainer + CLI owner). Per its own text, "failure exits must still be fixed" — done here; disposition/closure of PD-D05 is not part of this task. |

**Overall:** implementation and host-side evidence for AC-P1-07 are complete in this worktree; the matrix
row stays with the acceptance/verification owner (this task does not edit `ACCEPTANCE_MATRIX.md` or mark
phases complete). Remaining to reach Verified: independent verification by a separate owner and an
ENV-HEADLESS/CI run under the adopted CI (P1-04), plus the PD-D05 maintainer decision.

## 10. Remaining issues / out of scope

- No commits/push/PR were created (per task instructions); the diff lives only in the worktree.
- **Pre-existing test failure at the branch base (not introduced by this task):**
  `test_llm_agent_runtime::allExposedDescriptorsResolveWithoutDanglingMetadata` fails deterministically
  (`GiveUp` instead of `ToolCall` for at least one exposed built-in descriptor; last commits touching
  that area: `79667c4e` "feat(agent): ground coding workflows and observation intent"). Agent-runtime
  changes are outside P1-05 scope; recommend a separate agent-runtime task to identify the descriptor
  that no longer resolves and fix planner/registry behavior. See §8 for the independence proof.
- P1-01 (deterministic COR fixtures) and P1-04 (CI) were not modified or required.
- The Rust CLI (`docs/user/CLI.md` exit table) and `docs/interfaces/CLI.md` are untouched; the legacy
  table is explicitly scoped in the CLI reference.
- Host Qt loader warnings (`VDPAU`/`MESA-INTEL`) on stderr are environmental and unrelated to exit
  semantics.
- Provider-level retry/timeout behavior inside core (e.g., discovery internals) was not changed; the CLI
  only bounds the waits it performs.
