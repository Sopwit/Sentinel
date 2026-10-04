# SENTINEL — PHASE 2 DAEMON / IPC / CLI / TUI REPORT

Date: 2026-10-04. Host: macOS arm64, Qt 6.11.2; Rust 1.94.0 verified. Base: `db9eb38` on main. Results below include the uncommitted Phase 2 implementation. The previous current-host baseline is user-provided; this report records fresh verification rather than inheriting runtime PASS claims.

## 1. Architecture

- **Daemon:** existing `sentinel-daemon` expanded into a `QCoreApplication` headless runtime, without QML/UI construction.
- **Shared runtime:** reuse `ApplicationControllerBuilder::withStandardDefaults`; existing AgentRuntime, ChatModeService, ModelService, registry/gateway, permissions, sandbox, stores and optional extension services remain authoritative. Only ordinary C++ runtime/approval accessors and an optional path-provider profile override were added to core.
- **Desktop relationship:** certified direct path retained. Daemon has an isolated profile; Desktop and daemon are not yet one shared live authority. No large runtime refactor, new provider/tool, packaging or new Agent features were introduced.

## 2. IPC

- **Transport/socket:** Qt local server and Rust UnixStream, default `~/.sentinel/run/daemon.sock`; custom private directories supported.
- **Security:** owner-only directory/socket, UID verification (`getpeereid` on this host), lock before stores initialize; preserve regular files/live endpoints and probe stale same-owner sockets before recovery. No TCP exposure.
- **Protocol/version:** 1.0, distinct from canonical app version `1.0.0-rc.8`; handshake requires identity, versions and capabilities. Major mismatch rejected.
- **Contract:** `protocol/ipc-v1.json` generates C++ validation/constants and Rust typed command/result/event enums. Deterministic drift check registered in CTest and CI. Nested resource/model/session collections retain JSON values in this foundation; the defined outer payload fields are typed.

## 3. Messages

Requests: hello, daemon.status/shutdown, model.list/current, session.list/create/attach, chat.send, agent.start, run.cancel, approval.respond. Responses correlate request ID/name. ProtocolError covers malformed/invalid/unknown states. Streaming events: run.started, output.delta, tool.requested, approval.requested, tool.result, run.completed/failed/cancelled.

Approvals remain daemon/core decisions with one-use approval IDs, run/state validation and no permanent grant. Typed DTO drift, invalid booleans/envelopes, incompatible major, unknown sessions and replayed approvals are tested.

## 4. Daemon Lifecycle

Startup, endpoint collision prevention, stale Unix socket recovery, IPC shutdown and SIGTERM shutdown pass. Controller/services/stores settle before endpoint lock release. Optional provider unavailability does not stop startup. MCP unavailable configuration and absent plugin/speech runtimes permit status/readiness; provider requests fail truthfully. A deliberately broken installed plugin daemon-start fixture was not separately exercised.

## 5. Sessions

Create/list/attach use the existing ConversationStore. Multiple clients subscribe independently. Disconnect does not cancel; reconnect returns bounded current state/output/pending approval. There is no new session database/event-log service. One active foreground run reflects current controller ownership. Run IDs are ephemeral; restart restores persisted records without resuming execution or replaying historical events. Historical provider/model values remain unknown if the legacy stored message omitted them.

## 6. Chat Through Daemon

Deterministic Chat streaming, final text and cancellation pass. Real owned llama.cpp server with `sentinel-nemotron` returns exactly **MAVI** through CLI. JSON returns completed/MAVI. Real SIGINT produces cancelled exit 2, no late Completed; subsequent Chat returns MAVI. Killing the owned CLI leaves the daemon and active Chat alive; a reconnecting control client cancels it and a new Chat succeeds.

## 7. Agent Through Daemon

Provider/model: existing llama-cpp-server / sentinel-nemotron, using the existing local GGUF. Deterministic core Agent loop passes tool → explicit approval → observation → accepted final; IPC terminal text equals the actual core final rather than the last delta. Real filesystem Agent requests list-directory, receives explicit terminal approval, executes the actual workspace-root listing and reaches core Completed with observed entries.

