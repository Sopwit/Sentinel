# INT-01 — Multi-worktree integration readiness audit

Audit date: 2026-10-16 (Europe/Istanbul). Read-only source inspection and documentation; no integration or product acceptance execution.

## 1. Baseline and method

**Candidate baseline:** local `main`, stored `origin/main`, live remote `refs/heads/main` and GitHub branch API agreed on `619e989c419956393201a749da1cede386e83ba3`. The new audit worktree is `int-01-integration-readiness/Sentinel-sanitized`, branch `docs/int-01-integration-readiness`, at that same HEAD. Absolute workstation paths are intentionally excluded.

Personal-Brain resolution was attempted (`brain projects brief sentinel --json`), but `brain` was unavailable. Repository identity and worktree paths were resolved from Git and the managed-worktree API instead. Root AGENTS.md, architecture, Agent Runtime, security, building/testing, P0.1/P0.2, acceptance matrix and current workstream reports were inspected. Other worktrees were read only. Git status used optional locks disabled; no fetch, reset, stash, commit, push, cherry-pick, merge, PR creation or workflow dispatch was performed.

Inventory used `git worktree list --porcelain`, `rev-parse`, `status --porcelain --untracked-files=all`, `ls-files`, `log`, `merge-base`, `rev-list --left-right --count`, `diff --name-status`, read-only remote queries and SHA-256 file comparisons. HEAD-relative dirty status is distinct from the useful delta against current main. Ignored build artifacts are evidence inventories, not integration payloads or independent validation.

## 2. Source/worktree inventory

| Alias / branch | Actual HEAD | Main-only / branch-only commits | Entry tracked changes / untracked | State |
| --- | --- | --- | --- | --- |
| Original: `feat/p4-desktop-live-integration` | `0245e5fa94584a357a6b1de01344f69b577e0e41` | 22 / 0 | 0 / 0 | Clean at entry |
| P0 decisions: `docs/p0-03-decisions` | `fe82445b31df7c396c5bb267874e90c72103e840` | 18 / 0 | 0 / 0 | Clean at entry |
| IPC stress: `ci/p1-04-ipc-stress` | `d4230063df6d135057b6af37a8864743ad6a0dd5` | 18 / 0 | 0 / 0 | Clean at entry |
| Old P2 integration: `feat/p2-integration` | `ebad8152cfd9698694536983d06419e16cfc867a` | 38 / 0 | 0 / 0 | Clean at entry |
| Planner integration: `integration/p1-01-7-bounded-planner` | `35cf03c31d43794660a5eefc477bb3b8aba81667` | 16 / 0 | 15 / 10 | Dirty snapshot |
| P0 documentation: `docs/p0-architecture-consistency` | `6ee75bb0def43c76f1bcbd5660c056a8e08226ed` | 21 / 0 | 0 / 0 | Clean at entry |
| Agent source: `test/p1-01-agent-reliability` | `35cf03c31d43794660a5eefc477bb3b8aba81667` | 16 / 0 | 2 / 0 | Dirty snapshot |
| Provider source: `test/p2-01-provider-contracts` | `f27f893e7f32420ae28599cd182862f935847348` | 18 / 0 | 8 / 2 | Dirty snapshot |
| P3 lifecycle: `test/p3-01-lifecycle-ownership` | `619e989c419956393201a749da1cede386e83ba3` | 0 / 0 | 0 / 0 | Clean at entry |

**All nine existing worktree HEADs are ancestors of main.** Their committed work must not be imported again. All registered worktrees were accessible; no separate local P1-05 or P2-06 worktree was registered. Worktree aliases above identify the inspected checkouts without exposing personal paths.

**Snapshots changed during this audit:** Provider source changed `LocalInference.h` and `LocalInference.cpp` and gained untracked `core/include/sentinel/core/runtime/LocalInferenceTransport.h`; its initial 8 modified / 2 untracked inventory became at least 8 / 3. P3 lifecycle changed from clean to modified `tests/CMakeLists.txt` plus untracked `docs/audits/P3_01_MODEL_LIFECYCLE_OWNERSHIP_RESULT.md` and `tests/core/test_model_lifecycle_ownership.cpp`. These are actively changing, incomplete snapshots; the entry manifest is not a freeze of their eventual results. Other seven source worktrees retained entry HEAD, status and dirty-file hashes at the preservation recheck.

