# INT-03 — Concurrent Git history and integration snapshot reconciliation

Audit date: 2026-10-16 (Europe/Istanbul). Read-only inspection of existing worktrees; only this new report is written in the dedicated audit worktree. No product validation or acceptance execution.

## 1. Main identity and movement during inspection

Audit branch `docs/int-03-source-reconciliation`, managed worktree `int-03-source-reconciliation/Sentinel-sanitized`, baseline HEAD **`70e9fa072af9b4139d86f518c6d89f9c849c29b2`**. The clean local main at that SHA was the latest verified local main when the worktree was created. Absolute workstation paths and sensitive original identifiers are omitted.

At entry local main was `70e9fa07`, while stored `origin/main`, live `ls-remote` and GitHub branch API reported **`a69a9f3775c8b2f4670dfd9772f8b8c2d36db85b`**. Local main was eight commits ahead, none behind. During this audit the live remote main **and stored origin/main advanced externally to `70e9fa07`**. Final observed local/stored/live/API main agree. No fetch, push, existing ref/index mutation or merge was executed by this audit. This publication movement is preserved as evidence, not silently substituted for the entry state.

All twelve pre-existing registered worktrees were accessible and clean, with zero tracked modifications and zero nonignored untracked files at entry and the first preservation recheck. At the later delivery check, Provider source gained untracked `docs/audits/P2_09_PROVIDER_POLICY_LIVE_ACCEPTANCE_PLAN.md`; its HEAD remained f87843f1. First observed SHA-256: `7f23ab16ebfe200d44c7379114d7751c4c1821fd47f5129b545685f7a7e58f79`. This is an actively changing future-policy document, excluded from the completed P2-05…08 candidate and not imported. Other eleven source worktree HEAD/status snapshots stayed unchanged at that check. Remote/tracking publication also changed as recorded above. These are bounded observations, not future snapshot locks.

Preflight: attempted `brain projects brief sentinel --json` (unavailable); resolved paths from Git worktree inventory and managed-worktree API. Read AGENTS.md, [INT-01](INT_01_MULTI_WORKTREE_INTEGRATION_READINESS.md), [INT-02](INT_02_SEC_01_INTEGRATION_REVIEW.md), architecture/runtime/security/build/testing, acceptance/decision registers and relevant provider/lifecycle reports. Historical reports describe their dates/baselines; they were not updated to resemble this state.

## 2. Worktree and committed/uncommitted inventory

The counts below are local-main-only / branch-only commits at `70e9fa07`; every existing worktree HEAD is an ancestor of local main. All are also contained in the final observed remote main.

| Alias / actual branch | Actual HEAD | Main-only / branch-only | Entry dirty / untracked |
| --- | --- | --- | --- |
| Original main: `main` | `70e9fa072af9b4139d86f518c6d89f9c849c29b2` | 0 / 0 | 0 / 0 |
| P0 decisions: `docs/p0-03-decisions` | `fe82445b31df7c396c5bb267874e90c72103e840` | 43 / 0 | 0 / 0 |
| IPC stress: `ci/p1-04-ipc-stress` | `d4230063df6d135057b6af37a8864743ad6a0dd5` | 43 / 0 | 0 / 0 |
| Old P2 integration: `feat/p2-integration` | `ebad8152cfd9698694536983d06419e16cfc867a` | 63 / 0 | 0 / 0 |
| INT-01: `docs/int-01-integration-readiness` | `0183f1546cbd6dee00844902e764870e5bf495e4` | 24 / 0 | 0 / 0 |
| INT-02 SEC candidate: `review/int-02-sec-01-integration` | `467021b6d3c7aebbb167d8540cb8f2d595af3589` | 22 / 0 | 0 / 0 |
| Planner integration: `integration/p1-01-7-bounded-planner` | `9cdbd883347d05c671fa1a56f1a514e8040e0847` | 39 / 0 | 0 / 0 |
| P0 documentation: `docs/p0-architecture-consistency` | `6ee75bb0def43c76f1bcbd5660c056a8e08226ed` | 46 / 0 | 0 / 0 |
| Agent source: `test/p1-01-agent-reliability` | `91640b24e0e8c53a5cad82dd9257277833c10af6` | 39 / 0 | 0 / 0 |
| Provider source: `test/p2-01-provider-contracts` | `f87843f1edcab1e5e02defa61b8e560dcc001d92` | 39 / 0 | 0 / 0 |
| P3 lifecycle: `test/p3-01-lifecycle-ownership` | `53d0032a456a14a517552fe4c3d57a879167b148` | 21 / 0 | 0 / 0 |
| SEC source: `fix/sec-01-audit-privacy` | `58355635b61e7908bd608f956949995715953f11` | 23 / 0 | 0 / 0 |

