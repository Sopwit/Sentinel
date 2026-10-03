# SENTINEL — PLUGIN / SKILL RESIDUAL CLOSURE REPORT

Certification date: 2026-10-03. Platform: macOS 27.0.1, Qt 6.11.2.
This report distinguishes successful real runtime gates from unverified cases.
No model, dependency stack, credential, provider, plugin API, or skill format was added.
The previous certification report and its four established fixes are retained.

## 1. Skill Context Integration

Missing link: SkillService discovery, enabled state and workspace persistence existed,
but AgentRuntime did not supply those instructions to the Agent context assembly.

Implemented path:
`SkillService → AgentRuntime.configureLoop → AgentLoop.setSkills → ContextEngine → LlmAgentRuntime common prompt renderer → bound IChatProvider`.
The loop receives a by-value skill snapshot. ContextEngine owns admission and budgeting;
there is no QML concatenation, per-provider injection, or marker postprocessing.
The common renderer emits admitted instructions once, in a clearly named presentation-only
section. Existing context-kind numbers remain unchanged; Skill is appended as kind 9.

Enabled global and matching workspace skills are sorted by name, deduplicated, and filtered
by validity, enabled preference and Enabled state. Workspace identity is compared with the
session's workspace context. Disabled and other-workspace instructions are excluded.
The section uses the existing budget machinery with a ceiling of 1,200 estimated tokens
and one-third of the remaining budget; oversized entries are omitted, not silently expanded.
No new precedence mechanism was introduced.

Evidence: real SkillService-to-ContextEngine tests cover enabled/disabled marker, A/B scope,
two-skill ordering, duplicate suppression and oversized omission. A common request-rendering
test verifies single inclusion and disabled exclusion.

## 2. Real Skill Agent

Provider/model: existing llama.cpp / `sentinel-nemotron`, NVIDIA Nemotron-3 Nano 4B Q4_K_M.
Local endpoint: `http://127.0.0.1:8081`; existing GGUF was used without downloading.
Server flags: `--alias sentinel-nemotron --ctx-size 8192 --jinja --reasoning off`.

The deterministic fixture contains the requested Turkish instruction and an equivalent
English sentence. Turkish-only wording reached the model but was not consistently followed;
the bilingual fixture made the intended behavior observable without changing provider logic.

Enabled run: request audit confirmed the marker instruction in planning and continuation;
the real filesystem tool read CMakeLists.txt, and the grounded final was
`The project is named Sentinel. [SKILL_OK]`; terminal state Completed.
Disabled repeat of the same task: no marker instruction in model requests, final `Sentinel`,
terminal state Completed. The marker was not included in the user's task.
Earlier marker-only directory-list answers were not accepted as the grounded closure gate.

## 3. Skill Security

Skill text has no permission or credential authority. A malicious test skill requests
filesystem/process grants, approval/sandbox bypass and credential disclosure. The real
AgentRuntime still enters AwaitingApproval through the ordinary gateway, does not execute
the pending tool, and creates zero persistent grants; cancellation terminates the run.
Skills do not modify ToolDescriptor, resource authorization, sandbox plans, credential
brokers or permission policy. This verifies the context/security separation, not arbitrary
model compliance with every malicious instruction.

## 4. macOS Plugin Containment

Original blocker: `PluginHostSession::start` requires both sandbox enforcement and
`forbidDetachedChildren`; the macOS ProcessSandbox backend rejected that requirement.
PluginManager's containment plan also retains the same strict requirement.
Root cause: incomplete backend implementation of the no-descendant invariant, not a
requirement for the host itself to outlive the desktop or survive desktop restart.

The host is owned and monitored through ProcessExecutor, uses a plugin-private working
directory and empty environment except PATH, has direct network disabled, and receives
only explicitly scoped readable/writable paths. Existing macOS runtime dependency access
remains intact. The backend now adds `(deny process-fork)` for strict plans, preventing
all descendants rather than attempting to supervise detached descendants after launch.
Enforcement remains mandatory; unsupported launch does not become an unsandboxed fallback.

