# P1-05 — Current-main Legacy C++ CLI exit semantics integration validation

Audit date: 2026-10-16 (Europe/Istanbul). Isolated review worktree only; no commit, push, merge,
rebase, cherry-pick, reset, stash or PR was performed. No existing worktree, the remote source
branch or `main` was modified. No product code outside `apps/sentinel-cli` was changed.

## 1. Identities

| Item | Value |
| --- | --- |
| Review branch | `review/p1-05-current-main` |
| Review worktree | managed worktree `p1-05-current-main/Sentinel-sanitized` |
| Review HEAD | `70e9fa072af9b4139d86f518c6d89f9c849c29b2` (current `main`; no candidate commit created) |
| Current `main` (local / stored `origin/main` / live `ls-remote`) | `70e9fa072af9b4139d86f518c6d89f9c849c29b2` (all three agree) |
| Original P1-05 source branch | `origin/fix/p1-05-legacy-cli-exits` |
| Original P1-05 source commit | `43c7bef0fad84d6cd782080132b5eaabbb2dd55d` — `fix(cli): enforce nonzero failure and timeout exits in legacy sentinel-cli` |
| P1-05 merge base with `main` | `e71cca4e958109e0825b6277072bd1235e77e60a` (is an ancestor of `main`) |
| Source-branch ancestry | `43c7bef0` is **not** contained in `main` (one branch-only commit) |
| Related criterion | AC-P1-07 (row status in the acceptance matrix remains **Blocked**) |

Context documents read: `AGENTS.md`; [INT-01](../audits/INT_01_MULTI_WORKTREE_INTEGRATION_READINESS.md),
[INT-02](../audits/INT_02_SEC_01_INTEGRATION_REVIEW.md), INT-03 (see note), P0/P2/P3 audit results,
architecture / Agent Runtime / security / building / testing, and the
[P1-05 result report](P1_05_LEGACY_CLI_EXIT_SEMANTICS_RESULT.md).

INT-03 note: `docs/audits/INT_03_SOURCE_HISTORY_RECONCILIATION.md` exists only as an untracked file in
the separate `docs/int-03-source-reconciliation` worktree; it is not committed on `main`. Its §7
already scopes P1-05 as the single unmerged nine-file code candidate that needs a fresh
main-relative review, which this validation performs. That file was read without modification.

## 2. Exact main-relative change set (nine files)

The P1-05 commit touches exactly nine files. Compared against `main`, seven are unchanged since the
merge base and two diverged on `main` (`docs/reference/CLI_REFERENCE.md`, `tests/CMakeLists.txt`).
The candidate reconstructs all nine main-relative:

| # | File | Main-relative disposition | Candidate SHA-256 |
| --- | --- | --- | --- |
| 1 | `apps/sentinel-cli/CMakeLists.txt` | Applied as-is (unchanged since base) | `4871111271e27a900fce109ab0ac2103a3cd94cebffc9b3b36a186f46c9245bf` |
| 2 | `apps/sentinel-cli/commands/ChatCommand.cpp` | Applied as-is | `e6eb40188a00b44caf3745eb7560995bfb30382ef47674fdfc7678ba680880c5` |
| 3 | `apps/sentinel-cli/commands/ExitCodes.h` (new) | Applied as-is | `0c0f8dc1273138f6f4cf238f79c7f6784c4132dc411db62641b3c25964e867e3` |
| 4 | `apps/sentinel-cli/commands/ModelCommand.cpp` | Applied as-is | `1e49e64c0e42d67e047244add1306d8e0fde83f2a4e0cdb4862af8745a7d0afc` |
| 5 | `apps/sentinel-cli/main.cpp` | Applied as-is | `13f6e42160b5e883eb025353b1684ad0597a24f293d55648f2436cbae1250427` |
| 6 | `docs/audits/P1_05_LEGACY_CLI_EXIT_SEMANTICS_RESULT.md` (new) | Applied with a privacy redaction (§4) | `112cc7aea9589b22a8d9749ecc63d7aaebc9a4ac805a37898634b3239e04793a` |
| 7 | `docs/reference/CLI_REFERENCE.md` | Reconciled with `main` (§3) | `873fdbaaa30b370f69b52270d6a2c443c1278decf6f711f14b40983961cd5f9f` |
| 8 | `tests/CMakeLists.txt` | Reconciled with `main` (§3) | `46e8772b5df979d238d3ae1cd8c9bf69b5ccbd75802d339ec386e55b703edb06` |
| 9 | `tests/integration/test_legacy_cli_exit_codes.cpp` (new) | Applied byte-identical to source | `cab36aa414c461c52e523b6f22031025f712b86129d055aed2228f4fb4ba4b0c` |

