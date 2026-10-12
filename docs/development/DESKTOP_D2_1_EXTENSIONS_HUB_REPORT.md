# Desktop D2.1 — Extensions Hub

Date: 2026-10-11. Scope: native Desktop presentation and existing read-only IPC projections. No runtime, daemon, permission, Core Warden, Blender or FA-3 behavior changed. No commit, push, merge or publication performed. UX-C1 has not begun.

## 1. Architecture audit

The repository architecture, Agent Runtime, security, building and testing instructions were reviewed before implementation. Personal-Brain resolution was attempted with `brain projects brief sentinel --json`; the executable was unavailable. Work used the repository explicitly supplied by the user.

`Main.qml` owns local shell routing. `NavigationRail.qml` originally exposed Home, Models, Inspector and Settings; it did not contain a Tools destination. Production Desktop uses `DesktopControllerBridge` and `DesktopRuntimeClient`. The legacy local controller remains a fixture path, not a new authority for this page.

The existing `ExtensionService` aggregates `PluginManager`, `McpService` and `SkillService`. Its snapshots have richer metadata and actions than Desktop currently receives. `SettingsService` publishes canonical `extensions.plugin:…`, `extensions.mcp:…` and `extensions.skill:…` rows through the existing `desktop.settings_service` queries. D2.1 reads `productSettings` and, on inspection, `extensionSettingsState(id)`; it adds no IPC command or independent runtime.

Plugin discovery/lifecycle, isolated hosts, credential brokers and permission boundaries exist. [Plugins capability documentation](../capabilities/PLUGINS.md) calls this experimental, with no stable ecosystem or marketplace claim. Existing MCP handles configured servers, transports, resources/tools and connection failures; [MCP capability documentation](../capabilities/MCP.md) also calls it experimental. Runtime MCP tool execution still uses the existing registry and gateway.

`SkillService` implements discovered metadata, sources, preferences and context integration. Desktop's built-in `SkillProfileService` profiles are a separate presentation facility. Earlier certification gaps must be read together with [residual closure](PLUGIN_SKILL_RESIDUAL_CLOSURE.md); historical certification is not proof of live operation in this phase.

Runtime enabled-tool identities reach Desktop through its existing projection. Legacy tool contract/policy summaries also exist, but are not a per-`ToolDescriptor` inventory or execution grants. The hub deliberately does not join unrelated lists by index or infer authorization from discovery.

## 2. Implemented sections

All four sections share local name/ID filtering, themed cards, accessible controls, keyboard focus, scrolling, loading, empty, unavailable, error and disconnected states.

* **Plugins:** real reported IDs/names, strictly typed effective enabled/disabled/unknown status, reported issue category, and read-only availability/version/requirements inspection.
* **MCP Servers:** real reported server IDs/names and effective enablement, with the same availability inspection. Configured/enabled is explicitly distinguished from connected. Unsupported connection/capability/action details are stated rather than guessed.
* **Skills:** only daemon-reported canonical skill rows. Unknown enablement remains unknown. Discovery is explicitly distinguished from execution eligibility and Settings presentation profiles.
* **Tools:** existing reported tool identities, labeled as discovered with execution eligibility unreported. Existing registry policy summaries appear separately as contract metadata. No execution button is provided.

Availability is queried only for an ID in the current inventory. Unknown namespaces are ignored. Disconnect removes stale entries, tools and policy summaries; the local query text may remain visible but is disabled. Credentials, source payloads and unknown fields are excluded from detail presentation. Backend text in cards is rendered as plain text.

Refresh invalidates only the existing client cache for the two hub read queries. The existing polling/IPC path supplies updates. Query failures now have an error result instead of indefinite loading; hub queries retain the normal polling retry cadence. No settings store was added and no enablement preference is written.

## 3. Backend support matrix