There is no registered legacy P1-05 checkout. Its accessible stored/live remote branch `fix/p1-05-legacy-cli-exits` is at **`43c7bef0fad84d6cd782080132b5eaabbb2dd55d`**, with one branch-only commit not contained in main; merge base `e71cca4e958109e0825b6277072bd1235e77e60a`. No local legacy dirty/ignored snapshot is inferred.

### Ignored build/evidence inventory

Ignored files were inspected by names, sizes and SHA-256 only; no build output was imported or executed. Machine-specific cache contents and raw logs are not public artifacts.

| Worktree | Ignored roots | Relevant ignored evidence |
| --- | --- | --- |
| Original main | `.DS_Store`, `aqtinstall.log`, `assets/.DS_Store`, `assets/core-warden/.DS_Store`, `build/`, `cli/target/`, `tools/agent/__pycache__/`, `tools/analysis/__pycache__/` | `build/tests/CMakeCache.txt` (76428 bytes), `build/tests/compile_commands.json` (1248987 bytes), `build/tests/CTestTestfile.cmake` (387 bytes), `build/tests/Testing/Temporary/LastTest.log` (75345 bytes), `build/tests/Testing/Temporary/LastTestsFailed.log` (89 bytes) |
| P0 decisions | None | None in inspected build/tests evidence locations |
| IPC stress | None | None in inspected build/tests evidence locations |
| Old P2 integration | None | None in inspected build/tests evidence locations |
| INT-01 | None | None in inspected build/tests evidence locations |
| INT-02 SEC candidate | None | None in inspected build/tests evidence locations |
| Planner integration | `build/` | `build/tests/CMakeCache.txt` (76988 bytes), `build/tests/compile_commands.json` (1449975 bytes), `build/tests/CTestTestfile.cmake` (459 bytes), `build/tests/Testing/Temporary/LastTest.log` (505443 bytes) |
| P0 documentation | None | None in inspected build/tests evidence locations |
| Agent source | `build/`, `tools/analysis/__pycache__/` | `build/tests/CMakeCache.txt` (76596 bytes), `build/tests/compile_commands.json` (1369227 bytes), `build/tests/CTestTestfile.cmake` (435 bytes), `build/tests/Testing/Temporary/LastTest.log` (479629 bytes) |
| Provider source | `build/` | `build/tests/CMakeCache.txt` (76603 bytes), `build/tests/compile_commands.json` (1384942 bytes), `build/tests/CTestTestfile.cmake` (437 bytes), `build/tests/Testing/Temporary/LastTest.log` (518133 bytes), `build/tests/Testing/Temporary/LastTestsFailed.log` (51 bytes) |
| P3 lifecycle | `build/`, `tools/analysis/__pycache__/` | `build/tests/CMakeCache.txt` (76610 bytes), `build/tests/compile_commands.json` (1393335 bytes), `build/tests/CTestTestfile.cmake` (439 bytes), `build/tests/Testing/Temporary/LastTest.log` (506698 bytes), `build/tests/Testing/Temporary/LastTestsFailed.log` (29 bytes) |
| SEC source | None | None in inspected build/tests evidence locations |

LastTestsFailed.log can outlive a passing run. Its presence, cached binaries or LastTest.log is not fresh candidate validation. Entry and recheck manifests remain private temporary audit evidence; no original sensitive path text is reproduced.

## 3. Commit ancestry diagram and containment