Candidate diff vs `main` (tracked files):

```text
 apps/sentinel-cli/CMakeLists.txt            |   1 +
 apps/sentinel-cli/commands/ChatCommand.cpp  | 125 +++++++++++++++++++++++---
 apps/sentinel-cli/commands/ModelCommand.cpp | 130 +++++++++++++++++++++++-----
 apps/sentinel-cli/main.cpp                  |   3 +-
 docs/reference/CLI_REFERENCE.md             |  30 ++++++-
 tests/CMakeLists.txt                        |   7 ++
 6 files changed, 256 insertions(+), 40 deletions(-)
```

plus the three new files (`ExitCodes.h`, the P1-05 report, the new test). Only controlled patches were
applied (`git apply` of the source commit's own diff for the seven clean files; manual reconciliation
for the two main-diverged files). No cherry-pick, rebase or commit was used, and no Rust, Agent,
provider, daemon, workflow or other CMake content was overwritten.

## 3. CMake and docs conflict resolution

### 3.1 `tests/CMakeLists.txt`

`main` added newer registrations after the merge base (provider/bounded/agent-reliability/P3 targets,
and the agent-reliability IPC fixture wiring). The P1-05 hunk was inserted verbatim at its stable
anchor without touching them:

```cmake
 add_sentinel_core_test(test_rag_e2e_pipeline integration/test_rag_e2e_pipeline.cpp)
+add_sentinel_core_test(test_legacy_cli_exit_codes integration/test_legacy_cli_exit_codes.cpp)
+target_compile_definitions(test_legacy_cli_exit_codes PRIVATE
+    SENTINEL_CLI_BINARY="$<TARGET_FILE:sentinel-cli>")
+target_include_directories(test_legacy_cli_exit_codes PRIVATE
+    ${PROJECT_SOURCE_DIR}/apps/sentinel-cli)
+add_dependencies(test_legacy_cli_exit_codes sentinel-cli)
+set_tests_properties(test_legacy_cli_exit_codes PROPERTIES TIMEOUT 300)
 add_sentinel_core_test(test_startup_time performance/test_startup_time.cpp)
```

All newer registrations are preserved exactly once; no registration was overwritten or duplicated.
Configured test count on current main: **137**.

### 3.2 `docs/reference/CLI_REFERENCE.md`

`main` had retitled the page **"Legacy C++ CLI reference"** and added a scope sentence pointing at the
canonical daemon-backed Rust CLI. The candidate keeps that newer title/scope and layers the P1-05
exit-code contract on top, replacing the now-obsolete pre-fix "Automation limitation" paragraph (which
documented the very defects P1-05 fixes — chat timeout returning 0, pull failure returning 0). The
reconciled document:

- keeps the "Legacy C++ CLI reference" title and the Rust-CLI cross-reference to
  [`../user/CLI.md`](../user/CLI.md) (verified to exist);
- documents the legacy contract `0/1/2/3`, the bounded-wait and single-terminal-decision rules, and the
  two deadline seams (`SENTINEL_CLI_CHAT_TIMEOUT_MS`, `SENTINEL_CLI_PULL_IDLE_TIMEOUT_MS`);
- explicitly states the table is legacy-only and does not replace the Rust exit table.

Delta against the source-branch bytes (the reconciliation):

```diff
-# CLI reference
+# Legacy C++ CLI reference

+This reference covers the in-process compatibility executable, not the daemon-backed Rust `sentinel` CLI/TUI. See the [Rust CLI guide](../user/CLI.md).
+
 Syntax: `sentinel-cli <command> [args...]`. ...
```

The **canonical Rust daemon-backed CLI** and the **legacy C++ in-process CLI** remain distinct: only
`apps/sentinel-cli` and its legacy docs changed; `cli/` (Rust), `docs/user/CLI.md` and
`docs/interfaces/CLI.md` were not touched. Rust exit codes were neither replaced by legacy codes nor
vice versa.

## 4. Repository privacy conflict (main-relative)

The source-born P1-05 report contained one personal workstation path
(`/home/<user>/Desktop/sentinel-p1-05`). The repository privacy policy established by SEC-01 rejects
concrete home-directory paths, and `python3 tools/analysis/check_personal_paths.py` flagged the report:

```text
Personal directory path found: docs/audits/P1_05_LEGACY_CLI_EXIT_SEMANTICS_RESULT.md
```

The candidate redacts that single token to `<REDACTED_LOCAL_PATH>` (the SEC-01 placeholder), preserving
all technical content, dates and findings:

```diff
-**Worktree/branch:** `/home/<user>/Desktop/sentinel-p1-05` on `fix/p1-05-legacy-cli-exits`, based on `e71cca4e`
+**Worktree/branch:** `<REDACTED_LOCAL_PATH>` on `fix/p1-05-legacy-cli-exits`, based on `e71cca4e`
```

After redaction the privacy scan reports **0 file(s) flagged**. This is a required main-relative
privacy adjustment, not a historical-findings rewrite.

## 5. Exit-code contract evidence

Documented contract (candidate `docs/reference/CLI_REFERENCE.md` and `ExitCodes.h`): `0` success/help,
`1` usage/argument, `2` operation failure, `3` deadline timeout; failures never exit `0`; one terminal
decision per run; bounded waits; no signal trapping; no new Agent entry path.

### 5.1 Independent contract harness (ENV-HOST, macOS review host)

The real built binary (`build/tests/apps/sentinel-cli/sentinel-cli`) was run as a subprocess against a
loopback HTTP Ollama fixture, with per-run isolation via an isolated profile root (macOS honours
`CFFIXED_USER_HOME` as the `QStandardPaths` home base). 18 of 18 suite contract behaviours were
reproduced, plus two extra probes:

| Behaviour | Expected | Observed |
| --- | --- | --- |
| help / no args | 0 | 0 |
| unknown command | 1 | 1 |
| chat missing prompt / blank prompt | 1 | 1 |
| `config set` unknown key | 1 | 1 |
| `model pull` blank name | 1 | 1 |
| `status` / `config get` with no config | 0 | 0 |
| `model list` success (fixture) | 0 | 0 (`tags` contacted) |
| `model list` endpoint refused | 2 | 2 |
| `model pull` success | 0 | 0 (`pull` contacted, "Pull completed") |
| `model pull` endpoint refused | 2 | 2 |
| `model pull` HTTP 500 | 2 | 2 |
| `model pull` stalled endpoint | 3 | 3 (`model pull stalled`, `pull` contacted) |
| `chat` success (fixture) | 0 | 0 (`generate` contacted, "Hello from the fixture.") |
| `chat` default provider with no configured model | 2 | 2 (`chat failed`) |
| `chat` configured provider, endpoint refused | 2 | 2 (`chat failed`) |
| `chat` stalled generate | 3 | 3 (`[Timeout waiting for response]`, `chat timed out`, `generate` contacted) |
| `chat` refused conversation store | 2 | 2 (`chat request refused`) |
| interrupted pull (SIGINT) | signal death | SIGINT signal death (return code `-2`), never 0 |

Additional probes (informational, **not** part of the 20-case suite):

- A **stalled Ollama discovery** (`/api/tags` never answers) is reported by core after ~750 ms as a
  discovery failure, so the legacy CLI exits **2** (operation failure), not the documented discovery
  deadline `3`. The result is still nonzero, so AC-P1-07 ("failure must be nonzero") holds; the core
  discovery timeout itself predates `main` (present at `e71cca4e`) and the suite does not assert a
  list-discovery timeout. This is a documentation/implementation nuance for the CLI owner, not a
  regression.

Structural checks:

- **No daemon-backed fallback.** `sentinel-cli` links only `sentinel_core`, `Qt6::Core`, `Qt6::Network`;
  its linked/undefined symbols reference no `AgentRuntime`/`AgentLoop`/`ToolExecutionGateway`/daemon-IPC
  symbols, and `apps/sentinel-cli/` includes no daemon/Agent/IPC header. The fixture (not the daemon)
  served every request.
- **No invented tool execution.** The chat path calls `ApplicationController::sendMessage` directly; the
  success case returned the provider fixture's literal text with exactly one `/api/generate` request and
  no tool/Agent activity. There is no new Agent entry path.

### 5.2 In-tree 20-case suite

`tests/integration/test_legacy_cli_exit_codes.cpp` is applied byte-identical to the reviewed source
commit. Run on this host via CTest it **fails in `initTestCase`**:

```text
FAIL!  : LegacyCliExitCodesTest::initTestCase() 'settingsFile.startsWith(configRoot)' returned FALSE.
        (settings file is not rooted at XDG_CONFIG_HOME; test isolation would be invalid)
```

Root cause: the suite isolates each subprocess with `HOME` + `XDG_CONFIG_HOME`, which
`QStandardPaths` honours on Linux but **not on macOS** (macOS resolves `AppConfigLocation` from the
platform home base via `CFFIXED_USER_HOME`, not `XDG_CONFIG_HOME`). This is a host-portability
limitation of the test's isolation, **not** a main-relative or product regression: the test file is new,
`StandardPathProvider` and the CLI path semantics were not changed by P1-05 or by `main` between the
merge base and HEAD, and the identical assertion would fail on the source branch under macOS.