Remote-only P1-05: live `fix/p1-05-legacy-cli-exits` and stored remote tracking agree on `43c7bef0fad84d6cd782080132b5eaabbb2dd55d`. This commit is **not** in main; its merge base is `e71cca4e958109e0825b6277072bd1235e77e60a`. Exactly one branch-only commit was identified. No local P1-05 dirty or ignored inventory can be certified without its checkout. Its nine committed paths are `apps/sentinel-cli/CMakeLists.txt`, `apps/sentinel-cli/commands/ChatCommand.cpp`, `apps/sentinel-cli/commands/ExitCodes.h`, `apps/sentinel-cli/commands/ModelCommand.cpp`, `apps/sentinel-cli/main.cpp`, `docs/audits/P1_05_LEGACY_CLI_EXIT_SEMANTICS_RESULT.md`, `docs/reference/CLI_REFERENCE.md`, `tests/CMakeLists.txt` and `tests/integration/test_legacy_cli_exit_codes.cpp`.

Ignored roots: Original has build output, Rust target output, platform metadata/install log and Python caches; Planner integration, Provider source and P3 lifecycle have `build/`; Agent source has `build/` and analysis Python cache; the remaining four inspected worktrees had no ignored roots. Tracked inventory counts at entry were respectively 5226, 5229, 5229, 5204, 5232, 5227, 5232, 5233 and 5242 in table order.

Relevant ignored evidence exists at `build/tests/Testing/Temporary/LastTest.log` in Original, Planner integration, Agent source, Provider source and P3 lifecycle (entry/recheck sizes respectively 438006, 505443, 479629, 467106, 31209 bytes). Build directories and logs can change independently of Git and are not portable evidence packets. Never import ignored binaries, caches or these raw logs. Audit did not rebuild or run product tests in any source checkout.

## 3. Commit ancestry and existing integration

- Main `619e989c` merges provider branch `f27f893e`; parent integration `7d311eea` merges Agent branch `35cf03c3`.
- Earlier main ancestry already merges `feat/p2-integration` (`ebad8152`), P0 documentation (`6ee75bb0`), P0-03 decisions (`fe82445b`) and IPC stress (`d4230063`). These merge commits record containment, not acceptance.
- Provider `f27f893e` contains P2-04 documentation and `868fa5c3` HTTP classification correction; prior P2 contract/redaction/vision/discovery evidence is already in main.
- Agent `35cf03c3` includes COR-12/13 repairs and committed P1-01.5 transport/privacy coverage. The current remaining corpus expansion P1-01.6 is uncommitted, as is P1-01.7.
- Planner integration starts at `35cf03c3`, imports the uncommitted Agent two-file snapshot, imports prior committed P2 work and imports P2-05. Its current branch pointer alone does not contain these dirty additions. Importing its original-base diff wholesale would repeat work already in main.
- P3 lifecycle starts exactly at main. Its new lifecycle tests/report are uncommitted and independent of the planner/P2-06 completion claim.

Read-only reproducibility: for each recorded HEAD run `git merge-base --is-ancestor <HEAD> main` and `git rev-list --left-right --count main...<HEAD>`; compare dirty bytes separately using SHA-256. Re-resolve live main and dirty source manifests immediately before any authorized integration. Do not use branch names as immutable evidence.

## 4. File-conflict and duplicate matrix

