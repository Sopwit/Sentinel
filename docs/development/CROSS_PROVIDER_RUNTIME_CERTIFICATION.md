# SENTINEL — CROSS-PROVIDER RUNTIME CERTIFICATION REPORT

Date: 2026-10-02. Host: macOS arm64. Verdict: **PARTIAL**.

Only existing providers and existing local models were used. Live desktop checks used the real built desktop through Computer Use; automated provider fixtures are not represented as cloud runtime certification. No secrets were generated or logged. No new provider, tool, model-name branch, timeout, or grounding relaxation was added.

## 1. Baseline regression

Reported baseline: 101/103 CTest targets. Reproduced failures: `test_mcp_integration` and `test_agent_runtime`. Final suite: **103/103 targets passed**, 21.96 seconds, `ctest --preset tests --output-on-failure --parallel 4`.

MCP: **PRE-EXISTING REAL DEFECT** in macOS sandbox runtime dependency access. The child failed during dyld loading of Homebrew QtCore, then ICU, before handshake. After that fix, **PRE-EXISTING TEST DEBT** was exposed: fixtures expected the sandboxed child to write into the parent's temporary directory, expected denial to complete successfully, and used stale capability defaults. Updated tests preserve authorization denial, sandbox write restrictions and cancellation; no MCP feature was introduced.

AgentRuntime exact initial failing cases:

- `asyncRunCommandStreamsAndContinues`
- `asyncRunCommandCancellationAndTimeout`
- `asyncDockerUsesProcessExecutorAndPreservesRestrictions`
- `shutdownStopsActiveCommand`
- `streamsRealProviderDeltasWithStepCorrelation`
- `approvalEventsKeepToolCallIdentity`
- `approvalDelegatesToLoop`
- `resumesApprovalOnSameWorkerOwnedSession`
- `runtimeWiresSubagentExecution`

The first four are **ENVIRONMENTAL / PLATFORM NOT SUPPORTED**: macOS cannot enforce the requested detached-child guarantee. They now explicitly SKIP rather than treating a truthful unsupported result as success or failure of streaming. The remaining five are **PRE-EXISTING TEST DEBT**: stale registry defaults, real-executor fixtures, missing frozen streaming model binding/classifier response, content-free final answers, approval boolean semantics, and insufficient bounded subagent scope. Updated fixtures match the current contracts without weakening production security or final acceptance.

## 2. LM Studio

Endpoint: `http://127.0.0.1:1234`; real `GET /v1/models` succeeded. Chat model: `qwen/qwen3.5-9b`, loaded MLX 4-bit, approximately 5.98 GB, effective context 32768. Catalog also included Gemma, Nemotron and a Nomic embedding model.

Thinking was originally ON; temporarily OFF for the live checks and restored ON afterwards. This configuration is managed by LM Studio, not a Sentinel model-specific branch.

Chat first turn `Yalnızca MAVI yaz.` → `MAVI`; contextual second turn → `MAVI`; streaming count → 1 through 20. **PASS**. Agent cancellation disconnected the outstanding server request, produced Cancelled, unlocked the composer and allowed subsequent Chat requests. **PASS**. A later long Chat response finished too quickly to establish an unambiguous Chat-specific cancellation result; it is not counted as a separate cancellation pass.

Agent with thinking OFF: classifier completed with JSON (352 input tokens, 61 output tokens, zero reasoning tokens), then native tool selection, explicit approval and real filesystem execution occurred. First relative `workspace` directory was truthfully not found. A subsequent approved absolute workspace-root listing returned real entries. **Filesystem execution FIXED + PASS**. Continuation made additional calls; the bounded run was cancelled after seven steps without an accepted grounded final. **Agent BLOCKED**, not Completed PASS. That live run preceded the final process-handler workspace-root fix, so it does not establish a pure model limitation after all fixes.

## 3. llama.cpp

Executable: `/opt/homebrew/bin/llama-server`; version 0.5.0, build 11146, commit `7fe450e19`.

Existing model: `/Volumes/NeuralSilicon/LLM/Model/lmstudio-community/NVIDIA-Nemotron-3-Nano-4B-GGUF/NVIDIA-Nemotron-3-Nano-4B-Q4_K_M.gguf` (Q4_K_M, approximately 3.97B parameters).

Endpoint: `http://127.0.0.1:8081`; alias `sentinel-nemotron`; flags `--ctx-size 8192 --jinja --reasoning off --host 127.0.0.1`; four server slots. Real discovery succeeded.