```text
main first-parent chain:
619e989c -> 0183f154 -> 78a80d93 -> 1d7f4a5a -> 42b9d08e
         -> 74ba91e2 -> a69a9f37 -> 67a80a0c -> 70e9fa07

side histories merged at the indicated main commits:
SEC source: 9033bc62 -> 58355635 ----------------------> 78a80d93
INT-02:     83639109 -> a2f432a7 -> 467021b6 ----------> 1d7f4a5a
P3 early:   3863cdd5 -> 5d8ce5ba ----------------------> 42b9d08e
P1 corpus:  e6bb5ebd -> 91640b24 ----------------------> 74ba91e2
Planner:    2f5464d0 -> 9cdbd883 ----------------------> a69a9f37
P2: f27f893e -> fe2edb00 -> b7530494 -> e33db6de -> f87843f1
                                                      -> 67a80a0c
P3 final:   5d8ce5ba -> 09060055 -> 53d0032a ----------> 70e9fa07

P1-05: e71cca4e -> 43c7bef0     (not merged)
```

Diagram summarizes verified first-parent order and side-branch containment, not product dependencies or acceptance. Both SEC histories represent equivalent two-file changes with distinct commit IDs; keep history intact and do not import either again.

The eight commits absent from entry remote `a69a9f37`, now present in remote/main, were:

- `70e9fa072af9b4139d86f518c6d89f9c849c29b2` — Merge branch 'test/p3-01-lifecycle-ownership'.
- `67a80a0c38bf9d4b7414e213d9a05ae68921f307` — Merge branch 'test/p2-01-provider-contracts'.
- `53d0032a456a14a517552fe4c3d57a879167b148` — docs(audit): add P3-03 lifecycle readiness remediation result.
- `f87843f1edcab1e5e02defa61b8e560dcc001d92` — docs(audit): add P2-08 cloud endpoint security capability result.
- `090600555717e5326c1a1caa4d4f3a3c74ed9502` — fix(daemon): harden LocalModelRuntime lifecycle readiness.
- `e33db6de90b3bdc1e66e38acfcc2e47118abad8d` — fix(provider): validate cloud endpoints and route discovery through injectable transport.
- `b7530494862c52178539439f1d728ec722396b20` — docs(audit): add P2-05/P2-06/P2-07 provider contract results.
- `fe2edb00106a7b5dc5499fd0993e936cfbea9385` — feat(provider): enforce bounded streaming contract and add native/vendor cloud fixtures.

## 4. P2-05 through P2-08 tested-candidate reconciliation

| Stage | Exact source/history meaning | Current main disposition |
| --- | --- | --- |
| P2-05 | `IChatProvider` bounded overload/advertisement; Ollama and selected endpoint adapters; local request bound; deterministic fixture and bounded target/report. Selected copies also live in planner commit `2f5464d0`. | Already in final local/remote main. No remaining P2-05 payload; do not import both copies. |
| `fe2edb00106a7b5dc5499fd0993e936cfbea9385` | 18-file accumulation includes P2-05, native/vendor injection and fixtures, plus parts of P2-08 endpoint/security tests and implementation. Subject is not a phase boundary. | Contained in main, but **historically an incomplete P2-08 candidate**. Never publish this intermediate state as the tested P2-08 result. |
| `b7530494862c52178539439f1d728ec722396b20` | Adds P2-05/06/07 reports; its P2-08 executable inputs remain the same incomplete bytes as fe2edb00. | Contained in main. Reports alone did not complete P2-08. |
| `e33db6de90b3bdc1e66e38acfcc2e47118abad8d` | Seven-file correction: endpoint/discovery transport, vendor validation/redirects, capability guard and adaptation of historical HTTPS fixtures. | Supplies the missing final source corrections. Contained in main. |
| `f87843f1edcab1e5e02defa61b8e560dcc001d92` | Adds P2-08 result; current committed provider HEAD (later untracked P2-09 proposal excluded). | Contained through merge `67a80a0c`; no uncommitted P2-05…08 candidate files remain; new P2-09 proposal is separate. |

The [P2-08 report](P2_08_ENDPOINT_SECURITY_CAPABILITY_RESULT.md) specifies a ten-file source manifest: sorted repository-relative filename + NUL + bytes + NUL. Independent recomputation:

| Candidate | Manifest SHA-256 | Meaning |
| --- | --- | --- |
| fe2edb00 / b7530494 | `fdc026906737f1a21b7132bb3dfca1d8ee59d864e472323bfe3272377fa56ab8` | Does not match tested P2-08 source. |
| Clean P2 f87843f1 | `2033486fd8a50bc67fd68fbb2225435ae0f4985613a7329a8ca4a8e6ef439e0e` | **Exact match to report.** Previously uncommitted P2-08 components are now committed. |
| Integrated main 70e9fa07 | `aaf51190f2f139654f3bdb7f81515b5e0c2f042b81db33ff9f2f614623324fea` | Nine source/test files match exactly; CMake intentionally includes additional Agent/P3 targets. This is a new merged build configuration, not the original standalone binary. |

The nine exact files are `core/include/sentinel/core/runtime/LocalInference.h`, `core/src/runtime/LocalInference.cpp`, `core/include/sentinel/core/runtime/OllamaRuntime.h`, `core/src/runtime/OllamaRuntime.cpp`, `core/src/model/ModelService.cpp`, `tests/support/ScriptedProviderTransport.h`, `tests/core/test_endpoint_security_capability.cpp`, `tests/core/test_provider_contracts.cpp` and `tests/core/test_local_inference.cpp`. The tenth is `tests/CMakeLists.txt`.

Main CMake contains the complete provider registrations once each, plus Agent harness/IPC definitions and P3 tests absent from standalone P2. Copying P2 CMake wholesale would lose **four** existing targets: `test_agent_reliability_harness`, `test_agent_reliability_ipc`, `test_model_lifecycle_ownership`, `test_local_model_runtime`, along with compiler/Git/IPC fixture definitions/dependencies.

Native fixture SHA-256 in main is `aeb488dacfc9bebb579b6843d18bcb4f7af6ab679a39dff210d232c3a8d0cefb`, matching the later P2-07 receipt. Vendor fixture is `c7b7a3b9467fd90f23bd26bfc7b93043f6f1b65c7776b745207b51c91f31696d`, also matching P2-07. P2-06 earlier shared-source/native-fixture hashes describe its older candidate; later documented P2-07/08 changes supersede those bytes without rewriting its report. P2-08 scripted transport SHA-256 is `64cba580674acbb0130d6c21162fc2c64d21521321bac2de36b5b71d996d6376`.

Dependency map: bounded IChatProvider contract -> adapter implementation/ModelService capability gates -> P1 bounded planner. Native Claude/Gemini fixtures use the LocalInferenceTransport seam and existing network authority; vendor fixtures build on the same seam. P2-08 extends GET/catalog transport and endpoint/redirect/capability guards; **fe2edb00 and e33db6de must be reviewed together**. No alternative provider router/registry is required. Generic HTTPS configurability, DNS/private-network policy and real-model compatibility remain acceptance gaps.

### Exact historical P2 union scope (current main-relative delta: zero)

- `core/CMakeLists.txt`.
- `core/include/sentinel/core/chat/OllamaChatProvider.h`.
- `core/include/sentinel/core/interfaces/IChatProvider.h`.
- `core/include/sentinel/core/model/ModelService.h`.
- `core/include/sentinel/core/runtime/LocalInference.h`.
- `core/include/sentinel/core/runtime/LocalInferenceTransport.h`.
- `core/include/sentinel/core/runtime/OllamaRuntime.h`.
- `core/src/chat/OllamaChatProvider.cpp`.
- `core/src/model/ModelService.cpp`.
- `core/src/runtime/LocalInference.cpp`.
- `core/src/runtime/OllamaRuntime.cpp`.
- `docs/audits/P2_05_BOUNDED_STREAMING_CONTRACT_RESULT.md`.
- `docs/audits/P2_06_NATIVE_CLOUD_PROVIDER_CONTRACT_RESULT.md`.
- `docs/audits/P2_07_VENDOR_CLOUD_CONTRACT_RESULT.md`.
- `docs/audits/P2_08_ENDPOINT_SECURITY_CAPABILITY_RESULT.md`.
- `tests/CMakeLists.txt`.
- `tests/core/test_bounded_streaming_contract.cpp`.
- `tests/core/test_endpoint_security_capability.cpp`.
- `tests/core/test_local_inference.cpp`.
- `tests/core/test_native_provider_contract.cpp`.
- `tests/core/test_provider_contracts.cpp`.
- `tests/core/test_vendor_cloud_contract.cpp`.
- `tests/support/DeterministicChatFixture.h`.
- `tests/support/ScriptedProviderTransport.h`.