| Area | Sources / current finding | Required treatment |
| --- | --- | --- |
| `tests/CMakeLists.txt` | Main already registers `test_provider_contracts`; old Planner integration diff adds it again alongside bounded streaming. Provider adds bounded streaming; P1-05 adds legacy exit target; active P3 adds lifecycle target. | Reconstruct a main-relative registration union once per target. Source diff overlap is proven; textual merge conflicts have not been simulated. |
| P2-05 interface, adapters, runtime, fixture | Nine non-CMake files initially byte-identical between Planner integration and Provider source (listed below). Provider runtime files subsequently diverged during P2-06 work. | Freeze a P2-05-only snapshot; do not take current provider directory wholesale or import both copies. |
| `LlmAgentRuntime.cpp` | Planner integration alone introduces bounded planner streaming, depends on P2-05 interface. | Review as production behavior change in dependent PR, not a documentation/test-only import. Preserve finite deadline, binding, final and permission gates. |
| Agent harness + P1 report | Agent source and Planner integration differ. Planner report contains the complete Agent report as a byte-for-byte prefix. | Preserve P1-01.6 then apply only P1-01.7 extension; review harness diff separately. Do not copy older main over repaired/new fixtures. |
| Already imported provider files/reports | Eleven dirty/untracked Planner files are byte-identical to main (listed below). | Exclude from payload; do not re-add or relabel their historical acceptance. |
| Native provider fixture seam | Active Provider edits overlap P2-05 `LocalInference.h/.cpp`; new transport header is untracked. | P2-06 incomplete dependency; no silent inclusion in P2-05/planner PR. |
| P3 lifecycle | Active CMake registration overlap; model lifecycle production areas also overlap provider ownership concepts. | Separate test-only review on current main; future ownership changes require PD-D10 and fresh overlap review. |
| Legacy reference docs | P1-05 modifies `docs/reference/CLI_REFERENCE.md`, already touched by P0 documentation in main. | Preserve accurate daemon/legacy distinction while applying only exit-semantics delta. |
| Audit appendices | P1 histories accumulate in one report; P2-05/P3 reports untracked and still changing. | Append dated evidence, preserve old failures/skips; reconcile summary statements without rewriting historical results. |

### Entry snapshot: pending file inventory

**Planner integration** (`35cf03c3` plus uncommitted bytes):

