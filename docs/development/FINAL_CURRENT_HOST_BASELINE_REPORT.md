# SENTINEL — FINAL CURRENT-HOST BASELINE REPORT

Current host: macOS, Qt 6.11.2, 2026-10-03. This report certifies the **uncommitted working tree**, not a clean commit, release package, or cross-platform release. No model download, new provider/tool/UI, architecture change, or permission bypass was introduced.

Evidence is retained locally in `build/certification/final-current-host/`: initial/final status, configure/build/lint logs, complete QtTest output, suite counts, real-provider JSONL traces, desktop AX snapshots, persistence summaries, cycles and degraded-startup results. Failed runtime trials are retained alongside successful ones. Personal-Brain CLI was unavailable; the explicitly supplied workspace and repository architecture/security instructions were used.

## 1. Build / Static Gates

- Configure: **PASS**, `cmake --preset no-ccache`.
- Compile/link/QML compilation: **PASS**, `cmake --build --preset no-ccache -j4`, including the final format-only rebuild. Zero compiler diagnostic warnings/errors in recorded build logs.
- QML lint: **PASS**, `sentinel-desktop_qmllint`; 736 warning occurrences, zero errors. Previous certification also recorded 736. This is not warning-free.
- Format: **PASS** on tracked changed lines using the existing `clang-format-diff.py` gate, against HEAD and HEAD's parent. Host LLVM 23.1.2 was used; CI pins 18.1.8, whose exact-version run was not available. Small formatting discrepancies were corrected without behavior changes. The extended opt-in probe was formatted separately.
- clang-tidy: **PASS with warnings**, 236 production source files, all exit 0, 3,272 emitted diagnostic warning occurrences, zero diagnostic errors; existing `WarningsAsErrors: ''` policy retained. Initial direct analysis failed because LLVM 23 cannot read Apple Clang's PCH. Retry used a temporary compilation database without binary PCH and with the actual Xcode SDK/libc++ include paths; production build configuration was unchanged.
- Metadata: actual bundle Info.plist, privacy manifest and entitlements pass `plutil`; SDEF is well-formed XML. Linux AppStream/desktop validators are absent on this macOS host: **NOT RUN / environment**. An initial attempt to parse SDEF as a plist was corrected to XML validation; it was a validator-selection mistake, not invalid metadata.
- Documentation/reference integrity: no existing standalone gate found; no new lint policy introduced.
- `git diff --check`: **PASS**.

## 2. Automated Regression

Fresh-build initial run: **108/108 registered suites passed**, 125.09 s. Final run after formatting/rebuild: **108/108 passed**, **0 failed**, 92.47 s. QtTest case totals: **1,109 passed, 0 failed, 6 skipped**, including initialization/cleanup cases. Skips are not counted as passed cases.

- ApplicationController: **127 / 0 / 0**, unchanged.
- DesktopShell: **74 / 0 / 0**, unchanged. No Keychain authorization blocked the suite.
- Native credential store: **10 / 0 / 0**.
- MCP integration: **7 / 0 / 0**.
- Plugin tool integration: **10 / 0 / 0**.
- Skill/extension runtime: **11 / 0 / 0**.
- Settings/recovery: **16 / 0 / 0**.
- Persistence/startup: **7 / 0 / 0**.
- Unified audio lifecycle: **11 / 0 / 0**.

Exact skips:

| Test case | Exact reason / classification |
| --- | --- |
| `AgentRuntimeTest::asyncRunCommandStreamsAndContinues` | Shell fixture requires child creation; strict macOS plans deny forks. Platform/intentional containment incompatibility. |
| `AgentRuntimeTest::asyncRunCommandCancellationAndTimeout` | Shell fixture requires child creation; macOS strict no-detached-child plans deny all forks. Native host cancellation separately covered. Platform/intentional. |
| `AgentRuntimeTest::asyncDockerUsesProcessExecutorAndPreservesRestrictions` | Fake Docker shell fixture requires child creation denied by strict macOS plan. Platform/intentional. |
| `AgentRuntimeTest::shutdownStopsActiveCommand` | Shell fixture requires child creation denied by strict macOS plan. Native host shutdown separately covered. Platform/intentional. |
| `RealToolExecutorToolsTest::runCommandDockerSandboxReportsMissingDocker` | Docker is installed; missing-Docker branch cannot run. Environmental. |
| `RealToolExecutorToolsTest::browserToolsReportMissingNodeGracefully` | npx is installed; missing-Node branch cannot run. Environmental. |

No new certification test was skipped and no registered test count was reduced.

## 3. Chat Runtime

