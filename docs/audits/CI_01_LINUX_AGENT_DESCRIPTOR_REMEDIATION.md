# CI-01 — Linux Agent Descriptor Regression Remediation

Audit date: 2026-10-17 (Europe/Istanbul). Independent, read-only remediation in dedicated worktree
`ci-01-linux-agent-descriptors/Sentinel-sanitized`, branch `fix/ci-01-linux-agent-descriptors`.
Base commit: `70e9fa072af9b4139d86f518c6d89f9c849c29b2` (latest verified `origin/main`).
Related audits: INT-04, P1-01, P1-01.7, COR-06.

No commit, push, merge, reset, stash, PR, workflow dispatch or CI trigger was performed.

---

## 1. Root Cause Analysis

### 1.1 Four-Category Assessment

The investigation evaluated the four candidate categories defined in the remediation specification:

1. **Incorrect platform assumption in the test (Confirmed root cause of failure):**
   The test `LlmAgentRuntimeTest::allExposedDescriptorsResolveWithoutDanglingMetadata()` iterated
   over the static descriptor list returned by `BuiltInToolProvider::descriptors()` with the filter:
   ```cpp
   if (!tool.enabled || !tool.exposedToModel)
       continue;
   ```
   The test assumed that every descriptor in the built-in catalog with `enabled == true` and
   `exposedToModel == true` was exposed to the model and executable on all host platforms.
   However, `BuiltInToolProvider::descriptors()` defines the cross-platform catalog where platform
   compatibility is declared per tool via `ToolPlatformMatrix`. For `app-launch`, the platform
   matrix explicitly sets:
   ```cpp
   tool.platforms.linux = ToolPlatformSupport::Unsupported;
   ```
   (rationale: detached desktop application launching has no process sandbox guarantee on Linux;
   see [RealToolExecutor.cpp:536](../../core/src/runtime/RealToolExecutor.cpp#L536-L541)).
   In production, `LlmAgentRuntime::availableTools()` filters descriptors with:
   ```cpp
   if (!tool.enabled || !tool.exposedToModel || !toolSupportedOnCurrentPlatform(tool) || ...)
       continue;
   ```
   Consequently, on Linux `app-launch` is **never** exposed to the model. When the test synthetically
   scripted the LLM provider to request `app-launch` on Linux, `LlmAgentRuntime::nextStep()` correctly
   determined that the tool was unavailable and rejected the step, returning `decision.kind =
   AgentStepDecision::Kind::GiveUp` (Actual: 2) instead of `ToolCall` (Expected: 0). On macOS and
   Windows, `app-launch.platforms.macOS` and `app-launch.platforms.windows` are `PartiallySupported`,
   causing `toolSupportedOnCurrentPlatform` to return `true` and allowing the test to pass by host
   accident.

2. **Capability registry or descriptor inconsistency (Not the defect):**
   We investigated whether `app-launch` should have `tool.exposedToModel = false` in
   `BuiltInToolProvider.cpp`. Doing so would be inconsistent with Sentinel's capability architecture:
   `ToolDescriptor::exposedToModel` indicates whether a tool is designed for model invocation (as
   opposed to internal planner metadata like `local-plan-summary`), while `tool.platforms` specifies
   OS support. In `ToolExecutionGateway::toolMetadata()`, `!descriptor.exposedToModel` produces
   `"Tool is disabled or internal."`, whereas `!toolSupportedOnCurrentPlatform(descriptor)` produces
   `"Tool is unsupported on this platform."`. Conflating OS compatibility with the static model-visibility
   flag would corrupt gateway completeness diagnostics and hide platform unsupported status.

3. **Genuine production AgentRuntime defect (Not the defect):**
   `LlmAgentRuntime` correctly enforces capability boundaries. It advertises only supported tools in
   `availableTools()`, sends only supported tools to the planner in `input.tools = availableTools()` and
   native options `options.tools = availableTools()`, and fails closed with `AgentStepDecision::Kind::GiveUp`
   if the provider returns an unadvertised, unsupported, or unknown tool call. It does not fabricate
   receipts, does not dispatch unauthorized tools, and bounds planner repair loops to 2 attempts.

4. **Environment-dependent fixture setup problem (Contributing factor):**
   The test fixture directly iterated over the unprojected catalog `BuiltInToolProvider::descriptors()`
   rather than querying the runtime's authoritative projection `runtime.availableTools()`, and failed
   to include deterministic negative fixtures for unsupported platforms, disabled tools, and missing
   handlers.

---

## 2. Reproduction Evidence

### 2.1 Hosted CI Failure Witness (Linux CI Run 38957943894)

The hosted Linux CI job `Build and test` (ID `117165997057`) failed on commit `70e9fa07` at test step:

```text
2339: FAIL!  : LlmAgentRuntimeTest::allExposedDescriptorsResolveWithoutDanglingMetadata() Compared values are not the same
2340:    Actual   (decision.kind)                    : 2
2341:    Expected (AgentStepDecision::Kind::ToolCall): 0
2342:    Loc: [<ci-workspace>/tests/core/test_llm_agent_runtime.cpp(702)]
```

Summary: 135/136 tests passed; 1 test failed (`test_llm_agent_runtime`).

### 2.2 Descriptor Catalog Ordering and Failure Pinpointing

The 38 built-in tools in `BuiltInToolProvider::descriptors()` are ordered:
- Tools 0–14: `git-status`, `git-diff`, `stat-path`, `local-plan-summary` (skipped via `exposedToModel=false`),
  `list-directory`, `read-file`, `write-file`, `edit-file`, `grep`, `glob`, `delete-file`, `move-file`,
  `apply-patch`, `list-code-definitions`, `run-command`. All are supported on Linux.
- Tool 15: `app-launch`. In `BuiltInToolProvider.cpp:786`, `tool.platforms.linux = ToolPlatformSupport::Unsupported`.
  This is the first descriptor in the loop where `toolSupportedOnCurrentPlatform(tool)` evaluates to `false` on Linux.

When `allExposedDescriptorsResolveWithoutDanglingMetadata()` reached iteration 15 (`app-launch`),
`runtime.nextStep()` refused to project a `ToolCall` because `app-launch` is not in `availableTools()`.
The test assertion failed at line 702 expecting `ToolCall` (0) but receiving `GiveUp` (2).

---

## 3. Production versus Fixture Changes

### 3.1 Production Code Integrity

**No production code was changed.**

- `core/src/agent/LlmAgentRuntime.cpp`: Already correctly checks `toolSupportedOnCurrentPlatform(tool)`
  in `availableTools()` (line 214) and enforces tool presence in `decisionFromObject()` (line 789).
- `core/src/runtime/BuiltInToolProvider.cpp`: Already accurately classifies `app-launch` as
  `ToolPlatformSupport::Unsupported` on Linux due to the absence of a desktop process sandbox guarantee.
- `core/src/runtime/ToolExecutionGateway.cpp`: Already returns `ToolFailureCategory::Unsupported`
  and blocks unsupported execution.
- `core/src/runtime/InMemoryToolRegistry.cpp`: Already filters `enabledTools()` by platform support
  and handler availability.

Modifying production code to mark `app-launch` as supported on Linux would violate the Sentinel security
model and the rule: *"Preserve explicit permission, approval, sandbox, and platform-service boundaries.
Natural-language input must never become a shell command through heuristic planning."*

### 3.2 Fixture and Test Remediation

The remediation modifies [tests/core/test_llm_agent_runtime.cpp](../../tests/core/test_llm_agent_runtime.cpp):

1. **Authoritative Exposed Descriptors Query:**
   `allExposedDescriptorsResolveWithoutDanglingMetadata()` now queries `runtime.availableTools()`
   instead of iterating raw descriptors with an incomplete filter. It asserts that every exposed tool
   satisfies `tool.enabled && tool.exposedToModel && toolSupportedOnCurrentPlatform(tool)`.
2. **Platform-Specific Advertised Status Assertion:**
   On Linux (`#if !defined(Q_OS_MACOS) && !defined(Q_OS_WIN)`), asserts that `app-launch` is NOT
   supported on the platform and is NOT present in `availableTools()`. On macOS/Windows, asserts that
   `app-launch` IS supported and IS present in `availableTools()`.
3. **Comprehensive Negative Regression Tests:**
   Four dedicated deterministic tests cover the required regression scenarios.

---

## 4. Tests Added or Corrected

In [tests/core/test_llm_agent_runtime.cpp](../../tests/core/test_llm_agent_runtime.cpp):

### 4.1 Corrected Test

- `LlmAgentRuntimeTest::allExposedDescriptorsResolveWithoutDanglingMetadata()`
  - Verifies that every descriptor exposed by `LlmAgentRuntime::availableTools()` resolves without
    dangling metadata into an `AgentStepDecision::Kind::ToolCall` matching `toolId`, `toolName`,
    `riskLevel`, `executionMode`, and valid schema-derived arguments.
  - Verifies that `app-launch` is classified according to the host platform support matrix.

### 4.2 Added Regression Tests

- `LlmAgentRuntimeTest::unsupportedPlatformCapabilityFailsClosedWithoutDispatch()`
  - Uses a deterministic, host-independent fixture tool with `platforms.linux = Unsupported`,
    `platforms.macOS = Unsupported`, `platforms.windows = Unsupported`.
  - Verifies that the unsupported tool is never exposed via `runtime.availableTools()`.
  - Verifies that an unsolicited model tool proposal for this capability terminates with
    `AgentStepDecision::Kind::GiveUp`.
  - Verifies that `decision.toolId.isEmpty()` is true (no unauthorized dispatch).
  - Verifies that `runtime.lastDecisionUsedLlm()` is false (no invented success).
  - Verifies bounded retry loops: exactly initial request + 1 repair attempt (`prompts.size() == 2`).
  - Includes a platform-specific check for `app-launch` on Linux.

- `LlmAgentRuntimeTest::disabledAndInternalToolsAreNeverAdvertisedOrDispatched()`
  - Tests an administratively disabled tool (`enabled = false`) and an internal metadata tool
    (`exposedToModel = false`, including built-in `local-plan-summary`).
  - Verifies exclusion from `availableTools()`.
  - Verifies fail-closed termination (`GiveUp`), empty `toolId`, and no unauthorized execution.

- `LlmAgentRuntimeTest::missingHandlerOrUnavailableIntegrationFailsClosedInRegistry()`
  - Uses `InMemoryToolRegistry` with:
    1. A descriptor registered with `nullptr` handler (missing handler).
    2. A descriptor registered with a mock `IToolHandler` whose `unavailableReason()` returns a non-empty string.
  - Verifies exclusion from `registry.enabledTools()` and `runtime.availableTools()`.
  - Verifies that attempted model calls to both fail closed with `GiveUp` and empty `toolId`.

- `LlmAgentRuntimeTest::descriptorSchemaIdentityAndPlatformContract()`
  - Iterates all 38 descriptors in `BuiltInToolProvider::descriptors()`.
  - Validates schema identity: non-empty `id`, `name`, `category`, `description`, JSON Schema object type,
    properties object, and required fields subset.
  - Validates platform matrix contract: all entries (`macOS`, `linux`, `windows`) are valid enum values.
  - Asserts that `app-launch` has `platforms.linux == ToolPlatformSupport::Unsupported`.

---

## 5. Regression Results

### 5.1 Local Audit Host (macOS Darwin arm64)

| Check | Target / Command | Result |
| --- | --- | --- |
| CMake Configure | `cmake --preset tests` | OK |
| CMake Build | `cmake --build --preset tests` | OK (1517 build steps, exit 0) |
| Focused Agent Test | `test_llm_agent_runtime` | **53 passed, 0 failed, 0 skipped** (+4 new tests) |
| Tool Gateway Test | `test_tool_execution_gateway` | 9 passed, 0 failed, 0 skipped |
| Tool Registry Test | `test_in_memory_tool_registry` | 14 passed, 0 failed, 0 skipped |
| Reliability Harness | `test_agent_reliability_harness` | **97 passed, 0 failed, 1 skipped** (COR-11 assembly gap preserved) |
| Complete CTest | `QT_QPA_PLATFORM=offscreen ctest --preset tests --output-on-failure` | **136/136 passed (100%)**, 191.28 s |
| Rust IPC CLI Tests | `cargo +1.94.0 test --locked` (in `cli/`) | **141 passed, 0 failed** |
| IPC Generated Contract | `python3 tools/ipc/generate.py --check` | OK (exit 0) |
| Desktop IPC Contract | `python3 tools/ipc/generate_desktop.py --check` | OK (exit 0) |
| Personal Path Scan | `python3 tools/analysis/check_personal_paths.py` | PASS (0 files flagged) |
| Git Whitespace Check | `git diff --check` | PASS (exit 0) |
| Clang-Format Gate | `git diff -U0 origin/main -- tests/core/test_llm_agent_runtime.cpp \| clang-format-diff.py -p1` | PASS (0 violations) |

---

## 6. Linux / macOS / Windows Compatibility Risks

| Risk Area | Assessment | Mitigation / Verification |
| --- | --- | --- |
| Linux capability exposure | Zero risk of false advertisement | `availableTools()` excludes `app-launch` on Linux. Test verifies this explicitly. |
| macOS / Windows functionality | Zero degradation | `app-launch` remains supported and tested on macOS and Windows in `allExposedDescriptorsResolveWithoutDanglingMetadata()`. |
| Sandbox & permission model | Unchanged | No change to permission checks, authorization resolver, or gateway gates. |
| Agent retry bounds | Preserved | Negative tests verify the planner terminates after 2 attempts and never enters an unbounded loop. |
| Existing COR-01–18 fixtures | Preserved | Full `test_agent_reliability_harness` (97 passed, 1 expected skip) ran and passed. |

---

## 7. Remaining Hosted Validation Requirements

1. **Host-Scoped Limitation:**
   Local verification was performed on macOS (Darwin arm64). In accordance with the prompt
   guidelines (*"If Linux execution is unavailable locally, do not claim a hosted Linux fix. Prepare
   the candidate and record the verification limitation."*), hosted Linux execution has not been
   directly executed on this host.
2. **Pending Hosted CI Steps:**
   Upon maintainer approval and push of branch `fix/ci-01-linux-agent-descriptors`:
   - Linux CI (`ci-linux.yml`): `Build and test` job is expected to pass 136/136 (resolving test #104).
   - Linux CI (`ci-linux.yml`): `AddressSanitizer build and test` is expected to pass test #104.
   - macOS CI (`ci-macos.yml`): `test_llm_agent_runtime` remains green.
   - Windows CI (`ci-windows.yml`): `test_llm_agent_runtime` remains green.
3. **No Automated Actions Taken:**
   No git commit, push, merge, PR creation, or GitHub Actions workflow dispatch was executed.
