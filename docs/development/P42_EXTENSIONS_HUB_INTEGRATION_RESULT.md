# SENTINEL P4.2 EXTENSIONS HUB INTEGRATION RESULT

Date: 2026-10-15. Workspace: `<repo>`.
Branch remains `feat/p4-desktop-live-integration`; HEAD remains P3 commit `0f68c8e94bdbc09f2e31f037877e7c095839699d`.

## Result and working-tree ownership

Implemented two coherent improvements: authoritative Extensions Hub inventory and safe deactivation through existing lifecycle APIs. P4.2's configure/build and full CTest passed, with **127/127 CTest** and **130/130 Rust tests**. Native macOS Qt fixture UI and actual local MCP process fixtures passed. A production daemon in disposable portable storage returned the new inventory before and after restart.

**The final shared source tree is not certified build-clean.** During this task, runtime/tool files changed outside P4.2 (initially six; ten at the final audit). The user explicitly confirmed they belong to another concurrent task and must be preserved. An additional combined-tree build failed at `core/src/runtime/ToolExecutionGateway.cpp:283`: undeclared identifier `completion`. That source was not edited by P4.2. Concurrent source changes continued; a subsequent combined-build retry failed at arm64 linking with undefined Core symbols including OllamaModelPuller and EnterpriseConfigService. The cause of that later shared-build failure is not certified. The successful P4.2 test run used the completed P4.2 build before rebuilding those concurrent changes. No failure is concealed by the earlier passing result. Completing the concurrent task and obtaining a stable passing combined build/test remains necessary.

P4.1 production files and report/evidence retain their initial SHA-256 values. Its two added regression-test blocks remain byte-for-byte present; `test_desktop_ipc.cpp` received additional P4.2 test support and cases. No reset, stash, discard, checkout rewrite or branch creation was performed. No commit, push, PR operation, release or packaging modification was performed. Personal-Brain remains unavailable on PATH; the supplied sanitized path and active branch were verified.

Concurrent files preserved and excluded from P4.2 implementation ownership (final audit):

- `core/include/sentinel/core/runtime/RealToolExecutor.h`
- `core/src/runtime/BuiltInToolProvider.cpp`
- `core/src/runtime/RealToolExecutor.cpp`
- `core/src/runtime/ToolExecutionGateway.cpp`
- `tests/core/test_in_memory_tool_registry.cpp`
- `tests/core/test_real_tool_executor_tools.cpp`
- `core/include/sentinel/core/runtime/IToolHandler.h`
- `core/include/sentinel/core/runtime/ToolDescriptor.h`
- `core/src/agent/LlmAgentRuntime.cpp`
- `core/src/runtime/InMemoryToolRegistry.cpp`

## Existing Extensions architecture

`AgentRuntime` owns its authoritative tool registry, PluginManager, SkillService and ExtensionService. MCP services and their ToolProvider publish lifecycle/discovery into that registry. ExtensionService composes native service snapshots; it does not execute tools. `ApplicationController::extensionService()` exposes this composed service when the real AgentRuntime is present.

Plugins run outside the Sentinel process in per-plugin Native PluginHost sessions. PluginManager owns manifest validation, host lifecycle, remote tool registration and sandbox permissions. Host request brokers preserve invocation identity, resource authorization and credential isolation. MCP owns local/remote transport, connection generations, discovery, errors and interaction state; McpToolProvider replaces or removes registry entries as lifecycle changes. SkillService owns discovery, validation, source identity, requirements, workspace availability and persisted enablement preferences.

ToolDescriptor and IToolRegistry remain the tool contract; ToolExecutionGateway, approval/permission services and sandbox/resource policies remain the execution authority. Tool discovery, handler presence and an enabled extension are not authorization grants.

Existing Desktop Hub read `productSettings` and `extensionSettingsState`, plus tool-ID summaries. Those settings projections discarded most lifecycle metadata. The Hub consequently could not distinguish an enabled failed plugin from a healthy process, inspect MCP transport/staleness, or show structured tool origin/readiness metadata.

The implemented path is:

`ExtensionsHubPage → DesktopShellViewModel → DesktopRuntimeClient → typed desktop.settings_service → DaemonDesktopSettings → ExtensionService/PluginManager/MCP/Skills/ToolRegistry → bounded authoritative metadata → Hub`.