**Verification-only demonstration.** A temporarily patched build of the same suite (adding
`CFFIXED_USER_HOME` to the isolated environment and deriving the settings suffix from the probe root
instead of `XDG_CONFIG_HOME`, with **no assertion weakened and no case added, removed or skipped**)
passed:

```text
Totals: 20 passed, 0 failed, 0 skipped, 0 blacklisted, 10133ms
```

The patch was then reverted; the candidate keeps the source bytes. The exact recommended seam is
listed in §8.

## 6. Regression results — original versus current

| Run | Result on original P1-05 branch base (`e71cca4e`) | Result on current `main` (`70e9fa07`) |
| --- | --- | --- |
| Legacy CLI suite | 20/20 (source report, Fedora ENV-HOST) | 20/20 with the portable isolation seam (verification-only); 0 executed on the unmodified bytes under macOS (bootstrap) |
| `test_llm_agent_runtime::allExposedDescriptorsResolveWithoutDanglingMetadata` | **FAILED** (`GiveUp` instead of `ToolCall`, `test_llm_agent_runtime.cpp:702`) | **PASS** — deterministic 3/3 (also PASS in the full sequential run) |
| Full suite | 127/128 (1 failure = the above) | 136/137 (1 failure = legacy bootstrap on macOS; see §7); 12/12 focused Agent/LLM/provider tests pass |

The previously reported `test_llm_agent_runtime` failure **does not reproduce on the current baseline**
and is independently resolved: it passes on repeated isolated invocations and in the complete
sequential run. Historical totals (128) differ from current totals (137) because `main` integrated new
provider, agent-reliability, lifecycle and security test targets; totals are not compared as if the
build configurations were identical.

## 7. Verification runs (actual counts)

Build environment: macOS Darwin 27.0.0 / arm64, AppleClang 21.0.0, Qt 6.12.0, CMake 4.4.4, Ninja 1.13.2,
ccache. No concurrent heavy build from the INT-04 or P4-01 worktrees was detected when the build
started; build and test execution were serialized.

1. `cmake --preset tests` — configure success.
2. `cmake --build --preset tests` — full build success (1517 build edges; `sentinel-cli` and
   `test_legacy_cli_exit_codes` linked).
3. `ctest --test-dir build/tests -N` — **137** tests registered.
4. Focused legacy: `ctest -R test_legacy_cli_exit_codes` — **failed (0/20 executed)** on unmodified
   bytes (macOS bootstrap, §5.2); **20/20** with the verification-only seam.
5. Focused Agent/LLM/provider (12 targets incl. `test_llm_agent_runtime`,
   `test_agent_reliability_harness`, `test_agent_reliability_ipc`, `test_agent_loop`,
   `test_agent_runtime`, `test_observation_policy`, `test_real_tool_executor_tools`,
   `test_provider_contracts`, `test_bounded_streaming_contract`, `test_local_inference`) — **12/12
   passed**.
6. Complete sequential CTest: `QT_QPA_PLATFORM=offscreen ctest --preset tests --output-on-failure` —
   **136/137 passed, 1 failed** (185.92 s). The single failure is `test_legacy_cli_exit_codes`
   (macOS bootstrap). No failure was hidden by skips, retries or weakened assertions.
7. Rust CLI compatibility: **not affected** (no `cli/**` or `.rs` file changed; the Rust exit table is
   untouched and explicitly preserved in the docs). `cargo fmt --all --check` in `cli/` is clean on the
   unchanged tree; full Rust build/test was not required because compatibility is unaffected.
   (Pinned `rust-toolchain` files are absent in the tree; local cargo is 1.98.1.)
8. Formatting / static analysis: `/opt/homebrew/opt/llvm@23/bin/clang-format --dry-run --Werror` on the
   five changed/new C++ files — clean. (The repository pins clang-format 18.1.8; only 23.1.3 is
   installed on this host, so a version-difference caveat applies.)
9. Generated IPC checks: `python3 tools/ipc/generate.py --check` (rc 0) and
   `python3 tools/ipc/generate_desktop.py --check` (rc 0).
10. Repository privacy: `python3 tools/analysis/check_personal_paths.py` — **0 file(s) flagged** after
    the §4 redaction.
11. Diff checks: `git diff --check` clean; the candidate contains exactly the nine P1-05 files; the
    review branch/HEAD equals current `main` with no commit.

## 8. Remaining maintainer / PD-D05 requirements