**Grounding limitation found:** the real final additionally said “No hidden entries were found” although the structured observation had `includeHidden:false`. Existing `ClaimGroundingResolver` protects structured absence assertions, but unstructured extra narrative claims in a generic listing final are not exhaustively checked. The transport faithfully forwards core's accepted answer; this is not evidence that the hidden-file absence assertion is valid. No blanket grounding/security certification is given for that final. The same wording appeared in earlier Phase 2 evidence; root cause is the existing core final-claim coverage, not an IPC permission bypass. A semantic grounding-policy change belongs in its owning core layer with dedicated regression and runtime revalidation; it was not disguised as a transport fix.

Real stdin Agent greeting also fails truthfully (“Agent could not determine a grounded next action”, exit 1); stdin transport succeeds in deterministic tests. Model-specific failure is not mapped to successful completion.

Real CLI Agent cancellation while awaiting approval returns **exit 2 / run.cancelled**, with no late Completed. The next Agent reads the actual `CMakeLists.txt` through read-file and returns exactly **Sentinel**, core Completed / exit 0. Subsequent daemon status reports zero active runs.

## 8. Rust IPC

Reusable client performs handshake, bounded framing, typed DTO validation, request correlation, bounded event buffering and explicit error mapping. CLI/TUI share it and never link core. Rust 1.94 builds successfully. Connection failure, malformed/oversized payload, unknown event, typed request/event validation, correlation/queue limits and incompatibility are covered. Windows client currently exposes truthful unavailable/unsupported transport; Windows runtime is not certified.

## 9. CLI

Status/models/sessions/chat/agent/run, stdin, JSON/stream-json, explicit socket, version, shutdown and attach/TUI entry points implemented. Explicit task wins over stdin. Text prints final once; machine output remains structured. Approval uses user y/N, non-TTY denies. SIGINT uses an independent control connection, including while approval input waits. Exit codes: 0 success, 1 failure, 2 cancelled, 3 unavailable/disconnected, 4 incompatible. Actual binary tests cover all five mappings and text/JSON/stream/stdin. No implicit daemon spawning.

## 10. TUI

Real terminal PTY launches Ratatui and exercises Chat incremental output/completed, cancellation, Agent approval/Completed, and clean exit. Session navigation attaches snapshots; composer/status/provider/model/version and approval modal are present. Enter sends, `/agent` selects Agent, Up/Down navigate, Ctrl+C cancels/exits idle, y/n/Esc decide approval. Terminal final replaces streamed content. Empty session handling creates a conversation; IPC errors/disconnects are visible. Foundation rendering only; no final design polish or Windows terminal certification.

## 11. Security

Existing ToolRegistry, ToolArgumentValidator, ToolExecutionGateway, permissions, sandbox, frozen core ModelBinding, provider routing and accepted-core-final requirement remain intact. No IPC grant/provider mutation bypass added. Same-user socket and endpoint collision checks tested; an actual second-UID connection attempt was not performed on this host. Frame size 262144 bytes, per-client output queue 1048576 bytes, 32 clients; client event queue bounded. Slow/oversized clients disconnect without changing core terminal state. A sustained throughput/backpressure stress benchmark was not performed. The narrative grounding issue in section 7 prevents a blanket PASS for all invariants.

## 12. Resilience

Two concurrent deterministic clients, disconnect/reconnect attach and client crash pass. Real owned CLI kill leaves active daemon work intact. Graceful daemon shutdown gives truthful CLI unavailable exit 3. Restart restores conversations. Owned daemon SIGKILL followed by startup recovers stale endpoint/lock and preserved completed records; SIGTERM removes endpoint cleanly. No durable active-run resumption is claimed.

## 13. Desktop Compatibility

Existing in-process Desktop path remains; ApplicationController 127/0/0 and DesktopShell 74/0/0 pass. Stage A is daemon + Rust clients with isolated profile. Stage B adds Desktop IPC adapter and explicit profile ownership/migration. Stage C retires direct Desktop runtime and legacy C++ CLI after equivalent approval/recovery/lifecycle regression. Desktop migration was explicitly outside Phase 2 PASS requirements.

## 14. Regression