- `core/include/sentinel/core/chat/OllamaChatProvider.h` — SHA-256 `a2c4ba696335f7147ce12e002e977fd140c716d6d5d79c6247a53aa52df78c1b`.
- `core/include/sentinel/core/interfaces/IChatProvider.h` — SHA-256 `c0000502003e7f48c8dbe0454473984f9ed71ed1a362598a14c2785e97e42397`.
- `core/include/sentinel/core/runtime/LocalInference.h` — SHA-256 `c9ebf1e38dfce902468192745008af5bac5da9900db09aa6c8b537250efa1257`.
- `core/src/agent/LlmAgentRuntime.cpp` — SHA-256 `0cf84d2885bf9cea69ca0aedda54033a8d9b3ba6fe9cabcd3d646491ad2c318b`.
- `core/src/chat/OllamaChatProvider.cpp` — SHA-256 `b90756515aa03d081d70e8c2fc39142410f7f5709725df00eadab4b76578d886`.
- `core/src/model/ModelService.cpp` — SHA-256 `e72c9d76aa4657b0804b6c1e26295b7b865930f648872420b7454e862730e808`.
- `core/src/provider/ProviderErrorClassifier.cpp` — SHA-256 `d0f76c72773fd7066bfa95d574e2588b6e5c86b5d09f5737bb0dc9ccf23a7161` — identical to main; omit.
- `core/src/runtime/LocalInference.cpp` — SHA-256 `97b4da21169716e954cae5a9c310cd5db118cc13c845687f787379ccc0e0b27f`.
- `core/src/runtime/OllamaRuntime.cpp` — SHA-256 `b289ef1498c97c38d488c759529d069fcff585157a2e1963014e8556ae285051` — identical to main; omit.
- `core/src/runtime/ProviderRequestRuntime.cpp` — SHA-256 `02de4168a7d630160683d6dcc70473ddf25ecbf4468880cfde40d44f67761248` — identical to main; omit.
- `docs/audits/P1_01_7_PLANNER_STREAMING_INTEGRATION_RESULT.md` — SHA-256 `567992dc81313e14b8df02b82ab0a6c41e4a919233eba320f9d766878f50b971`.
- `docs/audits/P1_01_AGENT_RELIABILITY_HARNESS_RESULT.md` — SHA-256 `b867715b308a613a22a830ad4f78da003b53dc304549ed495ce8ca7a1c1686a6`.
- `docs/audits/P2_01_PROVIDER_CAPABILITY_CONTRACT_RESULT.md` — SHA-256 `dac487715adf7567537d156799938981837550fe6a463a06a0d61b899684d4d4` — identical to main; omit.
- `docs/audits/P2_02_PROVIDER_ERROR_REDACTION_RESULT.md` — SHA-256 `b3439bd3b1e688b24c0e2b2cd41d0f18a87628a32207e29ca0616a4f16d44b19` — identical to main; omit.
- `docs/audits/P2_03_VISION_CAPABILITY_ENFORCEMENT_RESULT.md` — SHA-256 `8e7e0bc9455c3650648d5bd2ccdd0c359c06b44e5ec1a24daee8fc271e368671` — identical to main; omit.
- `docs/audits/P2_04_HTTP_CLASSIFICATION_RESULT.md` — SHA-256 `76b2ae01795708f8ed02acca59b632655614ba79cd76c425949b93ad27e33cfa` — identical to main; omit.
- `docs/audits/P2_05_BOUNDED_STREAMING_CONTRACT_RESULT.md` — SHA-256 `498ac79757fb4b0648f3fdede55a211aeb1fd4998ec66ba12e6d5e8ea24300d0`.
- `docs/audits/P2_DEF_01_MODEL_DISCOVERY_REMEDIATION_RESULT.md` — SHA-256 `901dc2e1195a2c8798d33d027462c5154e8f26fbb15df36622185e4d5db46635` — identical to main; omit.
- `tests/CMakeLists.txt` — SHA-256 `10b3f44d6d8fbde441029cb41f265ad3f541eb633fb73ba7b515407f33ec64de`.
- `tests/core/test_agent_reliability_harness.cpp` — SHA-256 `54c2abdfcb49a7591061283eb9e72ef3723bd2825d6b04eb35a99d730165bae2`.
- `tests/core/test_bounded_streaming_contract.cpp` — SHA-256 `794f2c99e20e5aee2a66d6739ff9358eec22a73d5c05a6b7f0cdd2ff7b6eb831`.
- `tests/core/test_local_inference.cpp` — SHA-256 `e415b6b04cd3754ebbf17a0c38bc61e312bfa86362134872c34f6517f01a4ade` — identical to main; omit.
- `tests/core/test_provider_contracts.cpp` — SHA-256 `4e5ecb20c157ccf74dedbed786369369ae8d8ad44ef23fb24b55bf1f07e492f7` — identical to main; omit.
- `tests/support/DeterministicChatFixture.h` — SHA-256 `9d79e71bcb18027921bc8f2093f1fdd0c60b88255fcfead4dd3b1c71814c59a9`.
- `tests/support/ProviderHttpFixture.h` — SHA-256 `fea60e088c9fac4dd684edd3fd9f53e6734fc200b4132d44470a2ec1e44140d9` — identical to main; omit.

**Agent source** (`35cf03c3` plus uncommitted bytes):

- `docs/audits/P1_01_AGENT_RELIABILITY_HARNESS_RESULT.md` — SHA-256 `39cbcee7cc99b4f6174fe3b449a34f4ce8f54a4f8ef40e54aac536c35ff9e9bb`.
- `tests/core/test_agent_reliability_harness.cpp` — SHA-256 `1ce3356abb2de2cd33be90e82cd7bdc31f7b41a1c242326b5dff7d17a4aca2ed`.

**Provider source** (`f27f893e` plus uncommitted bytes):