Deactivation reuses `performExtensionSettingsAction → SettingsService::performExtensionAction → ExtensionService::perform → native service`. No parallel extension architecture, new execution engine or new mutation IPC endpoint was added.

## Capability matrices

“Reachable” below means the implemented API/UI path exists and has the stated local verification. It does not certify arbitrary third-party binaries, servers or accounts.

### Plugins

| Capability | Backend classification | Hub behavior / limits |
| --- | --- | --- |
| Registered inventory, global/effective enablement | Implemented and reachable | Live ExtensionService snapshot |
| Running host, lifecycle health, sandbox state | Implemented and reachable | Running/not-running, health and sandbox metadata; installed executable availability is not inferred |
| Initialization failure | Implemented and reachable | Typed failure category; raw stderr/path omitted |
| Manifest permission and declared host-capability inspection | Implemented and reachable | Bounded metadata; never treated as granted permissions |
| Disable/unload | Implemented and reachable | “Disable for this daemon”; runtime state only |
| Enable/start/reload | Implemented but not exposed in Hub | Can start a configured process; not added in this phase |
| Persisted plugin enabled preference | Partial / unsupported by this Hub | Current PluginManager disable path is not represented as a durable preference |
| Installation/marketplace/credentials/trust changes | Unsupported in Hub | No control or new endpoint |
| Arbitrary plugin production certification | Experimental/unverified | Existing host/sandbox fixture tests only |

### MCP

| Capability | Backend classification | Hub behavior / limits |
| --- | --- | --- |
| Configured servers and transport | Implemented and reachable | Live name, transport, enablement and connection projection |
| Connected/disconnected/connecting/failed | Implemented and reachable | Uses native connection/health; available only from Core snapshot |
| Discovery changes and stale metadata | Implemented and reachable | Enabled registry-tool count, resource count and stale flag |
| Startup/connection failure | Implemented and reachable | Failure category; no command, URL, headers, tokens or stderr |
| Disconnect | Implemented and reachable | Existing lifecycle operation; daemon-lifetime effect |
| Disable | Implemented and reachable | Existing setEnabled operation; runtime config state, not a silent persistent settings write |
| Connect/reconnect/refresh remote discovery | Implemented but not exposed as Hub lifecycle buttons | Internal/core fixture reconnect verified; UI Refresh only refreshes metadata |
| Complete server capability document / retry count | Not exposed by existing service projection | Not fabricated |
| Remote authentication/account management | Unsupported in Hub | No credential or external-account operation |
| Arbitrary external MCP servers | Experimental/unverified | Controlled native stdio process fixture only |

### Skills

| Capability | Backend classification | Hub behavior / limits |
| --- | --- | --- |
| Discovered metadata and version | Implemented and reachable | Name/ID/version and requirement metadata; instruction content omitted |
| Effective availability / missing requirements | Implemented and reachable | Core health, availability and failure category |
| Workspace scope evaluation | Implemented and reachable in Core | Effective state is projected; Hub does not override scope rules |
| Disable | Implemented and reachable | “Disable skill”; existing SkillService persistence behavior retained |
| Enable / source refresh | Implemented but not exposed in Hub | No new source/network loading action |
| Validation failures retained in discovered inventory | Partial | Only records retained by SkillService can be listed; rejected imports are not invented as installed skills |
| Skill instruction/content viewer or remote installation | Unsupported in Hub | Content and source locations omitted |
| Agent availability | Partial | Requirements/effective availability shown; successful execution for every model is not implied |

### Tools

| Capability | Backend classification | Hub behavior / limits |
| --- | --- | --- |
| Registered inventory including disabled entries | Implemented and reachable | IToolRegistry::listTools, bounded rows and total count |
| Built-in/plugin/MCP/internal origin and owner | Implemented and reachable | Native descriptor fields |
| Enabled flag / registered handler | Implemented and reachable | Explicit metadata, no “Ready to execute” inference |
| Permission category, risk, model visibility, metadata-only mode, maturity | Implemented and reachable | Authoritative descriptor metadata |
| Per-invocation resource permissions, approvals, capability/platform eligibility | Implemented in gateway; not fully projected in Hub | “Invocation authorization required”; gateway remains authoritative |
| Execute/grant/override policy from Hub | Unsupported | No execution or permission-changing control |

## Verified defects and implemented functionality

