# SENTINEL — PLUGIN / SKILLS / EXTENSIONS RUNTIME CERTIFICATION REPORT

Date: 2026-10-03 (Europe/Istanbul). This phase is **not closed**: its minimum Agent→Plugin and Skill→model context gates are unmet. Existing dirty-tree cross-provider/MCP/sandbox work was preserved. Personal-Brain command was unavailable; supplied repository/local architecture instructions used.

## 1. Architecture

- Plugin: executable Qt native ABI v5 extension. PluginManager parses directory plugin.json manifests, resolves identity/version/dependencies/permissions, owns lifecycle and namespaced registry registration. Native module objects are never exposed in the desktop/core process (`pluginInstance` returns null).
- PluginHost: separate `sentinel-plugin-host`, protocol v3 newline JSON framing, health/ABI handshake, module load/init/start/invoke/shutdown. PluginHostSession uses ProcessExecutor with enforced sandbox, no raw network, bounded writable plugin data directory, and mandatory detached-child control.
- Brokers: reverse IPC accepted only for current invocation + owning plugin + current host generation. Filesystem broker revalidates frozen authorized path and access. Network broker validates URL/method, current network mode, manifest permissions and exact invocation resource. Process broker validates program/arguments/workspace authorization and starts sandboxed ProcessExecutor child.
- Credential: no SDK secret getter. Scoped credential reference may be consumed by network broker only with declaration, permission, invocation authorization and allowed-host match. Sentinel injects Authorization header; actual secret not returned by a credential-read API.
- Skills: separate markdown instruction/metadata service, embedded/directory/HTTPS source types, enable preference, requirements and global/workspace scopes. Not executable plugins. SkillProfileService's built-in profiles are a different service, not evidence that directory SkillService content reaches the planner.
- ExtensionService: product state/action umbrella over plugin, MCP and Skill owners; it is not an alternative execution gateway or security authority.
- Authoritative plugin tool path: manager discovery → host tool declarations → ToolDescriptor/registry → ToolArgumentValidator → ToolExecutionGateway → permission/approval/security → host invocation → typed result → Agent observation. No bypass added.

## 2. Real Plugin

- Existing staged sample: `dev.sentinel.plugin.custom-tool`, Custom Agent Tool, version 1.0.0, API 5.0, vendor Sopwit Community, entry custom_agent_tool_plugin. Declared tool.execute and filesystem.read. Second sample `dev.sentinel.plugin.second-tool` also present.
- Sample tools: echo and delayed_echo, required string text, additionalProperties false, Medium risk. PluginResult contract is bool ok + string summary, not a new structured-result API.
- Actual manifests discovered and metadata read. Native host launch attempted through production manager/session.
- Actual result on this macOS: **PluginSandboxUnavailable**, because mandatory forbidDetachedChildren cannot be enforced (`DetachedProcessControlUnavailable`). Host is not launched; no registry tools are admitted. No in-process loading or weaker sandbox fallback.
- Docker executable exists, but daemon socket is unavailable. No alternate Linux runtime, image download or dependency installation attempted.
- Current PluginManifest has vendor/description/dependencies/permissions/capabilities/credential declarations; author/license/source/homepage are not typed manifest fields. No new mandatory license/compliance schema added.

## 3. Plugin Execution

- Direct call 1/2: **BLOCKED**, host cannot start under current enforced macOS contract. Sample capability presence is not actual execution evidence.
- Registration/execution/ID-collision behavior remains covered by existing suites where host can launch; here four plugin-integration functions skip at the launch gate. They are not runtime PASS.
- Additional real manifest-file coverage rejects malformed JSON, missing required identity, incompatible API, duplicate IDs and silent identity replacement. No native module is inspected through a desktop loader.

## 4. Plugin Security

- Filesystem: broker supported; source audit confirms declared permissions, per-invocation resource snapshot, exact argument/path/access, path revalidation and ExternalDirectoryGate. Actual authorized/unauthorized plugin broker calls **NOT VALIDATED** due host launch block.
- Network: broker supported; URL/resource/method checks, manifest permissions, current network mode and no unrestricted host network. Controlled-endpoint actual plugin calls **NOT VALIDATED**.
- Process: broker supported; exact executable/arguments authorization and sandbox enforced. Harmless child via plugin **NOT VALIDATED**.
- Credential: scoped network-injection contract supported; no plaintext SDK secret-store reader. Actual no-permission/approved-reference/unrelated-credential broker matrix **NOT VALIDATED**. No real secret printed or credential created.
- All four brokers are implemented, so these are unclosed runtime gates, not NOT SUPPORTED claims.