- `core/include/sentinel/core/chat/OllamaChatProvider.h` — SHA-256 `a2c4ba696335f7147ce12e002e977fd140c716d6d5d79c6247a53aa52df78c1b`.
- `core/include/sentinel/core/interfaces/IChatProvider.h` — SHA-256 `c0000502003e7f48c8dbe0454473984f9ed71ed1a362598a14c2785e97e42397`.
- `core/include/sentinel/core/runtime/LocalInference.h` — SHA-256 `c9ebf1e38dfce902468192745008af5bac5da9900db09aa6c8b537250efa1257`.
- `core/src/chat/OllamaChatProvider.cpp` — SHA-256 `b90756515aa03d081d70e8c2fc39142410f7f5709725df00eadab4b76578d886`.
- `core/src/model/ModelService.cpp` — SHA-256 `e72c9d76aa4657b0804b6c1e26295b7b865930f648872420b7454e862730e808`.
- `core/src/runtime/LocalInference.cpp` — SHA-256 `97b4da21169716e954cae5a9c310cd5db118cc13c845687f787379ccc0e0b27f`.
- `docs/audits/P2_05_BOUNDED_STREAMING_CONTRACT_RESULT.md` — SHA-256 `498ac79757fb4b0648f3fdede55a211aeb1fd4998ec66ba12e6d5e8ea24300d0`.
- `tests/CMakeLists.txt` — SHA-256 `5155ab321c81033ef8c08010d958b122accc8fbb57e778034cbb163ddbed0621`.
- `tests/core/test_bounded_streaming_contract.cpp` — SHA-256 `794f2c99e20e5aee2a66d6739ff9358eec22a73d5c05a6b7f0cdd2ff7b6eb831`.
- `tests/support/DeterministicChatFixture.h` — SHA-256 `9d79e71bcb18027921bc8f2093f1fdd0c60b88255fcfead4dd3b1c71814c59a9`.

The nine shared non-CMake P2-05 entry files are `IChatProvider.h`, `OllamaChatProvider.h/.cpp`, `LocalInference.h/.cpp`, `ModelService.cpp`, `DeterministicChatFixture.h`, `test_bounded_streaming_contract.cpp` and `P2_05_BOUNDED_STREAMING_CONTRACT_RESULT.md` at their full paths above. `tests/CMakeLists.txt` differs between the two snapshots because the older Planner baseline lacks the already-main provider registration.

## 5. Evidence reconciliation and open acceptance

| Workstream | Available evidence, provenance and limit | Current gate |
| --- | --- | --- |
| P0 architecture | [Documentation reconciliation](P0_DOCUMENTATION_CONSISTENCY_RESULT.md), already in main. | Review current active docs; do not re-import historical source snapshots. |
| P0-03 decisions | [Decision review](P0_03_OWNERSHIP_COMPATIBILITY_REVIEW.md), recommendations for 14 pending records. | Maintainer adoption still required; recommendations are not adopted decisions. |
| P1-01 through 1.5 | [Historical harness evidence](P1_01_AGENT_RELIABILITY_HARNESS_RESULT.md), including repaired COR-12/13, real COR-17/18 IPC and independent compile/test oracles. | Preserve original failures/skips. Current main report is not evidence for dirty P1-01.6/1.7. |
| P1-01.6 | Agent source report appendix: 24 new corpus rows, focused 26 Qt records; harness 96 passed / 2 skipped; reported 129 CTest targets passed. | Uncommitted; independent candidate validation still needed. |
| P1-01.7 | Planner integration report: harness 97 passed / 1 skipped; IPC 5; bounded contract 18; LLM runtime 49; full 131 CTest targets passed, 188.24 s on macOS. | Source-reported evidence, not tests rerun by this audit. Native tool-assembly skip and native/live evidence remain. |
| P1-04 | [Hosted stress readiness](P1_04_4_HOSTED_IPC_STRESS_INTEGRATION.md); dispatch job exists. GitHub query returned **zero workflow_dispatch runs** for `ci-rust.yml`. | AC-P1-04 not Passed. Need both 100-repeat hosted Unix cells at the reviewed candidate. |
| P1-05 | Remote commit report: legacy exit suite 20/20; full regression 127/128, existing LLM descriptor failure; independent reviewer/headless evidence and PD-D05 open. | Not in main; rerun on integrated baseline, do not inherit old failure or assume it resolved. |
| P2-01 through 04 / discovery | [Provider contracts](P2_01_PROVIDER_CAPABILITY_CONTRACT_RESULT.md), [redaction](P2_02_PROVIDER_ERROR_REDACTION_RESULT.md), [vision](P2_03_VISION_CAPABILITY_ENFORCEMENT_RESULT.md), [HTTP errors](P2_04_HTTP_CLASSIFICATION_RESULT.md), [discovery](P2_DEF_01_MODEL_DISCOVERY_REMEDIATION_RESULT.md), already in main. | Mock/API contract evidence does not establish live backend reliability or AC-P2-01 completion. |
| P2-05 | Entry report shared with Planner: bounded suite 18 Qt records; provider contracts 205; local inference 51; full 129 CTest targets passed, 163.10 s, source-reported. | Freeze before review; native structured/tool assembly unsupported in bounded path. |
| P2-06 | User explicitly identifies native Claude/Gemini fixtures as in progress. Provider source acquired transport injection seam during audit; no completed P2-06 report was present in inspected snapshot. | **Incomplete, unstable, not validated or approved for import.** Absence of report is not absence of work. |
| P3 | Newly appearing lifecycle report/test: partial fixture evidence; report records prior failed rows and full regression failure. | Active source; AC-P3-02 blocked on identity/unload/lease and PD-D10, not complete. |