Actual native code inside the host verified fork and posix_spawn denial, direct localhost
socket denial, and direct workspace file-open denial. This is stronger than a generated
profile-only assertion. Independent brokered access is tested separately.

Important limit: this backend has no kill-on-parent-exit primitive. No-fork containment
does not claim Windows Job-equivalent lifetime guarantees. Normal teardown and forced host
failure are tested; abrupt desktop death and a hostile host surviving parent death are not
certified. Plugins requiring direct child creation remain incompatible with strict plans.

## 5. Plugin Host

Real sample plugins were loaded by `sentinel-plugin-host` in a separate PID, with parent PID
checked against the test process. A representative observed pair was host 78929 / parent
78918. Mandatory sandbox launch precedes health negotiation and tool execution.
Unload/reload and normal teardown complete; owned-host process checks found no orphans.

An additional actual startup crash was diagnosed: the host reader placed a >1 MiB frame
buffer on a roughly 544 KiB thread stack. The same bounded buffer is now heap-backed;
the protocol size, ABI 5 and protocol 3 remain unchanged.

## 6. Plugin Execution

Echo: original sample plugin returned `PLUGIN ECHO: MERHABA` through registry, argument
validation, approval/policy, gateway and the actual host. Arithmetic: a small test-only
native SDK fixture returned `5` for add(2,3), through the same production path.
The fixture is a real compiled plugin, not a fake provider, and is not installed in user
discovery directories. It uses only existing SDK/broker contracts.

Successful plugin results now supply Generic structured observations and turn-scoped
ExternalService/Provider evidence. This proves actual plugin invocation, not filesystem
or process facts inferred from arbitrary plugin text. Previously evidence count zero
prevented a truthful plugin result from satisfying the Agent grounding gate.

## 7. Brokers

Filesystem: authorized workspace read returned `BROKER_READ_OK`; external read without
grant was denied; an explicit naturally reachable Plugin A resource grant permitted the
same external fixture. Direct unbrokered access remained denied.
Process: existing broker executed `/bin/pwd` successfully in the plugin-private directory,
not the desktop workspace. Policy and process sandbox remain authoritative.
Network: a controlled localhost HTTP server returned `BROKER_NETWORK`; LocalOnly denied
an external target, and Offline returned PluginNetworkOffline for the external target.
No external request was sent. The existing Offline contract still permits loopback.
Credential: a reference without declaration/grant was denied before secret-store access;
no secret was printed. Approved real credential-reference behavior is NOT VALIDATED,
because there was no approved credential fixture and none was created.

## 8. Plugin Failure Isolation

Crash: SIGKILL of the exact owned host yielded PluginCrashed, a single failed callback,
registry removal, a surviving desktop/runtime process, and successful restart.
Timeout: SIGSTOP of the owned host during a real pending gateway call, followed by a
bounded health timeout, yielded PluginTimeout / Timeout, terminated the host, and produced
one failure with no late success.
Cancellation: a real delayed plugin call was cancelled; result and failure category are
Cancelled, host termination completed, and no late Completed/success appeared.
Unload during a queued invocation is now rejected once stopping/failure begins, closing
a callback-loss race. Startup failures with no PID and crashes after startup are distinct.

## 9. Agent → Plugin

Provider/model: real llama.cpp / sentinel-nemotron.
Tool: original sample Echo. Trace: planning → plugin selection → gateway validation →
approval → separate sandboxed host → real `PLUGIN ECHO: MERHABA` → Generic observation
with one provider-evidence item → real continuation → grounded final
`The Echo plugin successfully returned: MERHABA` → Completed.
A separate Delayed Echo run also completed. No fake provider counts for these gates.

A later repetition after the final failure-category changes hit the probe's 180-second
bound during classification and ended Cancelled before tool execution; it is NOT recorded
as PASS. The earlier successful real Echo gate remains the success evidence. No model
fallback or fabricated completion was used.

## 10. Permission Isolation