## 5. P3-01 through P3-03 reconciliation

| Component | Where it actually resides now | Tested-candidate assessment |
| --- | --- | --- |
| P3-01 baseline ownership test + registration | `3863cdd5`, with report in `5d8ce5ba` | Preserved and contained in main. |
| P3-02 dependency seam | LocalModelRuntime header/cpp in `3863cdd5`, report in `5d8ce5ba` | Commit also contains in-progress P3-03; cannot be treated as a pure P3-02 import. |
| P3-03 selected identity, no implicit GGUF, health/revision fences | Already largely included in `3863cdd5` | Early commit lacks final destructor correction. |
| Destructor correction, formerly uncommitted | `090600555717e5326c1a1caa4d4f3a3c74ed9502` | 13-line diff; removes borrowed status callback from destruction, disconnects/fences/aborts owned probe. Now committed and merged. |
| P3-03 report, formerly untracked | `53d0032a456a14a517552fe4c3d57a879167b148` | Now committed and merged with correction; historical dirty-state wording is retained as historical evidence. |
| Remaining D4/D5b / D6 | Two QEXPECT_FAIL assertions / one QSKIP in current fixture | Still open; not hidden by clean Git or green target exit. |

[P3-03 reported input hashes](P3_03_LIFECYCLE_READINESS_REMEDIATION_RESULT.md) match current P3 branch **and main**:

| File | Current / reported SHA-256 |
| --- | --- |
| `apps/sentinel-daemon/service/LocalModelRuntime.h` | `34b8bac726fef3ffb327133a19bced301964190e072ffe765cd57b16b9cd991a` |
| `apps/sentinel-daemon/service/LocalModelRuntime.cpp` | `9490a57d4c242ad1fddf7fbad64f88b6c14852f5a0efad19f4ea37f3da17ff60` |
| `tests/desktop/test_local_model_runtime.cpp` | `bf1b716a84bde498608b5ca40db77af86d607d9fc541c0b0c53edb3a9a4e808a` |

At `3863cdd5` and `5d8ce5ba`, cpp hash is instead `918cf8053486ef32aa300947016c66aab6eb789ed3b412fef619ebad81099b1b`: those historical commits **cannot reproduce the tested source by themselves**. At current main none of the formerly missing uncommitted components is missing. Matching these three inputs is not proof of identical merged core library/binary or independent regression acceptance.

The commit subject containing “adopt” is not PD-D10 approval. The decision register still says Pending Decision; neither it nor the acceptance matrix changed between baseline 619e989c and current main. No resource lease, external process ownership, unload/removal fence or hard-stop authority is approved by this audit.

Exact historical P3 union scope (current main-relative delta: zero):

- `apps/sentinel-daemon/service/LocalModelRuntime.cpp`.
- `apps/sentinel-daemon/service/LocalModelRuntime.h`.
- `docs/audits/P3_01_MODEL_LIFECYCLE_OWNERSHIP_RESULT.md`.
- `docs/audits/P3_02_LIFECYCLE_OWNERSHIP_CONTRACT_RESULT.md`.
- `docs/audits/P3_03_LIFECYCLE_READINESS_REMEDIATION_RESULT.md`.
- `tests/CMakeLists.txt`.
- `tests/core/test_model_lifecycle_ownership.cpp`.
- `tests/desktop/test_local_model_runtime.cpp`.

## 6. SEC-01, P0/P1 and duplicate/conflict matrix