**Current documentation inconsistency:** Planner report §12/13 repeats COR-01 truncation, COR-10 planning-stage and COR-16 reread gaps, but its preserved P1-01.6 appendix explicitly reports those deterministic variants covered. This is an active summary mismatch, not justification to erase history. A future P1 PR should append/correct the current summary to distinguish completed synthetic variants from remaining live/platform coverage. This audit changes neither source report.

**AC-P1-01 remains incomplete:** native incremental tool-call assembly, COR-06 live capability integrations and required independent/native/live evidence remain. COR-01/06/10/16 deterministic extensions exist in the uncommitted source; main lacks them. Green target totals cannot remove required skips or establish all corpus variants.

**Historical privacy blocker:** `docs/audits/P1_04_2_LINUX_CI_VALIDATION.md` remains flagged by the repository personal-path scanner at baseline; SHA-256 `f00a8bcd35c4ad5d47d2ef4db070f4f9f3e0b85d86f4bad7209db34f3d67809d`. Audit reproduced one flagged file and did not print its sensitive content or modify it. This can block normal Linux/macOS/Windows CI before builds. Earlier workstream claims of clean scans on older branch bases do not clear this main-baseline finding. Obtain a separately authorized, history-preserving public-evidence remediation; no silent exclusion/waiver or historical rewrite in this audit.

## 6. Recommended integration order and proposed PRs

1. **INT-01 documentation PR**, after explicit publication authorization. Baseline is verified; only this report is proposed. Review inventory/unstable sources and privacy handling before preparing code payloads.
2. **P0 decisions review** without re-merging P0 branches. Adopt PD-D11 reviewer ownership and PD-D01 candidate/profile policy first; resolve PD-D05 CLI disposition and PD-D10 lifecycle/compute ownership for their affected closure. PD-D02/03 model/native access precede PD-D04 calibration. Do not change acceptance thresholds.
3. **P2-05 bounded streaming PR**: main-relative, frozen ten-file entry payload (including CMake once), excluding already-main imports and subsequent P2-06 seam changes. Security/provider owner review and fresh CI on exact candidate.
4. **P1-01.6/1.7 dependent PR**: preserve Agent additions and apply Planner-only runtime/harness/report delta after P2-05. This is dependent on provider contract, not on finishing P2-06. Do not import the integration worktree wholesale. Reconcile current report summary and keep historical sections intact.
5. **P1-05 legacy exit PR** independently from frozen remote commit after owner confirms compatible exit contract; keep distinct from Rust/daemon authority. Reconcile CMake/CLI docs and rerun its recorded LLM failure on current main. It may proceed in parallel with provider review if reviewer capacity permits.
6. **P1-04 hosted stress evidence** on each reviewed IPC candidate before AC-P1-04 closure; implementation already in main. Explicitly authorize workflow dispatch separately. Any later IPC change invalidates candidate-specific stress evidence and requires rerun.
7. **P2-06 native fixture PR**, only after owner freezes and finishes source/report. Rebase plan onto accepted P2-05 interface, review injected transport security boundary and independently validate native API serialization/errors/cancellation without contacting cloud. Do not block independent P3 inventory or bundle unfinished provider work.
8. **P3 lifecycle PR** separately once its active test/report snapshot is frozen. Reconcile CMake once; lifecycle production changes, if proposed later, need PD-D10 and cross-provider ownership review. No automatic dependency on P2-06.