Real Sentinel Chat: MAVI first turn (35 input / 3 output, 1.035 s); contextual MAVI (49 / 3, 0.766 s); streaming 1–20 (70 / 51, 4.297 s). **PASS** independently of LM Studio.

Long story streamed partial content; Stop produced server `cancel task` and slot release approximately 81 ms later. UI unlocked, provider/model selection stayed frozen while active, no late completion was observed, and the next request returned DEVAM (176 / 4, 1.600 s). **Cancellation and recovery PASS**.

Agent: **BLOCKED at capability binding**. `/v1/models` advertised completion only, while `/props` reported template support for tools/tool calls/parallel calls/object arguments. Sentinel's discovered model did not obtain a supported native-tool binding. This is **not** evidence that the GGUF/template lacks tools, and no unsupported model was forced into Agent PASS. No discovery enhancement was implemented without further contract evidence.

The certification-only server was stopped afterwards; the original Sentinel llama.cpp endpoint `http://127.0.0.1:8080` was restored.

## 4. Cloud provider #1

OpenAI candidate: **NOT VALIDATED — credential unavailable**. Existing OpenAI credential store entry and environment key were absent. No model was dispatched; Chat, streaming, cancellation, tools, grounding, persistence and Completed remain NOT VALIDATED.

## 5. Cloud provider #2

Claude/Gemini candidates: **NOT VALIDATED — credential unavailable**. Credential presence checks also found no usable credentials for the other existing cloud providers. Cloud UI selected no usable model and disabled Send; it did not substitute a local provider. Invalid credential, live rate limit and cloud provider-outage tests were not performed.

## 6. Network modes

Production `NetworkPolicyService` and automated tests were inspected and passed. Online permits valid endpoints. Local Only permits localhost/literal loopback and blocks remote endpoints. Offline also permits loopback under the current product contract and blocks remote endpoints; it does not mean every local inference request is forbidden. Invalid URLs are rejected. Workspace local-only privacy can constrain Online to Local Only.

These are production-contract/automated results, **not live cloud mode certification**.

## 7. No silent fallback

Live llama.cpp selection of a stale unavailable model produced `Selected model is unavailable in provider catalog.` rather than substitution. A later switch back to llama.cpp produced visible `Provider model discovery failed (ConnectionFailed).`, unlocked the UI and did not substitute LM Studio. The server was still present when that discovery failure occurred; the cause of this later connection failure was not isolated and is not called a server-stop test.

Cloud with no selectable credential-backed model left Send disabled; no local fallback was observed. Automated router/provider failure invariants passed. Explicit live LM Studio server-stop, cloud invalid-auth and independently isolated server-stop cases remain NOT VALIDATED.

## 8. Capability validation

Model registry, static runtime capabilities, provider catalog and local inference tests passed. Embedding-only models must not obtain a Chat/Agent-capable binding. The Nomic embedding model was present in LM Studio discovery, but a separate live embedding-selection rejection was not exercised in this run. llama.cpp template capability evidence and discovery-binding limitations are recorded separately above. No embeddings were certified live.

## 9. Cross-provider cancellation and switching

Active provider/model controls were disabled in actual LM Studio Agent and llama.cpp Chat runs; attempted selection changes did not alter the active binding. llama.cpp cancellation followed by a successful request was verified. LM Studio Agent cancellation followed by successful LM Studio Chat was verified. A later cross-provider switch reached the new provider's truthful discovery failure, not a successful new-provider completion. Successful post-cancel cross-provider switching and cloud frozen binding therefore remain NOT VALIDATED end-to-end; automated lifecycle/binding coverage passed.

## 10. Defects found

**DEFECT-1: filesystem registered handler used process cwd instead of the authorized frozen workspace.** Fixed `BuiltInToolProvider` to obtain execution cwd from the validated invocation resource snapshot in synchronous, deferred and parallel paths. Exact owning-layer regression: `registeredFilesystemHandlerUsesFrozenWorkspace`, process cwd intentionally differs from the temporary workspace. Approved real workspace listing succeeded live.

**DEFECT-2: process registered handler validated against process cwd instead of the authorized frozen workspace.** Fixed `RealToolExecutor` to use the resource snapshot working directory while retaining the sandbox-plan equality check. Regression: `registeredProcessHandlerUsesFrozenWorkspace`. On macOS it now reaches the truthful unsupported detached-child result rather than a spurious changed-plan denial. Real process success is not claimed on this host.

**DEFECT-3: macOS sandbox denied required Homebrew Qt/ICU runtime dependencies.** Fixed `ProcessSandbox` to permit Qt install-name aliases and read/map access to library directories associated with already-loaded Homebrew images, with ancestor metadata traversal. It does not grant writes, network access or read access to the entire Homebrew tree. Real MCP helper handshake and strict existing integration regressions pass.