Provider: existing local llama.cpp, alias `sentinel-nemotron`, NVIDIA Nemotron-3 Nano 4B Q4_K_M, loopback 8081, 8192 context, Jinja tools, reasoning off. No silent provider/model switch.

- First turn: **PASS**, actual production controller returned `MAVI`, Completed. Native desktop also displayed `MAVI` and restored input/model controls after generation.
- Streaming: **PASS**, controller observed 11 updates for `12345678910`; independent real streaming request produced 29 deltas and `1, 2, 3, 4, 5, 6, 7, 8, 9, 10`.
- Cancellation: **PASS**, Stop after first real content update returned true and terminal status Cancelled. Partial text remained.
- Post-cancel: **PASS**, next request returned `DEVAM`, Completed; cancelled row still Cancelled after the next generation, with no late replacement by Completed.
- Persistence: production SQLite conversation has nine unique message IDs: system plus four user/assistant pairs. One final assistant row per request, cancelled row preserved. Native desktop profile contains three unique rows and one completed assistant `MAVI`.

Harness correction: a first probe mistakenly let builder defaults overwrite its ModelService endpoint; it failed honestly before usable text. The probe now configures the controller endpoint and uses production SQLite defaults. No product change was needed. A separate earlier count request repeated MAVI; it was not counted as correct streaming content.

## 4. Agent Runtime

Real `ModelService`/ModelBinding → `LlmAgentRuntime` → `AgentRuntime`/AgentLoop → common gateway → real tools → observed evidence → accepted final was exercised.

- Filesystem: **PASS**, requested Turkish workspace listing; actual complete nonrecursive `list-directory` observation, evidence count 1, grounded final, Completed.
- Read: **PASS**, actual CMakeLists.txt line 15 `project(Sentinel`; final project name Sentinel, Completed, evidence count 1.
- Approval/security: the production static approval policy evaluated these low-risk read operations as NotRequired. No persistent grant was created. Higher-risk Ask/Deny/validation branches passed owner suites; this trace does not claim a manual approval dialog occurred.
- Cancellation: real in-flight planning request ended Cancelled. Runtime recovery was exercised in a new session in the same runtime; reusing a terminal session was a probe mistake, corrected without product changes.
- Recovery quality: multiple follow-up model trials ended Failed or Stuck, with outside-workspace guesses correctly Denied. One post-cancel listing reached Completed but shortened observed `:memory:.ses` to `.ses`; that trial is **not counted as an exact grounded-file-list success**. These outcomes are retained in the evidence and prevent claiming uniformly reliable default-sampling Agent answers.

Final deterministic-sampling cancellation/read: **PASS**, same explicit model/runtime with server flags `--temp 0 --seed 42`; real Cancelled → new session → actual CMake read → correct Sentinel → Completed. The final terminal audit confirms the original session remains **Cancelled** after that follow-up; no late Completed. This gate is qualified by explicit sampling; the default-sampling failures are retained as a model reliability limitation. No grounding or permission gate was relaxed to obtain completion.

## 5. Tool / Security

**Automated invariant checks PASS**, plus actual runtime boundary evidence above:

- Registry descriptors and argument schemas remain authoritative; validator/gateway execute before handlers.
- Filesystem uses scoped service/authorization; outside-workspace model guesses were Denied, never executed.
- Tool completion remains an observation; missing/invalid final decisions ended Failed/Stuck rather than being promoted by tool success.
- Active ModelBinding freeze/no provider or model fallback: owner tests pass; actual smokes retained explicit provider/model selection.
- Skill instructions do not grant permissions; malicious-skill regression still reaches approval and creates no grant.
- Plugin direct filesystem/network/fork/spawn access remains denied by actual macOS containment tests; broker authorization remains mandatory.
- MCP results remain untrusted external observations and require gateway/schema/approval handling.
- Filesystem negative claims require complete scope evidence in existing claim/grounding tests; no new negative claim was inferred from absent/incomplete data.
- Success still requires accepted AgentLoop FinalAnswer. Semantic accuracy of arbitrary model prose is limited; the filename mismatch above is disclosed rather than treated as proof of exact fact verification.

No permission/sandbox/no-fallback invariant regression was observed. Active-run **network-mode** freeze remains the distinct known residual, not confused with ModelBinding freeze.

## 6. MCP

- Discovery/direct: **PASS**, actual existing deterministic local server in `test_mcp_integration`; registry discovery, echo `ECHO: MERHABA`, add(2,3) structured sum 5, argument validation and denied/unavailable paths.
- Real Agent → MCP: **PASS**, actual `mcp.certification.add` output 5 → provider evidence → real continuation → `The result is 5.` → Completed.
- Remote HTTP and some manual desktop MCP flows remain unvalidated.

## 7. Plugin / Skills