- Configure/build/link/QML compilation: PASS, no-ccache preset. Final incremental compilation: **0 compiler warnings, 0 errors**.
- Full registered CTest: **109/109 suite PASS, 0 FAIL, 0 suite SKIP** (baseline108 + canonical-contract gate).
- ApplicationController: **127 / 0 / 0**. DesktopShell: **74 / 0 / 0**.
- Daemon IPC: **21 / 0 / 0 QtTest cases**, including setup/cleanup. Profile override targeted test passes and preserves old default paths when cleared.
- Rust: **16 tests PASS** (8 IPC + 8 actual CLI tests), 0 failed/ignored. Rust **1.94.0** build, `cargo fmt --all --check`, and `cargo clippy --all-targets -- -D warnings` all PASS.
- QML lint: PASS; **736 existing QML warnings**, no QML sources changed.
- Changed-line clang-format: PASS; generated C++ header is explicitly excluded from formatting by generator markers.
- clang-tidy: three changed daemon production sources PASS, **0 emitted source warnings/errors after fixes**. Homebrew LLVM cannot consume Apple's PCH; checks use the build commands with PCH removed and explicit current SDK includes. Earlier diagnostic/PCH failures are not counted as successful checks. Scope is these three files, not a full-repo tidy claim.
- Canonical generation/reference checks and git diff --check: PASS.
- AppStream tools unavailable on current macOS host; Linux-only metadata gate was not run.

**Six test-case SKIPs inside otherwise passing suites (not counted as case PASS):**

| Exact case | Exact reason | Class |
|---|---|---|
| AgentRuntimeTest::asyncRunCommandStreamsAndContinues | This shell fixture requires child creation; strict macOS plans deny forks. Native host lifecycle is covered by plugin integration tests. | Platform/security fixture |
| AgentRuntimeTest::asyncRunCommandCancellationAndTimeout | This shell fixture requires child creation; macOS strict no-detached-child plans now deny all forks. Plugin host cancellation is covered by real host integration tests. | Platform/security fixture |
| AgentRuntimeTest::asyncDockerUsesProcessExecutorAndPreservesRestrictions | The fake Docker shell fixture requires child creation; strict macOS plans deny forks. | Platform/security fixture |
| AgentRuntimeTest::shutdownStopsActiveCommand | This shell fixture requires child creation; strict macOS plans deny forks. Native host shutdown is covered by plugin integration tests. | Platform/security fixture |
| RealToolExecutorToolsTest::runCommandDockerSandboxReportsMissingDocker | docker is installed on this machine; the missing-docker branch cannot run. | Environmental |
| RealToolExecutorToolsTest::browserToolsReportMissingNodeGracefully | npx is installed on this machine; the missing-node branch cannot run. | Environmental |

## 15. Defects Found

1. IPC Agent terminal mapping initially expected AgentRunEvent while actual completion used AgentTextEvent. Fixed in adapter; deterministic exact final-text regression and real Agent terminal output pass.
2. New test application's long name made temporary socket paths exceed macOS sockaddr_un. Fixed private short fixture prefix; stale socket and full suite pass.
3. Two added tests retained QJsonValueRef into a temporary JSON object and crashed. macOS crash stack identifies the fixture; copy session ID as QString. Disconnect/approval cancellation and full suite pass.
4. Clang-tidy warnings in new daemon code (missing braces, signed permission mask, narrowed newline offset) fixed; fresh scoped tidy is clean.
5. Generic Agent narrative can add an unsupported hidden-file absence claim: **OPEN**, section7. No unsupported claim is certified as grounded. Do not label overall Phase 2 fully PASS while this remains.

Rust tests initially run under Codex's restricted sandbox failed local socket bind with EPERM; rerun with authorized local IPC execution passes. This was environmental, not silently skipped. Real-provider fixture alias and PTY harness timing/EOF issues were corrected during verification and did not require product fallback behavior.

## 16. Remaining Work

- Close and regression-test the core narrative grounding coverage defect before unconditional Phase 2 certification.
- Actual second-user socket rejection, broken-plugin startup fixture and sustained backpressure stress remain unvalidated; implementation boundaries/limits are present.
- Windows Rust transport remains a stub; current-host macOS results do not imply cross-platform runtime PASS.
- No historical event replay, durable ephemeral run IDs or resumed active run; use current/persisted snapshots.
- Desktop profile migration and IPC adapter are Stage B/C; no silent profile copying.
- Existing baseline residuals remain: incomplete Turkish localization, active-run network-mode freeze coverage, legacy plaintext migration debt, plugin credential-positive and abrupt-parent-death coverage, remote HTTP MCP/manual desktop MCP flows, LM Studio/model-specific Agent limits, partial secondary-cloud/cancel paths, denied microphone permission, blocked Whisper/Piper dependency paths, unavailable Kokoro, uncertified Voice Chat/Voice Agent, unvalidated WAL/SHM corruption.