- **PD-D05 (legacy CLI disposition: retain vs retire)** remains a Pending Decision. This validation
  implements only the "correct failure exits" step and does not close PD-D05.
- **Test isolation portability.** Before the legacy suite can gate cross-platform CI, its per-test
  isolation must honour the platform home base. Recommended minimal, assertion-preserving seam (§5.2):
  also insert `CFFIXED_USER_HOME` into the isolated environment in `initTestCase()`/`init()` and derive
  the settings suffix from the probe root rather than asserting `startsWith(XDG_CONFIG_HOME)`. This is
  a separately reviewable follow-up and was deliberately **not** folded into this candidate to keep the
  reviewed nine-file scope intact.
- **Discovery-deadline nuance.** Confirm whether a stalled Ollama discovery should be exit `3`
  (documented discovery deadline) or `2` (observed core-reported discovery failure). Both are nonzero;
  the corpus does not currently assert the list-discovery timeout.
- **Independent verification** by a separate owner (AC-P1-07 evidence rule) and an **ENV-HEADLESS / CI**
  run under the adopted P1-04 workflow (this task provides ENV-HOST evidence only).
- **CI matrix caveat.** `main`'s CMake configuration is a merged build, not the standalone P1-05 build;
  candidate-specific Linux/macOS/Windows CI totals must not be inherited from historical results.
- No unsupported claim of tool execution, sandbox, permission or Agent behaviour is made here; the
  legacy CLI has no such path.

## 9. Recommended PR scope

Exactly the nine files in §2, on a base of current `main`
(`70e9fa07`), as a single bounded commit/PR prepared from this isolated worktree, **without** replaying
the accumulated P2/P3/provider history and **without** re-importing already-merged SEC/P2/P3 work:

```text
apps/sentinel-cli/CMakeLists.txt
apps/sentinel-cli/commands/ChatCommand.cpp
apps/sentinel-cli/commands/ExitCodes.h                (new)
apps/sentinel-cli/commands/ModelCommand.cpp
apps/sentinel-cli/main.cpp
docs/audits/P1_05_LEGACY_CLI_EXIT_SEMANTICS_RESULT.md (new; privacy-redacted)
docs/reference/CLI_REFERENCE.md                       (reconciled with main title/scope)
tests/CMakeLists.txt                                  (reconciled with main registrations)
tests/integration/test_legacy_cli_exit_codes.cpp      (new)
```

No `docs/user/CLI.md`, `docs/interfaces/CLI.md`, `cli/**`, workflow or core file belongs in this PR.

## 10. AC-P1-07 status

| Criterion | Status |
| --- | --- |
| Timeout/provider/pull failure nonzero; success/help zero | **Met on current main** — harness 18/18; suite 20/20 with the portable isolation seam |
| Baseline gap (chat timeout and pull failure return 0) closed | **Met** |
| Documented nonzero failure/timeout | **Met** (candidate `docs/reference/CLI_REFERENCE.md`, reconciled with main) |
| Sync and async paths; one terminal decision; bounded waits | **Met** (code + harness) |
| No new Agent entry path / no daemon fallback / no invented tool execution | **Met** (structural + behavioural checks) |
| Injected/subprocess fixture, no real pulls | **Met** (loopback fixture only) |
| ENV-HOST evidence | **Met** (this macOS review host: harness + verification-only suite run) |
| ENV-HEADLESS / CI evidence | **Not claimed** — pending P1-04 CI execution |
| Independent verification by a separate owner | **Pending** (AC evidence rule) |
| PD-D05 dependency | **Open** (Pending Decision) |

**Overall:** the P1-05 exit-code implementation is behaviourally correct on current `main` and the
contract holds, but AC-P1-07 remains **Blocked** — it is not closed by this validation. Remaining to
reach Verified: the suite isolation seam (or an equivalent dedicated fix), an ENV-HEADLESS/CI run, and
the PD-D05 disposition decision.

## 11. Blockers and readiness

Blockers for a clean cross-platform draft:

1. The in-tree legacy suite cannot bootstrap on macOS (XDG-only isolation, §5.2). It passes 20/20 with
   the recommended seam, but the candidate intentionally keeps the reviewed bytes.
2. AC-P1-07 still depends on PD-D05 (Pending Decision) and independent/headless verification.

Ready now for maintainer source/scope review on the primary Linux/ENV-HOST target: the exact
main-relative nine-file candidate, the CMake/docs reconciliation, the privacy redaction and the
`test_llm_agent_runtime` resolution. **No commit, push, merge or PR was performed.** This validation
report is the only new artifact of the task beyond the reconstructed, uncommitted candidate in the
isolated `review/p1-05-current-main` worktree.