- Plugin: **PASS** in real integration tests: separate host PID, required sandbox, sample echo, arithmetic fixture add, unload, crash/timeout/cancel and broker-denial/isolation cases.
- Agent → Plugin: **PASS**, real sample echo result `MERHABA`, provider evidence, accepted Completed; explicit unload accepted and host stopped. `stopPlugin` only stops plugin activity and deliberately keeps the initialized host; the probe was corrected to call `unloadPlugin` for process teardown.
- Some teardown paths print `QProcess: Destroyed while process ... still running`; 14 such warnings occurred in the full suite. Exact process checks after teardown found no remaining host. This report does not hide those diagnostics or claim abrupt-parent-death guarantees.
- Skill context: **PASS**, deterministic `[SKILL_OK]` instructions appear in real bound-provider planning/continuation when enabled and are absent when disabled; initial intent-classification requests are separate from skill-bearing planning. Both optional real skill tasks failed to produce accepted final answers, so their context evidence is not new real Agent Completed evidence. Owner scope/budget/permission tests pass.

## 8. Settings / Onboarding

Native fresh disposable profile: first-run seven-step onboarding observed, finished, normal Quit, relaunch without onboarding. Theme Dracula, language Turkish, workspace Coding, llama.cpp endpoint/provider and `sentinel-nemotron` selected through UI and preserved across three normal cycles.

An automation typing attempt dropped the endpoint's final digit; atomic paste verified the complete value. It is recorded as an input-harness retry, not used to claim endpoint correctness before verification. Owner tests separately cover endpoint disk restore, settings failures and first-run/resumed onboarding.

Localization smoke: Turkish title/controls changed in the real desktop. Full TR localization remains incomplete; English strings were observed. No complete localization claim.

## 9. Secrets / Network

- Keychain: **PASS**, existing native test writes/reads/deletes a unique certification key, no skip; no secret value printed and no interactive authorization observed.
- Online / Local Only / Offline: **PASS**, real local bound-model requests each returned `NETWORK_OK`. Cloud policy Allowed only Online; LocalOnly/Offline rejected external targets. Existing actual-transport test proves blocked cloud requests fail before transport, HTTP status 0 and no local fallback.
- No successful secondary cloud credential/request matrix is claimed. Existing user's cloud secrets were not copied into disposable settings or logs.

## 10. Backup / Recovery

**PASS**, production SettingsRecovery fixture creates file-backed Chat/memory/settings, exports a disk backup, mutates state, restores supported domains and verifies returned values. Chat merge semantics are preserved, credentials excluded, malformed archive rejected without mutating restored data.

Forced termination: owned isolated desktop was killed by the bounded harness after UI automation could not reattach. Sample showed ordinary native event-loop/window-update processing, not synchronous startup network wait. Completed MAVI and all stores survived, then LaunchServices relaunch restored the desktop. This particular force-kill was idle, with no active run. Current full suite's production-store child-kill regression independently verifies Interrupted active Chat/Agent metadata, never false Completed. Prior phase's real interrupted desktop request evidence remains baseline, not re-counted as a new live active-run force-kill here.

## 11. Persistence

- Normal restart: **PASS**, native MAVI and representative settings/workspace restored.
- Store isolation: **PASS**, deterministic clear-memory leaves Chat; clear-Chat leaves memory; Agent metadata and Chat have separate stores. Full suite also covers retention/concurrency/checkpoint/reopen.
- Duplicates/phantoms: three native rows remain three over three cycles; no blank phantom assistant was observed. Recovered interrupted/cancelled presentation tests pass.
- Every inspected DB returned integrity `ok`; no data deletion was used as recovery.

## 12. Startup

- Repeated quick gate: **3/3 normal launch → observed restored main window → normal UI Quit**, all exit 0, restored settings/row counts intact. Earlier ten-cycle baseline retained. No startup/lifecycle behavior was changed in this final phase, so a new ten-cycle run was not required.
- Four degraded profiles: provider unavailable, configured unavailable MCP, broken plugin fixture, absent speech runtime; each rendered core desktop and exited normally, with all DB integrity checks `ok`.
- MCP/plugin fixture-presence startup smoke does not prove activated failure UI in those exact profiles. Real activation/failure isolation is covered by current owner runtime tests and real Agent smokes.
- Responsiveness: native Chat busy/ready, onboarding/settings and restart flows responded. A failed direct-subprocess AX reattachment was isolated by stack sampling; switching launch method to LaunchServices allowed all three normal cycles. Failed automation attempt not counted as a passed cycle.
- Cycle durations include human/automation inspection, not formal launch latency benchmarks. Prior slow-discovery fix remains covered by the final readiness-getter regression.

## 13. Process / Filesystem Hygiene