**FINAL VERDICT: PARTIAL**

Daemon/typed IPC/Rust CLI/TUI foundation and fresh automated Desktop baseline checks pass. A real current-host run exposed accepted unsupported narrative grounding; security/grounded-final closure cannot be truthfully marked fully PASS until that owning-core defect is resolved. No existing test failure remains.

Evidence is under ignored `build/certification/phase2/`; source, tests, schema, generated contractual models, CI and documentation are intentional changes. Generated Rust build artifacts remain ignored in `cli/target/`; no generated temporary fixtures are intended for source control. No user profile or external service was removed. Final owned-process inspection found no daemon, llama-server, CLI/TUI, MCP/plugin-host or speech-runtime orphan. Crash-fixture temporary directories and test-created empty Rust directories were cleaned; ignored build/certification and Cargo target output are intentionally retained.

---

# SENTINEL — PHASE 2 FINAL CLOSURE REPORT

Current-host closure attempt, 2026-10-04. This appendix preserves the earlier Phase 2 report and its PARTIAL verdict as historical evidence. Results below supersede that report only for the checks actually repeated here. No commit or push was performed.

## 1. Blocking Grounding Defect

**Reproduced before production changes.** `AgentLoopTest::hiddenNegativeClaimReproduction` used a temporary workspace containing `visible.txt`, `.hidden.txt`, `normal/file.txt`, and `.hidden-dir/nested.txt`. The scripted planner went through the production AgentLoop, registered built-ins, RealToolExecutor, ToolExecutionGateway, authorization snapshot and QtFileSystemService. A default `list-directory` excluded hidden entries, yet the runtime accepted “No hidden files were found” as Completed. The pre-fix regression passed because it demonstrated this incorrect acceptance; it is not counted as a successful grounding result. Evidence: `/tmp/sentinel-closure-repro.log` and `/tmp/sentinel-closure-repro-build.log`.

The tool already reported `includeHidden:false`. The owning defect was that structured claim checks did not constrain additional free-form filesystem prose. Generic inspection and a subsequent Context-mode final could escape claim-level coverage. Empty results were also insufficiently expressive about scope, traversal bounds and access limitations.

Affected path: AgentRuntime/AgentLoop → registered filesystem tool → ToolExecutionGateway → IFileSystemService → structured observation → ObservationPolicy/ClaimGroundingResolver → accepted final → daemon/IPC/client. The fix belongs in core, not Rust or transport.

## 2. Observation Semantics

`FileSystemCoverage` is the typed consumer of filesystem observation JSON. It records exact scope, Included/Excluded/Unknown hidden policy, complete/truncated/cancelled/permissionLimited, recursion, known symlink policy, skipped symlinks, depth and result limits. Missing fields default conservatively; no current-directory or home-directory fallback supplies missing scope. Explicit scope has precedence over the observation's root/path fields.

Listing/search producers retain their existing behavior and add truthful coverage. Listing is root-only with 500 entries; recursive file enumeration is bounded to 5,000 files, depth 128 and enumeration budget 20,000; glob/grep matches are bounded to 100. Reaching a relevant limit makes absence unprovable. Cancellation, traversal issues and denied/inaccessible paths also prevent absence. `permissionLimited` conservatively includes any recorded traversal/access issue, including I/O errors; the issue list retains its detailed cause. Hidden entry evidence is bounded to 100 paths and includes hidden directories, including empty directories. Hidden inclusion remains opt-in.

Symlink targets remain excluded from recursive enumeration. Unknown policy or skipped targets cannot prove broad absence. An actual directory alias is now represented by its own name/path in a listing, rather than the authorized target's name. Target authorization and traversal policy are unchanged.

## 3. Grounding Policy

Typed `HiddenEntriesExist` and `PathPatternExists` requirements use the same ClaimGroundingResolver authority as existing filesystem claims. Negative results require complete, uncancelled, untruncated, unrestricted observations with matching root/query, explicit hidden inclusion where relevant, and sufficient recursion/depth/limits. Subdirectory evidence cannot become workspace-wide evidence. Newer insufficient evidence cannot preserve an earlier absence conclusion silently.