Every grouping requires a main-relative allowlist, file hashes, reviewed HEAD/dirty delta, newly discovered CTest target list, preserved fixture inventory and all failed/skipped attempts. First freeze the source snapshot with its owner; uncommitted work must not be overwritten or treated as a reusable commit.

## 7. CI and verification gates

Common C++ gate in a future isolated candidate (not run by this audit):

```sh
cmake --preset tests
cmake --build --preset tests
ctest --test-dir build/tests -N
QT_QPA_PLATFORM=offscreen ctest --preset tests --output-on-failure
python3 tools/ipc/generate.py --check
python3 tools/ipc/generate_desktop.py --check
python3 tools/analysis/check_personal_paths.py
git diff --check
```

Discover actual targets per candidate; never require a historical fixed total. Use bounded test timeouts and disposable synthetic fixtures. Missing tools, compiler failures, product failures and required skips are separate outcomes.

| Group | Expected focused targets / checks | Required additional evidence |
| --- | --- | --- |
| Documentation / decisions | Relative links, Markdown fences/tables, whitespace, changed-file credential/personal-path review; preserve acceptance/roadmap files. | Reviewer confirms scope; full repository privacy finding tracked separately. |
| P2-05 / P2-06 | `test_bounded_streaming_contract` (new), `test_provider_contracts`, `test_local_inference`, `test_runtime_provider`, `test_provider_credentials`, `test_llm_agent_runtime`, `test_model_service` if registered. | Discover names via CTest before selecting. Parser/auth/redaction/cancel/no-fallback/unsupported features; no real cloud calls or model downloads. Native fixtures do not prove real endpoint/model reliability. |
| P1 planner/corpus | `test_agent_reliability_harness`, `test_agent_reliability_ipc`, `test_agent_runtime`, `test_llm_agent_runtime`, `test_daemon_ipc`, `test_desktop_ipc`. | Exact terminal/tool/mutation oracles, compile/link/test subprocess negatives, permission revocation, actual first streamed delta, stale/foreign identity fencing; retain native-assembly skip. |
| P1-05 | `test_legacy_cli_exit_codes` (new), `test_llm_agent_runtime`, full CTest. | Success/help zero; usage/failure/timeout nonzero; sync/async subprocess outcomes and existing failure independently investigated; no new Agent entry path. |
| P1-04 IPC | Rust workspace fmt/clippy/tests; C++ daemon/Desktop IPC and generated contracts. | Hosted 100 sequential unfiltered repeats per Ubuntu/macOS cell, no retry-to-green; all outcomes/exit codes/logs, runner/compiler/source SHA, workflow identity and artifact URLs. |
| P3 lifecycle | `test_model_lifecycle_ownership` (new), existing model/runtime/process/storage targets discovered via CTest. | Owned vs external resources; request/binding lease/health identity; no real external processes killed or models altered. Native identity/unload gaps stay explicit. |

Rust gate from `cli/` follows current CI: `cargo +1.94.0 fmt --all --check`, `cargo +1.94.0 clippy --locked --all-targets -- -D warnings`, `cargo +1.94.0 test --locked`. Hosted stress additionally uses `cargo +1.94.0 test --locked -p sentinel-ipc` 100 times in each matrix cell, not a substitute for full workspace validation.