Initial process inventory had no pre-existing provider/desktop/plugin/MCP service to stop. Exact certification-owned provider and probe processes were tracked; user services were never killed. During AX retries the UI tool transparently launched an extra desktop outside the isolated launch environment; that session-owned duplicate was stopped with TERM. Only disposable-profile desktops were forcibly killed. No user data was deleted, but zero incidental normal-startup writes by that duplicate cannot be asserted. Final cleanup results are recorded in the decision section.

Intentional retained artifacts: build outputs and evidence under ignored `build/`; uncommitted production/test/documentation files are reviewable source, not disposable artifacts. Prior root `:memory:.ses` (timestamp/session-ID text, present before this gate) was moved to ignored evidence `prior-session-temp.ses`; it is no longer left in source root. Temporary profiles/scripts are removed only after evidence preservation and process exit.

Initial status: 57 tracked modifications and 21 untracked entries. Final formatting added eight tracked paths to the dirty list; their changes are cosmetic. The starting tree already includes provider/native-Agent/Gemini, MCP evidence, plugin broker/containment, skill context, settings/recovery, permissions and audio/startup fixes. Tests ran on all of them. No clean-commit baseline claim is made.

Classification: production fixes belong to earlier certification phases; final phase adds format-only corrections, extends the opt-in runtime probe and adds this report. Existing regression tests/fixtures and earlier reports remain intentionally untracked. Initial/final status logs enumerate exact paths; generated/temp artifacts are kept out of source directories.

## 14. Defects Found In Final Gate

No new deterministic production implementation defect was proven/fixed in this phase. Formatting gate discrepancies were corrected, and the existing opt-in test probe gained bounded Chat cancellation/streaming, new-session Agent recovery, network-mode diagnostics and explicit plugin unload. Harness endpoint, terminal-session reuse, LaunchServices attachment, stop/unload and SDEF-validator mistakes were corrected; failed evidence retained.

Model-origin errors were observable: home-path guessing, repeated tools, inability to produce grounded final, and one inaccurate filename in an accepted listing. Their implications are included in the final decision, not hidden behind the automated suite PASS.

## 15. Known Non-Blocking Residuals

Retained backlog: incomplete full Turkish localization; active-run network-mode freeze not fully validated; failed legacy plaintext credential migration debt; plugin credential-positive path; abrupt plugin-parent-death guarantee; remote HTTP MCP; some manual desktop MCP flows; LM Studio Agent model/runtime limitations; some cloud conversation/cancel paths; microphone permission denied; Whisper/Piper sandbox dependency paths blocked; Kokoro CLI absent; Voice Chat/Voice Agent not runtime-certified; WAL/SHM corruption unvalidated.

Additional observed limits: default-sampling Agent answer reliability/exact filename reproduction, numerous existing lint warnings, exact CI formatter-version unavailable, and UI attachment sensitivity when multiple build bundles share the same bundle ID. Unavailable audio and these environment limitations were not automatically classified as core failures.

## 16. Blocking Issues / Final Decision

No unresolved implementation blocker was demonstrated in the exercised minimum gate. Build/test/startup/integrity/security boundaries passed; the required real Chat, filesystem/read Agent, cancellation/recovery, MCP, plugin, skill-context and state-lifecycle gates were exercised. Existing audio/environment gaps did not turn into startup/data-integrity/security failures.

**FINAL DECISION: CURRENT-HOST BASELINE READY**

**Reason:** the current uncommitted working tree meets the requested current-host development-baseline minimum under the explicitly tested runtime configuration. The final real Agent cancellation/read gate uses the same existing `sentinel-nemotron` model with **temperature 0 / seed 42**, accepted correct grounded Completed, and direct confirmation that the previous session stays Cancelled. This decision does **not** certify uniformly reliable Agent generation at default sampling, arbitrary model prose accuracy, a clean commit, production packaging, or the known residuals.

Final process inventory: **no owned provider, desktop, plugin host, MCP server or probe remains**; only the inventory command itself matched. No speech runtime was left running. Fifteen existing session-owned temporary paths were removed, including profiles/scripts from this gate and the prior audio/startup disposable profile after evidence preservation. Credentials created by the native Keychain test were deleted by its verified cleanup. Intentional build evidence and source fixtures/reports remain. Final `git diff --check`: **PASS**; tracked changed-line format diff: **empty/PASS**.

No startup/lifecycle behavior was changed during this final gate; three new normal cycles plus the prior ten-cycle certification remain the repeated-startup evidence. All failed model/automation trials are disclosed above and retained in local evidence. The model accuracy issue in one listing is a residual, not a claimed exact grounding success; correct filesystem/read finals supply the required successful runtime evidence.