Positive existence remains valid from directly observed entries even when traversal is partial. Exact authorized file reads and metadata keep their narrower evidence semantics.

Filesystem finals are deterministic, evidence-derived representations: scoped facts, quoted observed entries, or literal source/text extracts. LlmAgentRuntime produces that representation and AgentLoop independently checks it before acceptance, including Context-mode finals. Additional model-authored filesystem narrative is not admitted by a prose blacklist or a prompt-only rule. Filenames and file content are quoted data, so adversarial text does not become an unverified filesystem assertion. A file read can therefore return an observed-text extract rather than the earlier bare project-name response. Chat completion is separate.

## 4. Tests Added

- Pre-fix reproduction becomes rejection coverage for “No hidden files”, “There are no hidden files”, “No dotfiles”, and Context-mode escape.
- Production gateway/service integration verifies excluded hidden entries, included positive entries contradicting a negative, and complete no-hidden evidence accepting a negative.
- A–N resolver matrix has **17 rows**: excluded/unknown policy, present/absent hidden entries, truncation, cancellation, permissions, non-recursion, subdirectory scope, partial positive, recursive complete, hidden directory, `.env` excluded/complete, skipped symlinks, unknown symlink policy, and root-scope skipped symlinks.
- Actual service tests cover hidden-directory traversal, alias representation, permission limitation, cancellation and bounded listing completeness.
- Explicit scope precedence/missing scope, literal file extracts/idempotence and adversarial quoted filenames are covered.

Existing frozen-workspace authorization coverage remains. Its legacy mock returned no typed coverage; its final fixture was adapted to the production canonical representation while retaining authorization assertions. No parallel test-only grounding engine was introduced.

## 5. Focused Test Results

Final focused CTest: **3/3 PASS** (`test_agent_loop`, `test_observation_policy`, `test_daemon_ipc`). Full-run QtTest totals: AgentLoop **21/0/0**, ObservationPolicy **34/0/0**, DaemonIpcServer **21/0/0**, including initialization/cleanup. The A–N matrix and gateway tests execute the real resolver and tool boundary.

## 6. Real Agent Validation

Provider/model: actual owned llama.cpp server, `llama-cpp-server / sentinel-nemotron`, existing NVIDIA Nemotron 3 Nano 4B Q4_K_M GGUF, no provider/model fallback. `run.started` captures the frozen binding. Separate temporary portable profiles isolate the hidden and no-hidden workspaces. User approvals were supplied through the actual terminal client.

| Exact task | Actual result on final build |
|---|---|
| List the hidden files in this workspace. | **Completed / exit 0**. Real `list-directory`, hidden inclusion true, hidden file/directory observed, canonical positive final. |
| Are there any hidden files in this workspace? | **Failed / exit 1**. Model could not produce a grounded next action; no false negative or Completed was emitted. |
| Tell me whether there are no hidden files anywhere in this workspace. | **Failed / exit 1** in the hidden fixture. Model emitted a glob pattern with a trailing space; insufficient evidence did not become absence. |
| Same workspace-wide task in the separate no-hidden fixture | **Completed / exit 0**. Actual recursive glob observation supported scoped absence through core's typed resolver. |

The original accepted false-negative defect was not reproduced after the fix. Two failed model tasks are recorded as failures, not completed-task PASS. The existing AgentRunStore captures supported positive/negative claim values and their supporting tool-call IDs; `grounding-store.json` contains 2 claims, 8 evidence summaries and 4 requirements. That store persists summaries rather than the full traversal JSON. Actual listing metadata is captured in the IPC tool result. Earlier fixture/model failures are retained under `attempt1/` and `attempt2/`; they are not overwritten as successful evidence.

## 7. IPC Impact

Canonical IPC schema and protocol **1.0** are unchanged; generated C++/Rust contract verification passes. Listing metadata already travels as the existing JSON tool-result detail, retained unchanged by the typed daemon envelope and Rust event representation. Glob/grep completeness remains internal structured grounding data: the v1 tool-result detail is a human-readable summary, not a new public coverage DTO. No externally specified field was silently removed, and no internal traversal schema was unnecessarily exposed. Core's accepted final/terminal state reaches the client unchanged. Rust contains no new filesystem/security business logic.