**SEC-01 minimal publication scope is exactly two files:** `docs/audits/P1_04_2_LINUX_CI_VALIDATION.md` and `docs/audits/SEC_01_HISTORICAL_PRIVACY_REMEDIATION_RESULT.md`. INT-02 candidate, SEC source, current local main and remote main all contain identical reviewed bytes: historical sanitized file `0ab8f7405e3264acb4b0c20e5687c3cc00a3015886e9882dc823bca2a1fa1d63`; receipt `3260c7e88a75a798e61de643b28eb98b9265036cbf625865eb30ba9c8ff362a4`. Current-tree privacy scan now passes with zero flagged files. No new SEC-01 PR is needed; both equivalent commit histories are already merged.

Historical exposure is not recalled: original blob hash `f00a8bcd35c4ad5d47d2ef4db070f4f9f3e0b85d86f4bad7209db34f3d67809d`, introduced by `d4704e39`, remains reachable in history. Historic retention/remediation/publication is a separate maintainer/security decision. The original identifier is neither printed nor put into this report.

| Overlap / duplicate | Verified finding | Integration rule |
| --- | --- | --- |
| SEC source vs INT-02 | Distinct commit IDs, identical two-file remediation bytes; both ancestors of main. | Preserve history, no repeat patch/PR. |
| Planner vs P2 bounded contract | Six bounded files match main byte-for-byte: IChatProvider.h, OllamaChatProvider.h/.cpp, bounded test, deterministic fixture, P2-05 report. LocalInference.h/.cpp and ModelService.cpp subsequently contain native/security additions. | No whole old worktree copy; it would downgrade later provider/security behavior. |
| Planner commit `2f5464d0` | 17-file accumulation includes earlier P2 error/catalog/tests and P1 corpus. P1 corpus `e6bb5ebd` and P2 earlier fixes already contained independently. | Do not import commit once per claimed workstream. Containment and final merged bytes, not subject, govern. |
| All CMake variants | Current provider/bounded/native/vendor/security/lifecycle/helper test registrations occur once each. Standalone P2 lacks four integrated Agent/P3 targets. | Preserve the main union, compiler/IPC oracle definitions and dependencies. No duplicate registration was found in main. |
| Old planner whole-tree vs main | Includes older docs, workflow, privacy and provider content; replacing tree would revert P0 corrections/SEC remediation and drop later P2/P3 work. | Historical worktree is reference evidence only. |
| P1-01.6 source vs planner | P1 source and its report precede COR-11 planner extension; both histories already merged. | Copying earlier harness/report over main could lose later streamed cancellation evidence. |
| P1-05 | Nine-file unmerged legacy change overlaps current CMake and CLI reference docs. | Prepare a new main-relative review scope only when authorized; preserve daemon/legacy distinction and all current registrations. Actual merge conflict simulation not performed. |
| Reports vs current state | INT-01/02 and P2/P3 reports retain previously dirty/failed/skipped states. Planner summary still repeats some deterministic gaps that its preserved P1-01.6 appendix says covered. | Append current context in this audit; do not rewrite historical findings or acceptance thresholds. |

P0 documentation/P0-03 recommendations and P1-04 workflow are already contained in main. Fourteen pending maintainer decisions remain separate from adopted product direction. P1-01 native tool-assembly skip and live/native capability evidence remain; AC-P1-01 is not Passed. P1-04 requires hosted 100-repeat evidence for both Unix matrix cells; explicit GitHub workflow-dispatch query returned **zero runs**. AC-P1-04 remains open.

## 7. Proposed integration/review groups and order

These are logical review groups, not permission to rewrite already merged history. Their current main-relative scope is zero unless explicitly listed otherwise. No duplicate PR should be opened for a contained group.