| Facility | Backend maturity/support | Queryable by this Desktop contract | Hub behavior / missing contract |
| --- | --- | --- | --- |
| Plugin inventory | Implemented; experimental lifecycle | Settings rows: canonical ID, name, effective enablement, issue category; per-ID availability/version/requirements | Read-only; description, typed health and valid action list absent |
| Plugin configuration/actions | Existing backend operations | Generic action method exists, but supported actions per item are not advertised | Controls omitted; no fictional enable/install/configure action |
| MCP | Implemented; experimental transports/runtime integration | Extension rows and availability; richer backend state not projected here | No connected assertion; connection state, capabilities and full errors unavailable |
| Skills | Implemented discovery/preferences/context integration | Canonical discovered rows and availability | No independent execution engine; no claim that listed skills execute |
| Tools | Registry/Gateway implemented | Existing enabled-tool identities and legacy policy summaries | Per-tool description, permission requirements and runtime eligibility unavailable |
| Daemon transport | Implemented | Existing reachability/readiness projection | Disconnected state suppresses cached inventory and disables Refresh/filter |
| Empty inventory | Query implemented | No per-subsystem capability advertisement | Honest “No items reported”; cannot distinguish no installation from unavailable discovery |

## 4. Modified files

New production files:

* `apps/sentinel-desktop/include/sentinel/desktop/ExtensionsHubPresentation.h`
* `apps/sentinel-desktop/src/ExtensionsHubPresentation.cpp`
* `ui/qml/pages/ExtensionsHubPage.qml`
* `resources/icons/tabler/puzzle.svg` — UI outline glyph compatible with the existing icon loader; no official FA-3 asset changed.

Existing production files updated: Desktop CMake registration; `DesktopShellViewModel.h/.cpp` (read-only hub adapter); `DesktopRuntimeClient.h/.cpp` (existing query cache refresh/error projection); `Main.qml`; `NavigationRail.qml`.

Tests: new `tests/desktop/test_extensions_hub.cpp` and `tests/helpers/extensions_hub_certification.qml`; scoped additions to `tests/desktop/test_desktop_ipc.cpp`, `tests/helpers/settings_ui_certification_probe.cpp` and `tests/CMakeLists.txt`. Evidence is under `docs/reviews/desktop-d21-extensions/`. Other concurrent/uncommitted work was preserved.

## 5. Navigation integration

The rail now orders **Tools → Extensions → Settings**, retaining the existing bottom daemon indicator and Settings placement. Tools is a shortcut to the hub's Tools section; Extensions opens Plugins. Section selection remains inside the hub and does not write daemon navigation settings. Existing destinations and shortcuts remain intact. Escape from Tools/Extensions returns Home using the existing shell handler. Privacy & Permissions opens the existing Security settings category, avoiding a duplicate settings screen.

## 6. Native GUI evidence

The macOS native Qt certification window rendered the real rail and page with explicitly synthetic data. Screenshots are at 2× device scale: 1100×780 logical / 2200×1560 pixels for section captures, and 780×640 logical / 1560×1280 for compact disconnected capture.

Representative captures:

* [Plugins, light](../reviews/desktop-d21-extensions/plugins-light.png)
* [MCP Servers, dark](../reviews/desktop-d21-extensions/mcp-dark.png)
* [Skills, light](../reviews/desktop-d21-extensions/skills-light.png)
* [Tools, dark](../reviews/desktop-d21-extensions/tools-dark.png)
* [Compact disconnected](../reviews/desktop-d21-extensions/compact-disconnected.png)

Both theme variants of all four sections and loading/error/unavailable/disconnected captures are retained. Visually inspected captures show readable text, visible section focus/selection, intact rail icons, wrapped explanatory text and no clipped controls at the tested sizes. The test wrapper initially lacked the production theme background; it was corrected and the retained images were recaptured.

