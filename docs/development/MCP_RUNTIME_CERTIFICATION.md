# SENTINEL — MCP RUNTIME CERTIFICATION REPORT

Date: 2026-10-03 (Europe/Istanbul). Existing dirty-tree cross-provider and sandbox changes preserved. Personal-Brain unavailable (`brain` not found); repository path and local architecture instructions supplied by user were used.

## 1. MCP Architecture

- Configuration: AppSettings `mcpServersJson` → DesktopShellViewModel startup/change synchronization → ApplicationController configuration parsing → RealToolExecutor/McpService.
- Transport: local owned QProcess newline-delimited JSON-RPC stdio; remote HTTP/session/event-stream transport also implemented, not exercised against a real remote server here.
- Lifecycle: McpService owns local sandbox launch, initialize, inventory, pending request correlation, failure invalidation, reconnect and process-group shutdown. Remote service lifecycle is external.
- Registry path: McpService discovery → McpToolCatalog ToolDescriptor → McpToolProvider atomic provider replacement → InMemoryToolRegistry → ToolArgumentValidator → ToolExecutionGateway → approval, authorization/permission and sandbox evaluation → McpToolHandler → real MCP request → ToolExecutionResult/structured observation → AgentLoop accepted final.
- Source MCP is not trusted execution. Local server launch is independently sandboxed; imported tool authorization is ExternalService/Invoke scoped to `mcp:<server>`, tool-execution domain, default Medium risk.

## 2. Real MCP Server

- Existing small C++/Qt executable: `/Users/emir/Desktop/Projects/Sentinel/build/tests/tests/test_mcp_server` with arguments `"" --certification`.
- No installation or unrelated dependencies. Test-only opt-in server extension adds deterministic arithmetic/fault cases; no new production MCP feature.
- Transport: actual child stdio JSON-RPC, initialize protocol 2024-11-05.
- Startup/discovery: successful through production McpService sandbox launch.
- Eight tools: echo_value, delayed_echo, crash_echo, add, malformed_json, missing_result, wrong_id, server_error. Original non-certification mode remains three tools.
- Echo schema: object, required string value, additionalProperties false. Add schema: object, required numeric a/b, additionalProperties false. Fault/delay tools use the echo schema.
- Shutdown: explicit disconnect and service destruction tested. Post-run process listing found no remaining test_mcp_server process.

## 3. Discovery

- Initial eight registrations; exact schemas imported and JSON argument types retained.
- Repeated tools refresh and provider refresh preserve eight entries and stable IDs; no duplicates.
- Duplicate server config rejected deterministically. Duplicate remote tool names invalidate inventory in existing provider regression coverage.
- Server disconnect/crash removes active registry tools. Explicit reconnect rediscovers identical identities.
- UTF-8 injective escaping keeps underscore/hyphen names distinct; `mcp.<escaped-server>.<escaped-tool>` cannot silently replace built-ins. Collision checks and atomic replacement remain unchanged.

## 4. Direct Tool Execution

- Production registry lookup → validated typed arguments → gateway with actual PermissionPolicyService → actual stdio call.
- echo_value(value=MERHABA): Succeeded, actual content `ECHO: MERHABA`.
- add(a=2,b=3): Succeeded, actual text `5`, Generic structured observation `{"sum":5}`.
- Gateway has no synthetic fallback execution; NoFallback rejects if accidentally used. This deterministic direct test is not counted as a real model Agent gate.

## 5. Validation

- Missing required value, numeric value instead of string, and disallowed unknown field: InvalidArguments before MCP handler execution.
- Registered schema remains authoritative; numeric add arguments arrive as JSON numbers, not coerced strings.
- Malformed JSON: ProtocolError, inventory removed. Missing result: ProtocolError. Server JSON-RPC error: RemoteExecutionFailure.
- Unexpected response ID: ignored by the current stdio correlation contract; requested call fails Timeout after 30 seconds. It is never substituted into another call or accepted as success. No new cancellation/ID behavior manufactured.

## 6. Permissions

- Allow: real gateway PermissionPolicyService Enabled permits the scoped ExternalService invocation, with approved tool execution.
- Ask: actual real-model runs pause Awaiting Approval before dispatch; approved once, no persistent grant.
- Deny: existing real-child integration policy/approval denial and new gateway Disabled/Denied cases block execution. Sandbox capability denial also blocks before handler.
- These are isolated deterministic supported policy states, not mutation of the user's desktop policy or a blanket allow-MCP rule. Independent interactive desktop policy-matrix flow was not performed.

## 7. Agent → MCP