C++ portability gate uses existing Linux (`ubuntu-22.04`, Qt 6.11), macOS (`macos-14`, Qt 6.11) and Windows x64 (`windows-2022`, Qt 6.11.3/MSVC) workflows. Linux quality includes clang-format 18.1.8 changed-line checks, clang-tidy, QML lint and desktop metadata; ASan belongs to shared runtime/provider changes. Windows ARM64 is experimental and nonblocking, not verified product support. Rust hosted matrix is Unix only; Windows named-pipe Rust transport limitations require explicit unsupported scope, not assumed parity. Native Fedora KDE shortcuts, headless clipboard and Linux application launch limits remain open platform evidence.

Before publishing any PR: scan changed files and complete candidate for personal paths, credentials, private profiles/prompts and generated build artifacts. Use synthetic keys and redact diagnostics. Preserve dated historical failures/skips and evidence context; never replace them with a green aggregate. AC-P1-01 and AC-P2-01 require the acceptance matrix EV-01 packet and independent reviewer/environment evidence, not mock or unit totals alone.

## 8. GitHub state and maintainer authorization

Read-only GitHub inspection found [draft PR #17](https://github.com/sentinel-projects/sentinel/pull/17) (`fix/p2-cross-platform-ci-stabilization`) and dependency PRs #8, #12–16. They are separate pending work, not authorized imports; CI stabilization may affect interpretation of native failures and must be reviewed against the same candidate.

The [Rust main run](https://github.com/sentinel-projects/sentinel/actions/runs/38919069524) for `619e989c` was successful but was a **push** run, not hosted 100-repeat stress evidence. The explicit workflow-dispatch query returned total_count 0. GitHub branch API returned `protected: false`; repository rulesets API returned an empty list. These observations are not permission to bypass review. CODEOWNERS names `@Sopwit`; additional independent reviewer capacity must be assigned explicitly (PD-D11).

Maintainer must authorize, separately and explicitly:

- The exact frozen main-relative file payload and PR grouping, candidate SHA/digests and named code/security/validation reviewers; resolve active owner edits before taking a snapshot.
- Commit and push of approved snapshots and creation of named draft PRs. None is authorized or performed by this audit.
- Dispatch of hosted stress against a selected SHA, retention/download/publication of sanitized evidence and review of every failed attempt. No workflows were triggered here.
- A separate privacy remediation scope preserving dated source context and sensitive original handling; do not approve publication of raw personal paths.
- Adoption of pending product decisions and acceptance closure by the designated owners. PD-D05/10 and native/model/calibration/retention requirements remain open as applicable.
- Merge only after fresh candidate-specific CI, independent validation, conflicts resolved and reviewer approval. Live endpoint/model testing needs its own explicit network/credential/model authorization; no such execution is part of this task.

## 9. Audit validation and safe next action

Read-only ancestry, remote identity, status, file-hash comparisons and preservation rechecks were performed. Generated IPC/Desktop `--check` commands passed in both the inspected Provider snapshot and the isolated audit baseline without generation. Audit report relative links, fenced Markdown, whitespace and personal-path checks passed; only this new document is present in audit Git status. Repository privacy scan reproduced the one inherited finding above. Product builds, CTest, Rust tests, native runs and hosted stress were deliberately not executed: their cited totals are historical/workstream claims and do not certify a new integrated candidate.

**Safe next action:** review this audit, assign snapshot owners, then freeze only the P2-05 entry payload and the dependent P1-01.6/1.7 delta in newly authorized integration candidates. Re-resolve main and compare hashes before copying anything. Keep active P2-06 and P3 edits out of those payloads. Resolve privacy publication handling and arrange independent/native CI before requesting code merge approval.

**Readiness:** documentation audit is ready for maintainer review after its local checks. No production integration PR is certified ready to merge or to close acceptance: active snapshots, privacy failure, missing hosted stress, independent/native/live evidence and pending decisions remain blockers. No commit, push, PR or merge was performed.