## 5. Permissions

- Critical reproduced failure: a session FileSystem grant for provider plugin:one authorized same-resource request from plugin:two. Persistent grant record also lacked plugin ownership.
- FIXED in PermissionService: plugin owner participates in match, replace, revoke and persistent deduplication. Plugin grants cannot authorize another plugin or built-in request; ownerless built-in grants cannot authorize a plugin.
- SQLitePermissionGrantStore stores plugin_owner_id, transactionally migrates existing schema to version 2. Legacy ownerless grants retain ordinary built-in meaning, do not become plugin grants.
- Actual isolated SQLite disk round-trip verifies same owner Allow, other owner Ask, other session Ask for session-only grant; persistent grant allows same owner in a new session and not another owner. Legacy schema migration tested.
- General policy/approval suites PASS. Actual plugin-host Allow/Ask/Deny and brokered session/persistent execution remain blocked. No blanket allow or manifest permission widening.

## 6. Plugin Failure Isolation

- Launch denial: truthful typed PluginSandboxUnavailable, core process survives, no fallback success.
- Crash/malformed/wrong-ID/unexpected-result/timeout while a real host is running: **NOT VALIDATED**, because no safe host launch is available.
- Source contract: malformed envelope/protocol/wrong pending ID produces PluginProtocolError; call timeout produces PluginTimeout; process exit produces PluginCrashed; pending callbacks drained, broker operations cancelled. Cancellation kills synchronous plugin host.
- Recovery contract includes explicit unload/reload and hot reload. No automatic restart was added or claimed. Active-host recovery not tested.
- Process listing after attempts found no sentinel-plugin-host process; no orphan from certification.

## 7. Agent → Plugin

- Intended provider/model: proven llama.cpp sentinel-nemotron.
- Tool/result/grounded final: **BLOCKED** before model tool execution, because real plugin host/tools cannot be admitted.
- Terminal: no real Agent→Plugin Completed claimed. Existing synthetic planners in plugin integration tests do not satisfy this gate; their macOS skips are reported.

## 8. Skills

- Real markdown fixtures in tests/fixtures/skills discovered through actual SkillService. Name/description/version/author/instructions read; refresh does not duplicate registrations.
- Enable/disable: getter content present/absent correctly; preference written to isolated disk, service destroyed/recreated, Disabled restored; re-enable and workspace override tested.
- Scope: global and workspace-a fixture. Outside workspace-a, MissingRequirements and no instruction content; matching workspace makes content available. Workspace target identity preserved in ExtensionSnapshot after fix.
- Unsupported scopes are marked incompatible; session-specific SkillScope is not implemented. GlobalWithWorkspaceOverride uses current workspace preferences. New precedence rules not invented.
- Context gate: **FAIL / NOT CONNECTED**. SkillService is owned/discovered by AgentRuntime but its content getters have no planner/context callers; AgentContextInput/ContextEngine have no instruction-skill input.
- Actual llama.cpp Agent probes loaded enabled/disabled fixture. Enabled content_available=true, disabled false. A forwarding audit on actual provider requests reported skill_marker_in_model_request=false in intent, planning and continuation for both. No prompt injection workaround.
- Enabled/disabled filesystem Agent probes reached Completed with real list-directory observation, but neither produced [SKILL_OK]. This is not Skill-context PASS. Some native outputs miscategorized file entries as directories; this phase does not newly certify semantic correctness of those categorization claims.
- Earlier tool-free diagnostic Agent tasks failed grounding and were not counted as successful Skill runs.
- Duplicate addSkill silently overwrote instructions before fix; now rejected. Empty content rejected. Discovery's existing conflicting-source guard preserved. Simplified frontmatter parser can fall back to filename/raw markdown for unclosed frontmatter; no new skill format/parser feature added.

## 9. Skill Security

- Skills expose content and metadata, not grant/sandbox/tool registry/credential APIs. Loaded content does not change runtime permission state.
- Actual local instruction loading remains available under Offline and LocalOnly; external network endpoints remain blocked, loopback permitted according to current policy.
- Workspace-scope non-leak and requirement gating PASS in new owner suite.
- End-to-end malicious Skill prompt vs denied tool/resource/credential: **NOT VALIDATED**, because Skill instructions do not reach model context. Absence of injection is not claimed as a successful behavioral security implementation.

## 10. Extensions