## 8. Security Regression

Existing registry/validator/gateway, explicit approvals, AuthorizedPath, sandbox and frozen ModelBinding/workspace authority remain. The classifier is given the existing active root; missing observation scope is never invented from cwd/home. Hidden enumeration uses the same authorization checks. Actual negative authorization/cancellation tests and the full security suites run again. MCP/plugin integration uses existing boundaries; untyped filesystem evidence cannot prove absence. No Rust direct filesystem traversal, shell heuristic or new execution route was added. Cross-user socket rejection remains a previously unvalidated host limitation.

## 9. Full C++ Regression

Fresh no-ccache configure/build/link/QML compilation pass. Final compiler invocations emit **0 compiler warnings/errors**. All **109 registered suites** are executed; this closure cannot claim a clean full-suite result while native secure-store access remains blocked.

The latest full run uses a 120-second per-test bound. ApplicationController remains **127/0/0**, DesktopShell **74/0/0**. The earlier unrestricted attempt finished **108 PASS / 1 terminated (`test_upgrade`)** after stopping that test's indefinite native wait. A subsequent 30-second diagnostic run cut both UpgradeTest and a 31-second MCP fault test; the longer final run passes MCP. These attempts are retained rather than hidden.

`UpgradeTest::settingsBackwardCompatible` passes. `settingsForwardCompatible` waits inside `DpapiEncryptedSettingsStore::getOrCreateMacKeychainKey → SecItemCopyMatching → SecKeychainItemCopyContent`, confirmed by a stack-only sample. The macOS authorization UI cannot be inspected: Computer Use explicitly refuses `com.apple.SecurityAgent`. No secure-store value was logged and no key/access policy was deleted or bypassed. This is an observed native authorization/environment blocker, not evidence of a grounding failure, but it prevents the required 109/109 closure.

The same **six testcase SKIPs** listed in the historical report remain distinct from suite PASS: four strict-macOS fork fixtures, installed-docker missing-runtime branch, and installed-npx missing-runtime branch. No suite is silently skipped.

## 10. Rust Regression

Rust **1.94.0**: `cargo fmt --all --check` PASS; `cargo clippy --all-targets --all-features -- -D warnings` PASS; `cargo test --workspace` **16 PASS / 0 FAIL / 0 ignored**. Canonical contract generation/check passes. CLI/TUI share the existing Rust IPC client.

## 11. Real Phase 2 Runtime Regression

Final-build real smoke repeats daemon startup/handshake, status/models, sessions, actual Chat MAVI, streaming, JSON, Chat SIGINT cancellation/exit 2 and post-cancel MAVI. Actual Ratatui PTY launch, streaming/completion, approval, Agent completion, cancellation and exit pass. Client termination leaves daemon work alive; another client attaches/cancels and a next request succeeds. Graceful shutdown gives unavailable exit 3; normal and controlled forced daemon restart preserve completed conversations and recover owned stale endpoints. Deterministic contracts independently cover multi-client correlation/attach/approval replay.

Actual CLI Agent cancellation while awaiting approval returns exit 2 with no late Completed. The next Agent reads the real CMakeLists through `read-file` and reaches Completed/exit 0; its literal evidence contains `project(Sentinel ...)`. Subsequent daemon status reports zero active runs. The canonical extract is deliberately distinguished from an unverified narrative answer.

The generic Turkish workspace listing in the full real helper returns **Failed / exit 1** after repeated glob calls over a bounded repository/build tree. A separately attempted explicit root-listing prompt timed out. These are unresolved model/task outcomes, not declared PASS. Real stdin Agent greeting also returns Failed; deterministic stdin/exit/output contracts pass. No fallback disguises these failures.

## 12. Static Quality

Changed-line clang-format gate PASS. Clang-tidy runs all **17 changed compiled translation units**, with Apple's PCH removed and the actual SDK provided to Homebrew LLVM: **17 exit 0, 0 errors, 662 emitted warnings** under the existing `WarningsAsErrors: ''` policy. This is a passing configured gate, not warning-free certification. Warning classes include 509 missing-brace style findings, 103 optional-access findings (including existing `.ok()`-guarded filesystem result access), narrowing/signed-bitwise, copies/moves and existing analysis findings. New resolver findings are brace-style findings; no new tidy policy was added and the backlog is not concealed.