| Order / group | Current main-relative scope / committed status / remaining dirty files | Dependencies and review readiness | Focused target/evidence gate |
| --- | --- | --- | --- |
| 1 SEC-01 | Zero. Exact two-file scope above already contained twice; no dirty components. | Complete current-tree remediation; **no new draft PR**. Maintainer/security review historical exposure separately. | Privacy, editorial preservation, safe diff/link checks; no product acceptance claim. |
| 2 P2-05 | Zero; planner/P2 copies contained. Historical ten-file bounded scope preserved in reports. No dirty components. | Interface prerequisite for planner; review baseline once, no re-import. Provider/security owners. | bounded streaming, provider contracts, local inference, runtime provider, LLM runtime; Unsupported native/structured streaming remains explicit. |
| 3 P1-01.6/1.7 | Zero. e6bb5ebd/91640b24 and 2f5464d0/9cdbd883 contained. No dirty components. Core planner, harness and two report scopes already present. | Depends on bounded provider interface. Runtime/security/independent oracle reviewer; **review contained code**, no duplicate draft. | Agent reliability harness + real IPC, Agent/LLM runtime, daemon/Desktop IPC; required skip/live gaps prohibit AC-P1-01 closure. |
| 4 P2-06…08 | Zero. Historical 24-file union listed above; final e33db6de/f87843f1 and intermediate commits contained. No dirty P2-05…08 components; late untracked P2-09 proposal excluded. | Review complete sequence, never fe2edb00/b753 alone. Provider/network/credentials security reviewer. | native/vendor/endpoint security, bounded/provider/local inference, credential and ModelService regressions; mock API success does not close AC-P2-01/03. |
| 5 P3-01…03 | Zero. Historical eight-file union listed above; final 09060055/53d0032a contained. No dirty components. | Review readiness separately from unapproved PD-D10 ownership. Models/IPC/security and independent reviewer. | lifecycle ownership, helper, ProcessExecutor, model/runtime tests and daemon/Desktop IPC; two XFAILs and D6 skip preserved, AC-P3-02/P5-04 open. |
| 6 P1-05 | **Nine-file unmerged scope below**, one remote commit; no accessible local dirty checkout. | Ready for source/scope review; **not certified ready for draft publication or merge**. Needs current-baseline independent validation and CLI owner/maintainer PD-D05 review. | new legacy exit suite, existing LLM runtime and full suite; old report has 127/128 regression result and independent/headless evidence gaps. |
| 7 P1-04 hosted stress | No new source scope; existing manual workflow contained. | Execution blocked pending explicit maintainer dispatch authorization and reviewed immutable candidate. IPC/security reviewer examines all attempts. | 100 sequential unfiltered sentinel-ipc runs per Ubuntu/macOS cell; exact SHA, versions, logs, timeout/exit, both complete cells. |

P1-05 exact proposed source scope:

- `apps/sentinel-cli/CMakeLists.txt`.
- `apps/sentinel-cli/commands/ChatCommand.cpp`.
- `apps/sentinel-cli/commands/ExitCodes.h`.
- `apps/sentinel-cli/commands/ModelCommand.cpp`.
- `apps/sentinel-cli/main.cpp`.
- `docs/audits/P1_05_LEGACY_CLI_EXIT_SEMANTICS_RESULT.md`.
- `docs/reference/CLI_REFERENCE.md`.
- `tests/CMakeLists.txt`.
- `tests/integration/test_legacy_cli_exit_codes.cpp`.

Do not cherry-pick/split/rebase accumulated commits as part of this audit. If maintainers require retrospective separate PR groups, first authorize explicit frozen main-relative candidates on an agreed base; rewriting main or already published history is not authorized. The safe next code proposal is the remaining legacy scope or a newly scoped missing-policy fix, not replaying P2/P3.

## 8. CI, reviewer and acceptance gates

This audit ran no product builds, tests or generated source writes in existing worktrees. Prior P2-08 132/132, P3-03 132/132 sequential/parallel, and planner 131/131 are source-reported evidence for their own builds. They are **not** new evidence for the complete merged main configuration. Do not add or compare totals as though configurations were identical.

For a future authorized frozen candidate, discover actual targets and use:

```sh
cmake --preset tests
cmake --build --preset tests
ctest --test-dir build/tests -N
QT_QPA_PLATFORM=offscreen ctest --preset tests --output-on-failure
QT_QPA_PLATFORM=offscreen ctest --preset tests --output-on-failure -j6
python3 tools/ipc/generate.py --check
python3 tools/ipc/generate_desktop.py --check
python3 tools/analysis/check_personal_paths.py
git diff --check
```