The real filesystem broker accepted Plugin A's exact resource grant. Replacing it with
Plugin B's grant or a built-in grant denied Plugin A. Owner and resource scope therefore
remain independent in real execution. Ask uses explicit normal approval; no blanket
plugin permission was added. Prior identity/permission fixes were not reimplemented.

## 11. Persistence

Store/service disk round-trips cover plugin enabled state and grants, global skill state
and workspace skill state. The new workspace persistence test disables in workspace A,
reconstructs SkillService from disk, verifies disabled/content exclusion, reenables, then
verifies workspace B cannot receive the instructions. Desktop relaunch was not required
for this evidence. Test settings use unique test application identity and cleanup.

## 12. Defects Fixed

1. Missing SkillService-to-authoritative-Agent-context integration.
2. Strict macOS no-descendant plans rejected despite enforceable no-fork containment.
3. Plugin-host reader thread stack overflow at real startup.
4. Queued invocation accepted during host teardown, losing completion.
5. Actual post-start host crashes misclassified as startup failures.
6. Real plugin observations lacked provider evidence, blocking grounded Agent completion.
7. Intentional plugin cancellation surfaced as crash/failure instead of typed cancellation.

Tests also correct an asynchronous assertion, canonical-path sandbox fixture, and historical
shell fixture assumptions. No permission bypass, hardcoded tool success, or silent fallback
was introduced. Stronger no-fork semantics affect all strict macOS callers, not only plugins.

## 13. Regression

Final build: `cmake --build --preset tests -j 4` PASS.
Final focused CTest: plugin integration, Skill/Extension runtime, AgentRuntime and
LlmAgentRuntime: 4/4 PASS.
QtTest totals: plugin integration 10/0/0; Skill/Extension 11/0/0; LlmAgentRuntime 17/0/0;
AgentRuntime 18/0/4 SKIP. The four explicit macOS skips are historical compound-shell/
Docker fixtures requiring forks under strict no-descendant plans. They are not plugin
runtime skips; real native-host crash/cancel/timeout tests run without skips.

Registered CTest total remains 104; added fixture is a build target, not a new CTest suite.
Final broad regression excluding only the blocked DesktopShell target: 103/103 PASS in
127.46 seconds, including
MCP, providers/chat, context, registry, validation, gateway, sandbox and permissions.
ApplicationController: 127/0/0. `git diff --check`: PASS.
Two additional pre-existing environment-dependent RealToolExecutorTools skips cover absent
Docker and absent npx branches, because both programs are installed here.
The owned certification llama-server was shut down normally. Final process inspection found
no llama-server, sentinel-plugin-host or provider_certification_probe remaining.
Full 104/104 is NOT certified: see the next section. No skipped QtTest case is presented
as proof of that case's runtime behavior.

## 14. Remaining Blocks

`test_desktop_shell_view_model` blocks in macOS Keychain authorization while reading existing
Gemini configuration. A stack sample traces DesktopShell metadata → ModelLibrary →
ModelService.providerConfig → AppSettings.geminiApiKey → credential backend →
SecItemCopyMatching/securityd wait. The owned stalled process was stopped after roughly
227 seconds. A restricted run exited without QtTest totals and is not accepted as PASS.
Resolving this requires the user's OS credential authorization; no credential contents or
OS approval were obtained. Optional Gemini probing encountered the same boundary, so its
readiness is pending authorization, not asserted absent. Credential/backend code was not
changed to manufacture a passing result.

Approved credential-reference positive behavior and abrupt-parent-death cleanup are not
validated. No broad macOS containment/lifetime claim is made beyond the actual tests.

CERTIFICATION:

- Skill Agent Context: FIXED + PASS (real enabled and disabled runs).
- Plugin Runtime: FIXED + PASS for tested containment, execution, broker negatives and lifecycle;
  credential-positive and abrupt-parent-death cases remain unvalidated.
- Agent → Plugin: FIXED + PASS (real grounded Echo Completed).

FINAL VERDICT: PARTIAL — real closure gates achieved, but full 104/104 and the explicitly
listed additional boundaries remain uncertified. Stop after this report; no new phase starts.