The native automated flow exercised rail routing, keyboard activation, section navigation, filtering, empty/loading/error/unavailable/disconnected states, disabled refresh and compact layout. These are GUI fixture observations, **not live plugin/MCP/skill operation**. A separate production Desktop launch against an absent test socket was attempted, but the existing single-instance guard reported “Sentinel is already running.” The user's running instance was left untouched. Full production-shell manual navigation, Linux/Windows native rendering, screen-reader interaction and live extension operation were not independently verified in D2.1.

## 7. Tests and build results

* `cmake --preset tests`: passed.
* `CCACHE_DISABLE=1 cmake --build --preset tests`: passed; subsequent scoped Desktop, probe, hub unit and Desktop IPC builds passed.
* `sentinel-desktop_qmllint`: passed (exit 0); existing context/unqualified-access warnings remain. This is not a warning-free lint claim.
* Hub unit executable: 6 Qt Test passes, 0 failures. Covers stale disconnect, distinct loading/error/unsupported/empty, namespace filtering/deduplication, strict boolean enablement, mismatched/missing details and secret-field exclusion.
* Real Desktop IPC regression reads existing settings, refreshes, handles unknown extension details, verifies no model message was sent, and suppresses stale disconnected tool state.
* Offscreen and native `--extensions-smoke`: passed, using synthetic data only.
* Branding validator: passed (7,849 source files / 69 branding files), including FA-3 hashes and selectors.
* `git diff --check`: passed.
* `ctest --preset tests --output-on-failure --timeout 60`: 124/125 targets passed in 198.84 seconds. The new Desktop IPC fixture initially omitted `server.setSettings(&settings)` and correctly received `invalid-settings-action`. Attaching in-memory Settings, matching production wiring, corrected the fixture without a daemon change. After rebuilding the affected target, `ctest --preset tests --rerun-failed --output-on-failure --timeout 60` passed `test_desktop_ipc` in 22.90 seconds. All 125 targets therefore have passing results across the full run and scoped rerun; this is not a claim of one uninterrupted green full run.

Automated assertions are distinct from visual observations in section 6. No live extension execution or remote MCP compatibility is certified by these results.

## 8. Known unsupported operations

Install/remove/update plugins, lifecycle toggles, credential editing, server connect/disconnect, MCP capability browsing, skill execution and tool invocation are omitted. Existing generic backend mutation hooks are insufficient to invent valid actions without item-specific action capability data. No second plugin/MCP/skill runtime or permission bypass was introduced.

## 9. Cross-surface inconsistencies found

* Existing “Daemon ready” terminology can mean transport/service readiness rather than verified inference readiness. The hub uses daemon connectivity only and never turns it into extension execution readiness.
* Settings presentation profiles and discovered runtime skills share related terminology but have different authorities and behavior; the hub labels the distinction.
* Runtime tool identities and legacy registry contract summaries have different granularity. They cannot safely provide a per-tool permission/execution table without a typed join contract.
* Backend extension health/actions/descriptions and MCP state are richer than the Desktop settings projection. Absence in Desktop must not be presented as backend absence or healthy connection.
* Tools is a rail shortcut into the shared hub; changing its internal section keeps the originating rail destination selected. UX-C1 should assess whether route/section selection needs one agreed convention across surfaces.

These findings are recorded only; no unrelated consistency redesign was performed.

## 10. Recommendations for UX-C1

Agree common definitions for discovered, configured, effectively enabled, available, connected, permitted and executable. Specify one typed read-only extension/tool inventory projection with explicit subsystem support/maturity, per-ID descriptions, health, sanitized errors, capabilities and supported actions before enabling management controls. Keep registry/Gateway authorization authoritative and avoid exposing credentials/source secrets. Review rail shortcut/section selection and readiness terminology across Desktop, Quick Panel, Onboarding and Settings. Add future Linux/Windows native and assistive-technology checks plus independent live extension verification when the appropriate contract and fixtures exist.

D2.1 stops here. UX-C1 and AgentRuntime changes are outside this delivery.