QML lint target exits successfully with **866 emitted warnings**, matching the earlier closure lint invocation; no QML source changed. The historical Phase 2 count of 736 is not reused as this run's count. Existing QML qualification/type warning debt remains. Canonical generation and changed-document local-reference checks pass. AppStream tools remain unavailable on macOS. Final `git diff --check` is required and recorded below.

## 13. Process Hygiene

Runtime helpers own and shut down only their freshly created daemon, CLI/PTY and llama-server children. Unique profile directories and temporary sockets/workspaces are removed in their `finally` blocks. No external user service is killed. Final process inspection is performed after all checks complete. Ignored build/certification evidence, Cargo target output and `/tmp/sentinel-closure-*` diagnostic logs/helpers are intentionally retained; they are not source artifacts or credentials. Native authorization cannot be resolved by deleting a real user Keychain item.

## 14. Defects Fixed

1. Accepted unstructured filesystem negative claim despite hidden-excluding evidence: fixed by typed scope/completeness authority and independently enforced canonical filesystem finals.
2. Context-mode escape after insufficient filesystem evidence: covered by the same final boundary.
3. Truncation/cancellation/access limitations and glob match bounds could look complete: explicit conservative coverage now prevents absence inference.
4. Hidden directories/empty hidden directories and skipped recursive symlinks lacked sufficient typed evidence: bounded evidence added without forcing hidden enumeration.
5. Listing a symlink alias emitted the target name: preserve the actual authorized directory entry name/path.
6. Adversarial filename/file text could look like narrative: quoted/literal evidence representations and idempotent checks.
7. Final inspection caught a scope-precedence editing defect before closure; fixed and an explicit precedence/no-fallback regression added.

## 15. Remaining Work

- Resolve the observed native Keychain authorization wait and rerun `test_upgrade` plus the full suite before a Phase 2 PASS decision.
- Real model failed two hidden-file questions, a generic root-listing task and stdin greeting; an explicit root-listing trial timed out. No false success is accepted, but these task outcomes remain limited.
- Existing Phase 2/platform/audio/localization/cloud/remote-MCP/WAL residuals from the historical report remain. No new provider, feature, persistence rewrite or Desktop migration was started.
- Clang-tidy and QML warning debt is reported, not silently treated as zero.

## 16. Commit Readiness

Production changes are the existing intentional Phase 2 foundation plus owning-core grounding/coverage fixes and the inspector's added claim labels. Regression changes cover the real authority. Documentation/CI/canonical schema/generated contractual models are intentional. Existing changed core files were formatted; this contributes to diff size. An attempted cosmetic reconstruction of formatting-only hunks was rejected by automatic approval review because it could lose uncommitted work; it did not run. The safe alternative retained the existing changes and verified formatting normally.

Untracked source files are intentional Phase 2 files; no source-tree log, database, model, private key, socket, backup or build output was detected. No intentional Phase 2 work was removed. Final status/stat/diff checks are recorded after this appendix. The tree can be reviewed for a future commit, but **commit/release approval is not recommended as a fully certified Phase 2 closure until the full regression is clean**. No commit/push was made, per the latest request.

**FINAL VERDICT: PARTIAL**

The original hidden-files false-negative acceptance is reproduced, fixed and successfully rejected in deterministic and real runtime revalidation. A truthful complete no-hidden observation is still accepted. Phase 2 remains PARTIAL because the required full regression is not clean: UpgradeTest cannot complete native Keychain access on this host. Real model task limitations are also explicitly recorded. This appendix does not turn a timeout, rejected run or unavailable native authorization into PASS.