### 1. Lifecycle information was lost at the Desktop boundary

ExtensionService already distinguished health, effective/global enablement, availability, transport, running host, stale inventory, requirements and failure category. The settings-derived Hub retained essentially enablement and IDs. A failed-but-enabled plugin/server could therefore be indistinguishable from a functioning one.

Added read-only `extensionsHubState` to the existing generated settings-service action whitelist. `ExtensionService::managementSnapshot()` serializes native snapshots and tool descriptors with explicit bounds: 64 extensions, 128 tools, at most 12 entries per permission/requirement/capability list, bounded strings and a 180,000-byte row budget. Counts and a truncation flag distinguish bounded inventory from a full listing. Oversized IDs are omitted rather than mutated into actionable identities.

No source path, URL/header, credential state/value, skill content or raw failure detail is serialized. The presentation layer uses a second field allowlist, checks boolean types and filters management actions to disable/disconnect. Unknown service availability becomes an unavailable/error phase. Empty authoritative inventory is distinct from service failure.

Shell detail reads the selected row from this same safe inventory rather than querying credential-bearing extension settings. Rows show native health, maturity, transport/connection, host/sandbox state and failure categories. Detail exposes bounded requirements, requested permissions, declared capabilities and counts. Tools show origin/owner, enabled state, handler registration, permission category and risk. Layout/navigation/components were preserved.

### 2. No safe management loop or refresh-response barrier

The Hub exposed no lifecycle controls. Explicit Refresh removed cached settings values but did not invalidate pending reads; an older response could repopulate invalidated metadata. Periodic query-marker clearing could also submit another read while the same query was pending.

Added client-side management state and a deactivation-only allowlist. Both ID and operation must occur in the currently received authoritative inventory. The existing daemon independently validates arguments, rejects mutation while runtime work is active, and ExtensionService checks its current availableActions before invoking a native service. The UI does not grant permission or trust.

Only one Hub mutation can be outstanding per Desktop client. All lifecycle buttons and Refresh are disabled while it is pending. The actual daemon response produces accepted/rejected state; transport failure/timeout produces **unconfirmed**, never a fabricated success. There is no mutation replay. A late response to a timed-out request cannot change that outcome.

Explicit Refresh invalidates matching in-flight read correlations as well as cached query values/markers. Old and duplicate responses cannot refill the current inventory. Query submission also checks whether the same request is already pending. Disconnect clears Hub cached inventory/action state; ready/connection gates hide stale data. Existing periodic daemon projection updates trigger new Hub reads while the page is visible.

An accepted deactivation triggers a fresh authoritative read. Accepted is not used as a substitute for native lifecycle health. Plugin/MCP deactivation is labeled daemon-lifetime; skill disable uses its existing durable preference contract. Full lifecycle enable/connect/reload is visibly described as unavailable in this Hub package.

## Security and sandbox verification

No policy, sandbox, filesystem/network/process grant, credential backend, invocation identity, approval rule or resource ownership contract was modified by P4.2. New IPC capability is read-only within the existing typed settings envelope. Mutation uses an existing endpoint and two existing capability-reducing actions; no process launch, installation, trust mutation or credential API was added to the UI.

The daemon's active-run mutation guard and ExtensionService's native availableActions checks remain in force. ToolExecutionGateway is not bypassed by listing or deactivation. Built-in/plugin/MCP tool origin is descriptor metadata, not a grant. Existing denial, sandbox, host broker, stale invocation, cancellation and late-result regression suites remain separate execution evidence.

Host test access was used for local sockets, isolated Qt storage and controlled test executables. This does not change product permissions. The native UI fixture requested no notification permission and used synthetic metadata; no existing personal Sentinel window was operated on.

## New and extended tests