## 11. Model-specific limitations

Qwen thinking ON: historical reasoning-only classifier behavior remains MODEL-LIMITED / configuration characteristic. Thinking OFF: classification and approved filesystem tool execution succeeded, but the bounded continuation did not yield grounded final; current live Agent result BLOCKED. Gemma: previous BLOCKED continuation, not newly certified. Nemotron: previous MODEL-LIMITED repeated identical tool call; llama.cpp Chat newly PASS, Agent binding BLOCKED rather than a newly proven model failure.

## 12. Automated regression

`cmake --preset tests`: PASS. `cmake --build --preset tests`: PASS. Full CTest: **103/103 passed, zero failed targets**. Provider/catalog/router/model registry/local inference; observation policy, LLM agent runtime, AgentLoop, AgentRuntime service, gateway; Chat mode/controller/view model; desktop shell all passed. ApplicationController: **127 passed / 0 failed / 0 skipped**. `git diff --check`: PASS.

Target-level green does not imply every platform-specific subcase ran. Explicit skips include the four AgentRuntime detached-child cases; existing plugin detached-child platform skips; and missing-Docker/missing-npx branches because both executables exist on this host. Existing fenced JSON strictness, nonduplicated native schemas, frozen roots, grounded-final acceptance, doom-loop, cancellation and timeout=0 coverage were preserved. No new failed target remains.

## 13. Remaining blocks and certification matrix

Cloud credentials are required for cloud Chat/Agent/file grounding/nonexistent-file/process/security/persistence tests. No tool-capable provider has reached a newly verified real Agent Completed grounded final in this run. llama.cpp native-tool capability binding needs further isolation; successful cross-provider recovery remains unverified. The final process fix has owning-layer coverage but was not followed by another full live Agent continuation.

| Field | LM Studio | llama.cpp | Cloud #1 | Cloud #2 |
|---|---|---|---|---|
| Discovery | PASS | PASS; later connection failure | NOT VALIDATED | NOT VALIDATED |
| Chat first turn/context | PASS | PASS | NOT VALIDATED | NOT VALIDATED |
| Streaming | PASS | PASS | NOT VALIDATED | NOT VALIDATED |
| Cancellation/recovery | PASS (Agent→Chat) | PASS (Chat→Chat) | NOT VALIDATED | NOT VALIDATED |
| Active model binding | PASS | PASS | NOT VALIDATED | NOT VALIDATED |
| Agent classification/planning | PASS with thinking OFF | BLOCKED binding | NOT VALIDATED | NOT VALIDATED |
| Native tool execution | FIXED + PASS filesystem | BLOCKED | NOT VALIDATED | NOT VALIDATED |
| Grounded final/Completed | BLOCKED | BLOCKED | NOT VALIDATED | NOT VALIDATED |
| Provider unavailable | NOT VALIDATED | Truthful discovery failure observed | NOT VALIDATED | NOT VALIDATED |
| Auth/network error | NOT VALIDATED | ConnectionFailed visible | NOT VALIDATED | NOT VALIDATED |
| No silent fallback | Automated PASS; outage not live tested | PASS observed errors | Readiness blocked; live auth NOT VALIDATED | NOT VALIDATED |

| Provider | Chat | Streaming | Cancel | Agent | Tools | Grounding | Verdict |
|---|---|---|---|---|---|---|---|
| LM Studio | PASS | PASS | PASS | BLOCKED | FIXED + PASS filesystem | BLOCKED final | PARTIAL |
| llama.cpp | PASS | PASS | PASS | BLOCKED | BLOCKED binding | NOT VALIDATED | PARTIAL |
| Cloud #1 | NOT VALIDATED | NOT VALIDATED | NOT VALIDATED | NOT VALIDATED | NOT VALIDATED | NOT VALIDATED | BLOCKED credentials |
| Cloud #2 | NOT VALIDATED | NOT VALIDATED | NOT VALIDATED | NOT VALIDATED | NOT VALIDATED | NOT VALIDATED | BLOCKED credentials |

**FINAL VERDICT: PARTIAL.** Minimum certification criteria are not met; neither cloud runtime nor an accepted real Agent Completed final is certified. Automated regressions improved from the reported 101/103 baseline to 103/103 targets, with platform skips explicitly disclosed. Computer Use influenced the evidence: real desktop flows and approval/cancellation states were checked rather than inferred from provider-only API responses.