Run appropriate changed-line clang-format 18.1.8 and scoped clang-tidy, plus Linux ASan/QML/desktop quality checks for affected areas. From `cli/`, preserve Rust CI commands `cargo +1.94.0 fmt --all --check`, `cargo +1.94.0 clippy --locked --all-targets -- -D warnings`, `cargo +1.94.0 test --locked`. No baseline skip or timeout may be silently weakened. Retain P3 intermittent nativeDiagnostics(content-usage) history; passing reruns are not a root-cause fix.

Require candidate-specific Linux Ubuntu/Qt 6.11, macOS 14/Qt 6.11 and Windows x64/MSVC/Qt 6.11.3 CI. Experimental Windows ARM64 remains nonblocking/unverified. Rust hosted validation is Unix; Windows named-pipe limitations remain. Fedora KDE/native backend behavior needs its own authorized evidence. Full source/privacy/credential review must exclude ignored build outputs, personal profiles, credentials and private raw logs.

GitHub open dependencies observed: [draft PR #17 CI stabilization](https://github.com/sentinel-projects/sentinel/pull/17); dependency PRs #8 and #12–16. None was imported by this audit. CODEOWNERS names `@Sopwit`; branch API reports protected=false and repository rulesets returned empty. These do not authorize bypassing maintainer/security/independent review; PD-D11 reviewer capacity remains pending.

At the observed `70e9fa07` GitHub snapshot, [Rust CI](https://github.com/sentinel-projects/sentinel/actions/runs/38957943863) succeeded; [Linux](https://github.com/sentinel-projects/sentinel/actions/runs/38957943894), [Windows](https://github.com/sentinel-projects/sentinel/actions/runs/38957943785), [macOS](https://github.com/sentinel-projects/sentinel/actions/runs/38957943786) and [CodeQL](https://github.com/sentinel-projects/sentinel/actions/runs/38957944070) were still in progress. These are ordinary push runs, not the required hosted stress execution. This is a dated snapshot, not ongoing monitoring or a final green certification.

## 9. Audit validation, blockers and next safe action

Performed safe read-only Git/remote/API, file SHA-256, report-manifest, CMake declaration, dirty/untracked/ignored and source-preservation checks. Repository personal-path scan in this clean audit baseline passed (zero findings). Report links, Markdown structure, whitespace and changed-file privacy are checked before delivery. No production build/test was run and no CI workflow triggered.

Immediate blockers/risks:

- Provider source also gained an active untracked P2-09 policy plan; do not sweep future work into an already validated provider snapshot. Main publication moved while inspected: re-resolve local/tracking/live SHA and source manifests before any next action. At the recorded final observation they agree at 70e9fa07.
- Historical intermediate P2/P3 commits are incomplete tested candidates; use their final corrections/reports. At current main there are **no missing uncommitted P2/P3 components**.
- Current merged candidate differs from standalone provider CMake/binaries; await complete candidate-specific cross-platform CI and independent evidence rather than inheriting old green totals.
- No hosted P1-04 stress run, native tool-assembly evidence, full live provider acceptance or approved PD-D10 lease/unload policy exists. AC-P1-01/P2-01/P3-02/P5-04 are not closed by containment.
- Legacy P1-05 remains unmerged and needs fresh independent/current-main regression. Historical sensitive Git objects remain a separate publication/retention concern despite the now-clean current tree.

**Next safe action:** maintainer review this reconciliation and the complete final main diff/evidence; wait for current-SHA CI completion, assign security/independent reviewers and separately authorize hosted stress. If a new code PR is desired, prepare only the nine-file main-relative P1-05 candidate in a separately authorized workspace after fresh validation. Do not open duplicate SEC/P2/P3 PRs or publish intermediate commit subsets.

**Readiness:** this audit is ready for review. No new code PR is certified ready for publication/merge authorization by this read-only task. P1-05 is the only identified unmerged code scope and is ready for scoped source review, with explicit validation gaps. Existing main code remains subject to pending CI/acceptance and policy gates. Only this new audit report is written; no commit, push, merge, staging, rebase or PR creation was performed.