- Snapshot identity/status/enable/disable tested through real ExtensionService and SkillService; actions affect owner and preferences, not grant authority.
- Plugin-manager failure does not corrupt usable Skill content; invalid Skill insertion does not add/remove plugins.
- Workspace identity overwrite fixed: workspace-a Skill remains workspace-a even while active workspace is workspace-b, and remains unavailable there.
- Skill enable preference persistence and plugin-scoped explicit grant persistence PASS.
- PluginManager enabled/disabled state itself is in-memory; no independent plugin-global preference disk persistence owner found. Workspace extension preferences are separate product state. No persistence feature added.
- Desktop configure/quit/relaunch not performed; isolated store recreation evidence is reported separately as requested.

## 11. Offline / LocalOnly

- Disk Skill discovery/content works in these modes; does not widen external network access.
- NetworkPolicyService permits loopback under both modes. Remote Skill fetch checks policy before request.
- Local plugin execution could be network-independent in its contract, but current host launch block prevents actual mode-specific execution tests. Network/process brokers remain unvalidated, not inferred PASS.

## 12. Defects Found

1. SkillService addSkill overwrote existing name silently. Fix: reject duplicate. Regression reproduced failure then passes and retains original instruction.
2. ExtensionService overwrote declared workspace Skill owner with active workspace. Fix: preserve Workspace-scope identity. Regression verifies target + unavailable status across workspace switch.
3. PermissionService/SQLite store ignored plugin owner. Fix: owner-scoped session/persistent grants and safe legacy schema migration. Actual two-owner session and disk round-trip regressions reproduce/pass; no permission widening.
4. PluginManager discovery silently selected/replaced duplicate plugin IDs. Fix: duplicate inventory becomes PluginDuplicateId Error with no executable load; cross-root identity collision rejected. Actual malformed/missing/incompatible/duplicate manifest fixtures tested.

Remaining capability gaps are not hidden as fixes: macOS detached-child plugin isolation and missing SkillService→ContextEngine integration. Implementing absent context selection/precedence wiring would expand existing feature behavior, so it was not added under the no-new-features scope.

## 13. Automated Regression

- Configure/build: PASS.
- Plugin manager/hooks and shared permission/registry/validator/gateway/sandbox/credential suites: CTest PASS, with **five macOS plugin runtime skips** (four integration functions, one sample-loading function).
- New test_skill_extension_runtime: **9 passed / 0 failed / 0 skipped** including init/cleanup. Covers real loader/prefs/scope, ExtensionService, manifest failures and plugin-grant/store boundaries.
- SkillProfileService/context/AgentLoop/LlmAgentRuntime/AgentRuntimeService and Chat/provider suites: PASS.
- MCP provider and real stdio integration: PASS in final full run; no MCP feature changes.
- Full suite: **104/104 CTest targets PASS**, 101.03 seconds; one new target, no failures. This does not convert skipped runtime gates to PASS.
- ApplicationController: **127 passed / 0 failed / 0 skipped**.
- git diff --check: **PASS**.

## 14. Remaining Blocks

- Mandatory positive isolated plugin-host execution, all implemented brokers, failure/recovery runtime and real Agent→Plugin Completed require an environment capable of enforcing current host isolation. macOS rejects it truthfully; no safe Linux runtime currently available.
- Skill context injection is not connected in current architecture. Minimum instruction-visible real Agent behavior and security prompt matrix therefore cannot pass.
- Plugin enabled-state independent persistence and session-specific Skill scope are not existing supported contracts; no new features added.
- Manual desktop relaunch/UI failure/busy flows not performed.

## Certification matrix

| Area | Result |
| --- | --- |
| Plugin Discovery | FIXED + PASS |
| Plugin Host Isolation | fail-closed denial PASS; positive isolated execution BLOCKED |
| Plugin Execution | BLOCKED |
| Plugin Permissions | owner boundary FIXED + PASS automated; broker runtime BLOCKED |
| Agent → Plugin | BLOCKED |
| Skill Discovery | FIXED + PASS |
| Skill Context | FAIL / NOT CONNECTED |
| Skill Security | API/scope/network tests PASS; end-to-end prompt boundary NOT VALIDATED |
| Persistence | Skill + plugin-owner grants PASS; independent plugin enable state NOT IMPLEMENTED |

**FINAL VERDICT: PARTIAL — four proven defects fixed and 104/104 regression green; minimum phase closure criteria NOT MET.**

Cleanup: isolated skill preference fixture files removed after probes; no user credential or plugin/Skill preferences edited. Temporary llama-server stopped. No plugin host orphan, new provider, dependency download or security relaxation.