- `managementSnapshotIsBoundedAndDoesNotGrantPermissions`: available/disabled skill, private-content omission, missing handler, permission/risk metadata and inventory/frame bounds.
- `managementSnapshotPluginFailureAndDisabledState`: discovered disabled plugin and missing-binary failure; no available/running claim; no private source, credentials or raw failure detail.
- `extensionsSnapshotFollowsRealProcessLifecycle`: actual MCP stdio fixture, eight tools, disconnect/stale metadata, reconnect, disable and registry invalidation.
- `extensionsLiveInventoryAndSafeSkillDeactivation`: actual daemon socket → safe inventory → persisted skill disable → refreshed Disabled/unavailable state. Unknown IDs, unsupported enable, duplicate pending action and disconnect are rejected/cleared.
- `extensionsDesktopMcpRealProcessDisconnectAndRecovery`: Desktop client → real daemon IPC → Core ExtensionService → actual MCP process; eight tools, disconnect, reconnect and disable. Persistent MCP configuration is unchanged.
- `extensionsRejectStaleReadsAndSettleActions`: accepted/rejected/timeout rows, explicit refresh, old/duplicate inventory responses, duplicate actions and late action acceptance. A post-action query acts as a protocol barrier; no arbitrary sleep is used.
- `liveInventoryUsesLifecycleAndOmitsPrivateFields`: safe field/action filtering, typed availability and private-data omission.
- Extensions QML fixture: backend-supported deactivation, keyboard action, one request, pending-control disable, unconfirmed outcome text, plus existing four-section navigation, empty/error/disconnection, focus and compact-layout checks.

Initial test-only failures were diagnosed and corrected: the live-skill fixture needed an AgentRuntime; the new plugin fixture declared PluginManager before its borrowed registry, causing destructor-time use-after-free. Registry lifetime now exceeds PluginManager lifetime. These are fixture corrections, not claimed production defects.

## Local validation and evidence boundaries

| Check | Result |
| --- | --- |
| CMake configure | PASS |
| P4.2 build with `CCACHE_DISABLE=1` | PASS |
| Complete CTest on completed P4.2 build | **127/127 PASS**, 150.22 seconds |
| Focused extension/MCP/skills/Desktop/QML tests | **5/5 PASS**, before the added full Desktop MCP process case; that case is included in final full CTest |
| Rust 1.94.0 locked tests | **130/130 PASS** |
| Rust formatting / Clippy `-D warnings` | PASS |
| QML lint, 72 files | Exit 0; **113 warnings**, same count as P4.1; no ExtensionsHubPage warning |
| Both IPC generators `--check` | PASS |
| Scoped clang-tidy | Zero errors; totals/classification below |
| Native macOS Cocoa Qt UI fixture | PASS; synthetic data, keyboard/compact/pending/unconfirmed checks |
| Disposable production daemon | PASS; authoritative 0-extension/35-tool inventory and post-restart query |
| Additional shared-tree build | **FAIL**: initial ToolExecutionGateway compile diagnostic, then undefined Core symbols at arm64 linking; concurrent work preserved |
| Hosted CI | Not used |

CTest target total remains 127 because regression rows were added to existing executables. Seven pre-existing QtTest subcases skipped: Linux missing-secret-helper contract; four macOS strict-sandbox child-process shell fixtures (`asyncRunCommandStreamsAndContinues`, `asyncRunCommandCancellationAndTimeout`, `asyncDockerUsesProcessExecutorAndPreservesRestrictions`, `shutdownStopsActiveCommand`); opt-in production observation classification; and missing-npx branch because npx is installed. Exact reasons are captured in the validation summary.

Evidence categories:

- **Mocks/wire fixtures:** request ordering, timeout, duplicate/stale response, deactivation rejection, presentation/privacy and synthetic native UI states.
- **Real-process fixtures:** MCP native test server, actual daemon IPC service and existing PluginHost/sandbox integration tests. These are controlled local processes, not arbitrary external MCP servers or production third-party plugins.
- **Real production executable:** copied built daemon under an owner-only temporary portable profile/socket; new Hub query returned 0 extensions and 35 tools, no private fields, then worked after a changed-generation restart. This verifies empty production inventory and read-only reachability, not an installed production plugin or external account.
- **Native GUI:** Cocoa test window with real QML components and synthetic data. Full production Desktop-to-real-installed-extension acceptance is unverified. Native Linux/Windows certification remains deferred.

No inference, marketplace, remote account, cloud credential or external server certification is claimed. Whole-repository static analysis, ASan/coverage, full remote transport certification and cross-platform native acceptance were not performed in this phase.

Artifacts: [validation summary](p42-evidence/validation-summary.json), [production daemon smoke](p42-evidence/production-daemon-smoke.json), [static inventory](p42-evidence/static-analysis.json), [P4.2 source hashes](p42-evidence/p42-source-manifest.json), [native pending-action fixture](p42-evidence/management-pending.png), [native unconfirmed-action fixture](p42-evidence/management-unconfirmed.png).