- Provider/model: existing llama.cpp sentinel-nemotron, effective context 8192, native tools Supported, existing Nemotron GGUF. Same server flags as cross-provider certification, loopback 8081; no download.
- Only two safe MCP tools exposed: mcp.certification.echo_5f_value and mcp.certification.add.
- Actual production ModelService binding, LlmAgentRuntime planner, AgentRuntime/AgentLoop, gateway, permission checks, approval, real MCP server and native correlated continuation.
- Two tool executions succeeded after approval. Final: `echo cevabı: ECHO: MERHABA` and `toplam: 5`.
- Terminal: **Completed**, two tool steps, four scoped external evidence records. No fake provider or synthetic tool result counted.
- An earlier llama.cpp run executed both tools but failed final grounding. A Gemini attempt also executed both tools, then hit provider rate-limit/malformed-response failure; it was not counted as PASS and did not silently fall back.
- Some actual observation classifiers returned no requirements for deterministic arithmetic/echo tasks; diagnostic output retained this fact. Separate regression forces an ExternalService requirement and verifies the corrected evidence gate accepts it.

## 8. Failure / Reconnect

- Real child crash during tools/call exits 7; inventory disappears, pending request receives truthful failure, bounded callback settles; reconnect restores tools.
- Explicit server stop removes registration; stale lookup fails rather than silently substituting another tool.
- Unavailable executable: startup fails with nonempty error and non-Connected state; no fake success.
- Manual reconnect via current connectToServer contract succeeds after stop/failure. No claim of automatic retry/backoff.
- Core loop/runtime correlation tests preserve session/turn/step/call IDs and terminal counts. Manual desktop UI busy/error/reconnect interaction not verified.

## 9. Cancellation

- Real delayed_echo stays active for 400 ms. AgentLoop cancellation invokes MCP handler cancellation, yields Cancelled, and remains terminal after a 600 ms late-response window. No late Completed/callback.
- Exact stdio implementation removes pending callback; it does not send a server-side cancellation notification. Server work can continue. Handler/gateway protect against late result publication.
- Existing real-child deterministic cancellation test passes. Manual desktop Stop with a real model was not performed.

## 10. Persistence / Startup / Shutdown

- Actual AppSettings + JsonSettingsStore written to isolated temporary disk settings, destroyed and recreated. Sanitized configuration round-trip matches, command/args restored, real server and eight tools rediscovered.
- Service exists before child launch; available/unavailable startup, explicit reconnect after stop, child-before-service shutdown, and normal service destruction covered.
- QProcess owned child running before service destruction; its QPointer null afterward. Disconnect removes inventory. No test server process remained in post-test process listing.
- No user desktop MCP settings or credentials changed. Manual full desktop configure → quit → relaunch was not performed; core persistence is PASS, interactive persistence gate remains unverified.

## 11. Security Boundary

- MCP tool execution ≠ trusted execution. Common validation, permissions, approval and gateway remain mandatory on Agent path.
- Local server process runs under existing production sandbox. No unsandboxed launch fallback added. Child attempts to log outside its sandbox temp space remain denied in existing integration coverage.
- Imported generic results now produce only ExternalService evidence, not filesystem/process facts. Declared filesystem semantic-contract handling is unchanged; untrusted text cannot fabricate filesystem grounding.
- Regression proves Failed/Blocked/Cancelled cannot satisfy Verified ExternalService final, and successful generic MCP result cannot satisfy FileSystem requirement.
- IMcpService remains a lower-level transport API; direct internal transport calls are not themselves Agent authorization. Authoritative Agent path uses the gateway.

## 12. Defects Found

- Proven owner: McpToolCatalog generic descriptor mapping had empty evidenceProduced. Real tools succeeded, but evidence records were absent, so required external observation could not ground a final.
- Fix: turn-scoped ExternalService Provider/Operation evidence descriptors for generic MCP tools. No tool-name hardcoding, global permission grant, filesystem trust or grounding-policy weakening.
- Deterministic regression covers provider/operation requirement acceptance, incompatible domain rejection and unsuccessful/cancelled rejection. Real two-tool llama.cpp scenario rerun to grounded Completed.
- Existing model classifier occasionally labels invocation as execution or uses literal `empty` resource; not overridden or papered over. Earlier failed trials are not counted as successful certification.

## 13. Regression

- CMake tests configure/full build: PASS.
- MCP provider/integration, registry, validator, gateway, AgentLoop, LlmAgentRuntime and AgentRuntimeService: PASS in final full run.
- Expanded MCP integration: five test functions, seven QtTest passes including init/cleanup; no additional CTest executable registered.
- Full suite: **103/103 PASS**, 96.15 seconds.
- ApplicationController: **127 passed / 0 failed / 0 skipped**.
- git diff --check: **PASS**. New regression: 0 in final run.

## 14. Remaining Blocks

- Real remote HTTP MCP server not tested; certification uses the requested single deterministic local stdio server.
- Manual desktop failure/busy/Stop and configure/quit/relaunch UI flows not verified. No UI redesign or unrelated scope work attempted.
- Stdio has client-side cancellation suppression, not server-side interruption; wrong response ID has bounded 30-second timeout, not immediate protocol failure.

**FINAL VERDICT: PARTIAL — local MCP runtime FIXED + PASS, real Agent → MCP Completed PASS; manual desktop/remote runtime gates remain unvalidated.**

Cleanup: all owned MCP test children shut down; temporary llama-server stopped. No model, credential, dependency or user MCP config created/downloaded.