Final recorded checks: **109 executed, 108 PASS, 1 FAIL (`test_upgrade`, Timeout 120.04s), 0 suite SKIP**; total runtime 193.46s. MCP integration passes in 31.41s, confirming the 30-second diagnostic bound was insufficient. ApplicationController **127/0/0**, DesktopShell **74/0/0**, AgentLoop **21/0/0**, ObservationPolicy **34/0/0**, DaemonIpcServer **21/0/0**. Final process inspection found **no owned daemon, llama-server, CLI, plugin-host or MCP/test process orphan**. Four exact timed-out UpgradeTest temporary directories were verified to contain only the test's `settings.json` schema/theme fixture and removed; no Keychain item or real profile was removed. Final `git status --short`, `git diff --stat` and `git diff --check` were executed; diff check PASS. The source tree retains intentional Phase 2/closure changes and **18 intentional untracked source/document files**. Evidence snapshots are `/tmp/sentinel-closure-git-{status,stat,check}.log`; runtime evidence remains in ignored `build/certification/phase2-closure/`. **Source review readiness: yes; fully certified Phase 2 commit readiness: no, pending the native Keychain/full-regression closure.**

---

# PHASE 2 TIMEOUT RECHECK

Current-host recheck, 2026-10-04. The previous 108-PASS/1-timeout result is preserved above as historical evidence. This recheck changes only this report: no production code, test seam, secure-store policy or timeout was changed. `test_upgrade` was not skipped; every CTest invocation retained the existing 120-second bound. No commit or push was performed.

**test_upgrade standalone:** three consecutive runs PASS. Each runs all UpgradeTest cases, including settings migration/encryption and chat-history schema migration. The complete regression additionally records UpgradeTest **5/0/0** including setup/cleanup.

**authorization prompt:** the developer confirmed native authorization with “onaylandı” during the first standalone run, then explicitly confirmed that no further prompt appeared during runs 2 and 3. The native dialog was developer-observed/approved; the agent did not inspect or operate SecurityAgent and did not record a password. The previous stack identifies the requesting case as `UpgradeTest::settingsForwardCompatible`, at `DpapiEncryptedSettingsStore::getOrCreateMacKeychainKey → SecItemCopyMatching → SecKeychainItemCopyContent`. Current-run dialog text was not captured; case attribution comes from that exact prior timeout stack. The test completed after authorization with no code change.

| Run | Test duration | CTest total duration | Authorization | Result |
|---|---:|---:|---|---|
| 1 | 5.00 s | 5.01 s | Approved by developer | PASS |
| 2 | 0.37 s | 0.38 s | No further prompt, developer-confirmed | PASS |
| 3 | 0.12 s | 0.12 s | No further prompt, developer-confirmed | PASS |

Durations are the exact precision reported by CTest. No timeout was extended to obtain these results.

**full C++ suite:** **109/109 PASS, 0 FAIL, 0 TIMEOUT, 0 suite SKIP**, total **69.21 s**. The six previously documented testcase SKIPs are unchanged: four strict-macOS fork fixtures and the installed-docker/installed-npx unavailable-runtime branches. There is **no new SKIP**; these cases are not counted as testcase PASS. Full per-case evidence is retained in `/tmp/sentinel-timeout-recheck-full-cases.log`.

**ApplicationController:** **127/0/0**, independently rerun after the clean full suite.

**DesktopShell:** **74/0/0**, independently rerun after the clean full suite.

**Rust:** `cargo fmt --all --check` PASS; `cargo clippy --workspace --all-targets --all-features -- -D warnings` PASS; `cargo test --workspace` **16 PASS, 0 FAIL, 0 ignored**.

**git diff --check:** PASS after the report update.

**owned orphan processes:** none observed after all test processes completed. No external service was stopped or user state removed. Diagnostic/test logs remain intentionally outside the source tree under `/tmp/sentinel-timeout-recheck-*`; normal test fixtures clean themselves up on these successful runs.

**root cause of previous timeout:** unattended interactive macOS Keychain authorization. The prior native-wait stack, developer-confirmed approval, first-run completion after approval, two immediate prompt-free repeats, and the complete clean regression confirm the external cause for the observed timeout. No production defect or test-seam change is indicated by this result. In particular, increasing the timeout or skipping UpgradeTest was unnecessary.

**FINAL VERDICT: PASS**

The previous full-regression timeout blocker is closed. Together with the preceding owning-core grounding fix and recorded Phase 2 runtime validation, the current-host Phase 2 baseline may now be marked PASS. Previously documented model/task limitations, platform/runtime gaps and static warning debt remain explicit residuals; this recheck does not claim those separate tasks were rerun or resolved. The historical PARTIAL results above remain intact. The working tree is ready for review and a separately authorized commit; **no commit/push was performed**.