## Static-analysis classification

Warning counts refer to unique emitted source diagnostic lines in the selected scope, not the analyzer's aggregated/suppressed system-header counts.

| Scope | Before | Final | Classification |
| --- | ---: | ---: | --- |
| Same three P4.1 sources, P3 source baseline copied to temporary analysis files | 105 | 107 in preserved P4.1 | Two P4.1-added braces/readability warnings in RemoteAgentInspectorService; no functional diagnostic |
| DaemonDesktopSettings.cpp | 36 | 36 | Existing |
| DesktopRuntimeClient.cpp | 8 | 7 | One existing warning removed by scoped replacement |
| DesktopShellViewModel.cpp | 156 | 153 | Three existing warnings removed by scoped replacement |
| ExtensionsHubPresentation.cpp | 16 | 16 | Existing |
| ExtensionService.cpp | 58 | 58 | Existing; new null-registry analyzer finding resolved by explicit guard |
| Five P4.2 implementation sources | **274** | **270** | **Zero new final diagnostics** |
| Combined eight-source scope | **381** at P4.2 start | **377** | 375 P3-existing + 2 preserved P4.1 readability warnings |

Warnings on added/replaced P4.2 source lines: **0**. Remaining warnings in the five modified implementation files: **270**. The intermediate null-registry diagnostic was corrected; no suppression or broad cleanup was added. Baseline reconstruction used `git show HEAD:<file>` into `/tmp` and an analysis-only compile database, without changing any historical checkout. These totals do not assert a whole-repository warning count, and exclude the other task's modified runtime/tool files.

## Changed files owned by P4.2

Production/contracts:

- `core/include/sentinel/core/extension/ExtensionService.h`
- `core/src/extension/ExtensionService.cpp`
- `apps/sentinel-daemon/service/DaemonDesktopSettings.cpp`
- `apps/sentinel-daemon/service/DesktopProjection.generated.h`
- `apps/sentinel-desktop/include/sentinel/desktop/DesktopRuntimeClient.h`
- `apps/sentinel-desktop/include/sentinel/desktop/DesktopShellViewModel.h`
- `apps/sentinel-desktop/src/DesktopRuntimeClient.cpp`
- `apps/sentinel-desktop/src/DesktopShellViewModel.cpp`
- `apps/sentinel-desktop/src/ExtensionsHubPresentation.cpp`
- `protocol/ipc-v1.json`
- `protocol/QtIpcContract.generated.h`
- `ui/qml/pages/ExtensionsHubPage.qml`

Tests:

- `tests/CMakeLists.txt`
- `tests/core/test_mcp_integration.cpp`
- `tests/core/test_skill_extension_runtime.cpp`
- `tests/desktop/test_desktop_ipc.cpp` (P4.1 tests preserved)
- `tests/desktop/test_extensions_hub.cpp`
- `tests/helpers/extensions_hub_certification.qml`
- `tests/helpers/settings_ui_certification_probe.cpp`

This report and `p42-evidence/` are P4.2 documentation/evidence. Remaining P4.1 files and the user-confirmed concurrent files are not P4.2 modifications.

## Remaining limitations and P4.3 readiness

P4.2 functionality is locally verified within the stated snapshot/fixture boundaries. Shared-tree readiness is conditional on completion of the concurrent work and a passing stable combined build/CTest. No commit is authorized or created.

The Hub remains intentionally deactivation-only. Enable/connect/reload, source refresh, remote installation, credentials and permission/trust mutation are not available. Plugin/MCP disable is runtime state, not durable configuration. Inventories are bounded and polled; complete histories, rejected skill imports, MCP capability documents and per-invocation/platform readiness are not fully exposed. A timeout is unconfirmed: the backend may have applied a synchronous action, so the refreshed state must be consulted.

P4.3 should establish durable extension configuration/lifecycle ownership before adding process-starting controls. Define explicit authorization and pending/completion semantics for plugin enable/reload and MCP connect/reconnect, preserve sandbox/grant isolation, and test state restoration after daemon restart. Add complete paginated inventory if production registries exceed current bounds. Validate the actual Desktop against existing approved plugins and controlled MCP servers on native macOS/Fedora KDE; keep marketplace installation and credential workflows separate. The P4.1 provider/model readiness follow-up remains outstanding.
