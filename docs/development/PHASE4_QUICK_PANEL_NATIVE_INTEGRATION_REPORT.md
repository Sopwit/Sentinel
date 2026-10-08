# SENTINEL — PHASE 4 QUICK PANEL + NATIVE DESKTOP INTEGRATION REPORT

Validation date: 2026-10-06. Host: macOS, Qt 6.11.2. **FINAL VERDICT: PARTIAL.**

Implementation and regression checks are complete. Several required native acceptance gates could not be certified with the available macOS UI automation. The overall phase is not a native PASS. Windows and Linux runtime checks remain deferred as requested.

## 1. Architecture

`NativeCompanionAdapter` owns the single production tray icon, activation, menus, shortcut registration, native notifications and client lifecycle. `QuickPanelController` projects presentation state and forwards actions through the existing `DesktopRuntimeClient`. It creates no second transport, model service, permission store or runtime. QML presents this state. Chat, Agent, workspace selection, approvals, cancellation and terminal acceptance remain daemon/core responsibilities.

The protocol source adds `currentWorkspaceName` and a read-only `networkMode` setting projection. Generated bridge/daemon contracts remain generated from `protocol/ipc-v1.json`; the desktop generator now supports read-only settings.

## 2. macOS

Menu bar: implemented with `QSystemTrayIcon`. Icon: production `:/branding/tray.png`, `QIcon::setIsMask(true)`; the full app icon is not used. Left click toggles the panel; context activation shows a separate menu. Native menu-bar visibility, light/dark template appearance and direct icon clicks are **not visually certified**: available captures expose the app window rather than the complete menu bar.

Panel anchor: tray geometry selects the screen; position and size are clamped to available geometry, with cursor-screen fallback. Multi-monitor behavior is statically reviewed, not physically tested. Keyboard activation opened the actual panel with prompt focus. Escape/toggle behavior is implemented; native results are recorded below.

## 3. Windows

Production tray icon, left-click panel activation, context menu, separate main-window activation and Quit are implemented in platform-neutral Qt. `RegisterHotKey`/`WM_HOTKEY` provide the optional shortcut, with explicit `user32` linkage and unregister on teardown. Run-key startup registration uses `QSettings`. Source review only; neither compilation on Windows nor native tray validation is claimed.

## 4. Linux

Qt `QSystemTrayIcon` supplies desktop-supported tray integration, including compatible StatusNotifier environments. No KDE-only core logic is introduced. Unsupported/headless environments report native companion unavailable and retain a visible main-window route. KDE Plasma, GNOME with/without extensions, Wayland and X11 require native validation. A global shortcut portal implementation is not present; the setting reports unavailable rather than claiming registration. Autostart uses an escaped `.desktop` Exec entry.

## 5. Quick Ask

The panel reuses the daemon-backed current conversation. Quick Ask sends the same `chat.send` authority path as the main client. Only a bounded 2,000-character preview is displayed; no conversation-history surface is added. Streaming, terminal errors and cancellation use existing IPC events.

Real native Nemotron Chat returned `SENTINEL_PHASE4_OK`. An immediate subsequent Chat cancellation was observed as cancelled, with input usable again. A longer request that finished before cancellation was not counted as a cancellation test. Deterministic tests cover submission, streaming, cancellation/recovery and continuation identity.

## 6. Quick Agent

Quick Agent forwards `agent.start` through the same daemon runtime. Starting/running/approval/terminal state, tool summary, cancellation and main-window continuation are available. Active Agent output previews are suppressed to avoid exposing planner output; only terminal response text is displayed.

A real safe read-file task read the disposable workspace's `CMakeLists.txt`; the accepted terminal answer included the observed `project(SentinelPhase4Fixture LANGUAGES CXX)` content and source. Agent cancellation was also observed. An earlier model attempt that failed to produce an accepted final answer was cancelled and was not counted as successful completion.

## 7. Approvals

The panel aggregates daemon session snapshots, including approvals in another conversation, and targets the cached session/run/approval ID. It shows tool, resource, risk and detail, with Allow Once, Deny and Open actions. No new session or persistent grants exist. Pending responses disable repeated clicks; daemon validation remains authoritative.

Real native approval display and Allow Once were observed with a loopback web-fetch followed by a grounded read-file. Validation exposed a stale card after acceptance: the daemon retained its old approval payload after clearing the ID. This is fixed by clearing the payload on acceptance and rejecting snapshot payloads without live approval state/ID in the client. Tests cover allow/deny/cancel, other-session forwarding, reconnect, late terminal rejection, planner-output suppression and stale snapshot rejection. A previous Deny click on an expired card correctly produced `invalid-approval`; it is not counted as real Deny success. After the fix, a fresh real approval was denied through the panel: the main conversation showed “User denied the required action”, the run failed as expected, pending count returned to zero, and the prompt recovered.

## 8. Voice

PTT and audio level: unavailable. The daemon contract does not expose native capture here. The disabled Voice/PTT control and explanatory label state that limitation; there is no fabricated level or waveform.

## 9. Runtime State

Workspace comes from authoritative WorkspaceService selection through the daemon projection. Model/provider come from daemon state; while a run is active, the panel shows its frozen binding IDs. Network mode is a read-only daemon setting. Connection displays the transport enum: Disconnected, Connecting, Connected, Reconnecting, VersionMismatch, Unavailable (and shutdown state). Actions require both projection readiness and an attached session, so a connected socket alone does not enable execution. Native startup briefly exposed Connected with unavailable projection and disabled actions; real model/workspace values appeared after initialization. This startup delay is a remaining performance concern.

## 10. Global Shortcut

Default: **Disabled**, to avoid claiming an OS-wide chord by default. Configurable supported choices: Ctrl+Alt+Space and Ctrl+Alt+S. macOS uses Carbon registration; Windows uses RegisterHotKey. Failure status is visible in Settings and logged; unsupported choices/platforms report their limitation. Registrations are removed on replacement and teardown.

Native Settings visibly reported **Registered** for the explicit test-profile Ctrl+Alt+Space preference. Actual system-wide toggle is **not certified**. The available Computer Use `press_key` targets an application and explicitly cannot invoke global shortcuts, so app-scoped shortcut activation cannot establish this acceptance gate.

## 11. Notifications

Approval and Agent terminal transition notifications are deduplicated by approval ID/run state. Existing DesktopShell notifications route through the same native icon, preserving their existing policy/cooldown path. Quick Panel notifications respect notificationPolicy and custom Agent-response preference. Click routing uses the associated session or existing main-surface category.

OS delivery, macOS notification authorization and actual notification-click routing remain **unverified**. Wiring alone is not reported as native notification PASS.

## 12. Lifecycle

With companion enabled and a usable tray, closing the main window hides it and keeps the client alive; otherwise normal last-window behavior applies. Quit exits the client through QApplication, removes shortcuts and hides the icon. Daemon ownership is unchanged. Disabling companion restores a hidden main window.

Startup is an explicit local presentation preference: macOS LaunchAgent, Windows Run key, Linux autostart. Preferences are changed only after successful registration; headless calls report unavailable. Existing enabled preferences do not claim that OS registration was verified. No real login registration was written during validation. Normal Quit/relaunch was exercised with an isolated profile. Clicking the main Close control kept the owned client PID alive; subsequent window inspection timed out, so the precise hidden-window appearance is not certified.

## 13. File / Deep Link

`QFileOpenEvent` routes a bounded sentinel URL or reveals the main window for an ordinary file; it does not import/execute the file. Single-instance forwarding uses user-only local-server access and a bounded asynchronous buffer.

Supported links: `sentinel://settings` and `sentinel://session/<id>`. Maximum 512 characters; session ID is 1–128 ASCII alphanumeric/underscore/hyphen characters. Credentials, port, query, fragment and unsupported targets are rejected. Session links require a ready daemon. Broad run/action protocols and cold-start session-link queuing are not implemented. Platform file association installation is not claimed.

## 14. Real macOS E2E

| Gate | Evidence/status |
| --- | --- |
| Build and native launch | PASS; real desktop bundle, isolated daemon/profile |
| Menu-bar icon / light-dark / direct click | UNVERIFIED |
| Keyboard panel activation and input focus | PASS via existing application Ctrl+Shift+C |
| Quick Ask and streaming preview | PASS with existing Nemotron |
| Chat cancel and recovery | PASS |
| Safe grounded Quick Agent | PASS |
| Approval display / Allow Once | PASS observed before final stale-card fix |
| Real Deny | PASS on the final implementation; expected failed run, no pending approval |
| Continue / Open Sentinel / Settings | PASS through actual panel buttons; conversation identity also tested |
| Real daemon reconnect | PASS: Reconnecting/disabled actions, then Connected/usable prompt |
| Escape | Native close not certified; explicit window handler and regression test added |
| Global shortcut | UNVERIFIED; automation cannot invoke system-wide chord |
| Native notification delivery/click | UNVERIFIED |
| Multiple monitors / login startup | DEFERRED |
| Quit/relaunch | PASS through normal application Quit |

The existing local model was used; no model was downloaded. Native evidence/logs reside in ignored `build/phase4-native/`, not committed assets. Automated fixtures are separate from these real-model observations.

## 15. Tests

Desktop IPC: **38 passed, 0 failed, 0 skipped**, including Quick Panel validation and deterministic daemon-backed Chat/Agent actions. Native integration: **3 passed, 0 failed, 0 skipped** (Qt init/body/cleanup), covering Settings/main activation routing, continuation activation, local shortcut preference and truthful headless startup/shortcut unavailability, and native-window Escape event handling. These headless tests do not exercise a real tray or notification click. Existing transport tests cover subscriptions, generation filtering, reconnect and no local fallback.

## 16. Regression

Final `cmake --preset tests` configuration and `CCACHE_DISABLE=1 cmake --build --preset tests -j 6`: PASS. `ctest --preset tests --output-on-failure`: **112/112 PASS**, 73.76 seconds. ApplicationController **127/0/0**; DesktopShell **74/0/0**; Desktop IPC **38/0/0**.

Rust fmt: PASS; clippy with all targets/features and `-D warnings`: PASS; workspace tests: **17 PASS** (8 CLI + 9 IPC). Socket-based tests require the authorized unsandboxed local test environment; the sandbox-only attempt failed because local socket creation was denied, then passed in that environment.

QML lint: exit zero, with existing/context-property and Main.winId warnings; not warning-free. Scoped C++ formatting and both generated-contract checks: PASS. `git diff --check`: PASS. Configured clang-tidy: **BLOCKED by Apple PCH format incompatibility**, with style diagnostics in the partial output; not PASS.

## 17. Security

Quick Panel input remains ordinary user input. It forwards typed IPC operations and cannot convert natural language into a shell command. PermissionService, ToolExecutionGateway, AgentLoop terminal acceptance, ModelBinding freeze, workspace/network policy, secrets and sandbox checks remain in daemon/core. Denied/stale approval requests cannot create client grants. No local runtime fallback or model/provider implementation is added. URL parsing never executes a requested command. Active Agent planner output is suppressed in the compact preview.

## 18. Hygiene

One production NativeCompanionAdapter owns the tray; remote DesktopShell notification wiring avoids a second icon. Shortcut unregister and icon teardown are explicit. Validation supervisors own only their disposable desktop/daemon/model processes and temporary profile/socket. Final cleanup verified that all owned desktop/daemon/model/supervisor PIDs exited and that the temporary profile/socket were removed. The pre-existing user daemon is outside that ownership and is preserved. Screenshots/logs/build products remain ignored. No commit or push was performed; pre-existing Phase 3 and branding changes remain in the working tree.

## 19. Remaining Work

Certify macOS menu-bar light/dark appearance, direct icon toggle/anchor, global shortcut, native notification delivery/click, physical multi-monitor behavior and login startup. Complete any remaining native E2E gates listed above, particularly Escape/outside-click closure and direct menu-bar activation. Available UI capture continued returning the panel after Escape, so no native close PASS is asserted. Measure open latency and address slow initial projection/main-window startup: the panel has no separate runtime or enumeration, but it still shares eager full-desktop initialization and the existing periodic projection refresh. Native Windows/Linux checks and Linux global shortcut integration are deferred. Responsive geometry is statically clamped; real small-screen/theme/contrast and full accessibility certification require later verification. No final visual redesign was performed.

## 20. Commit Readiness

The implementation builds and the regression baseline is clean. It is reviewable, but **not ready to claim Phase 4 PASS** until the unverified current-host native gates pass. The working tree also contains the user's earlier Phase 3/branding work, so any eventual commit must be scoped deliberately. No automatic commit was made.

**FINAL VERDICT: PARTIAL**


---

# Native Acceptance Final Closure — 2026-10-06

This addendum preserves the preceding PARTIAL report as historical evidence. Native observations below distinguish human observation, real application interaction, automated tests, and unverified OS delivery. No redesign or platform scope expansion was performed. Personal-Brain was unavailable (`brain` not installed); the explicitly supplied repository and its local instructions were used.

## 1. Menu Bar

Visible: human-confirmed. Light: human-confirmed acceptable. Dark: human-confirmed acceptable, with a contrast request subsequently addressed for the Quick Panel header. Duplicate icon: human-confirmed exactly one. Production tray resource remains the FA-3 mask/template, controlled by AppKit. The panel logo and wordmark now use the theme text color; actual dark rendering was captured and the user accepted the white appearance. This change does not replace the native menu-bar template with a white raster.

## 2. Direct Activation

Open/toggle: human-confirmed actual icon clicks open and close. Focus: actual native app observations showed Quick prompt focused on five openings. No second panel or daemon/model process was created. App-scoped Ctrl+Shift+C was used for automated cycles; it is not global shortcut evidence.

## 3. Anchor

The user broadly confirmed the post-fix native dismissal/anchor request ('o dediğinde doğru gibi geldi'). This is supportive observation, not a separately measured menu-item-to-panel geometry assertion. One physical screen was detected: 1470 × 956 points, usable 1470 × 923. Geometry uses the tray anchor and usable-screen clamp. Physical multi-monitor: NOT VALIDATED / deferred.

## 4. Dismissal

Escape: FIXED + PASS. The user first reproduced the failure, then confirmed the fix. Five consecutive actual native opening/focused-prompt/Escape cycles closed the panel; desktop and owned daemon remained alive. Regression coverage exercises three ShortcutOverride/KeyPress cycles. macOS application activation is now explicit before focusing the panel; the competing QML Escape shortcut was removed. Qt Cocoa window activation alone only makes the window key/first responder, which explains the inactive-application keyboard failure. [Qt Cocoa source](https://github.com/qt/qtbase/blob/v6.11.2/src/plugins/platforms/cocoa/qcocoawindow.mm#L1605).

Outside click: deterministic loss-of-activation dismissal remains implemented. The user broadly accepted the combined native check; an explicit separately recorded outside-click result is still weaker than the Escape evidence.

## 5. Global Shortcut

Real OS invocation and toggle: human-confirmed from the requested other-foreground-application test. Native event-post permission was unavailable to the automation helper, so application-scoped injection was never counted as global proof. Carbon registration now requests exclusive ownership, making conflicts observable. A second exclusive native registration returned -9878 while Sentinel owned Ctrl+Alt+Space. After normal Quit the probe registered and unregistered successfully (0/0). Settings replacement to Ctrl+Alt+S then freed Space (0/0) and retained S (-9878); no persistent user profile was changed.

## 6. Notifications

Safe real grounded read-file Agent completion and real loopback web-fetch approval events were exercised. Application surfaces and permission handling worked. The user initially said they did not directly see a notification, then clarified that approval was performed inside the application. This cannot establish native macOS notification delivery or notification-click routing. No notification permission prompt was observed; OS notification authorization is NOT VALIDATED.

Approval notification emission dedupe: PASS in Desktop IPC tests. The last notified approval ID is preserved across disconnect/resolution, avoiding repeat emission when the same pending approval reappears after reconnect. A new approval or terminal transition still emits its appropriate signal. This validates emission, not OS delivery.

## 7. Notification Routing

Session/run/approval native click route: NOT VALIDATED. Actual in-app approval navigation is confirmed, but it does not substitute for clicking an OS notification. Source wiring and headless routing tests do not qualify this gate as PASS.

## 8. Performance

Five warm native-toggle-entry to first QQuickWindow frameSwapped measurements: 8.76083, 7.51337, 8.56817, 7.11096, 10.7229 ms. Median: **8.56817 ms**. Worst: **10.7229 ms**. First observed opening: 29.6142 ms. These exclude external OS click dispatch and are not end-to-end mouse-to-pixels measurements. No invented pass threshold was used.

Daemon/model PIDs remained unchanged across the five openings. Opening forwards visibility/focus state and does not initiate model enumeration or daemon startup. Existing periodic projection refresh is independent. Full desktop initialization still occurs on cold startup; it is already complete for warm openings and no warm critical-path slowdown justified a broader architecture change. Evidence: ignored closure-cycles.json and closure-latency.json under build/phase4-native.

## 9. Startup Projection

Connected denotes the protocol handshake, while actionable readiness additionally requires all 28 projection pages and authoritative session attachment. The observed 1379.81 ms handshake-to-authoritative-state interval is truthful; actions remain disabled meanwhile. Refresh is sent immediately on Connected, without waiting for the two-second periodic timer. A separate warm diagnostic connection measured 27.107 ms summed sequential page RTT, worst page 9.807 ms. These measurements do not isolate every cold-start scheduling cost and cannot prove a precise rendering-delay cause. No readiness race or unnecessary initial polling wait was established, so readiness semantics were preserved. Verbose timing was added to make future investigation reproducible.

## 10. Deferred

Login startup: NOT VALIDATED, deferred to release/platform certification without persistent user-environment changes. Multi-monitor: NOT VALIDATED / deferred. Windows: STATIC IMPLEMENTATION COMPLETE, NATIVE VALIDATION DEFERRED. Linux: STATIC IMPLEMENTATION COMPLETE, NATIVE VALIDATION DEFERRED; global shortcut NOT IMPLEMENTED / DEFERRED. Voice/PTT: unavailable through current daemon contract, NOT VALIDATED / deferred.

## 11. Regression

Final configure/build: PASS. Final C++ suite after Escape, exclusive shortcut, approval dedupe and contrast fixes: **112/112 PASS**, 77.76 seconds. Controller: **127 PASS**. Shell: **74 PASS**. Desktop IPC: **38 PASS**. Native integration: **3 PASS** (Qt init/body/cleanup). Rust fmt/clippy: PASS; workspace tests **17 PASS**. QML lint: exit zero with existing context-property warnings, not warning-free. Generated contracts and diff-check: PASS. Configured clang-tidy: BLOCKED by Apple PCH/toolchain incompatibility, not PASS.

## 12. Security

Typed daemon operations, AgentRuntime/AgentLoop final acceptance, ToolExecutionGateway permissions, frozen ModelBinding, sandbox/workspace/network boundaries and no-local-fallback behavior remain unchanged. Test execution used only disposable local files and loopback services. Active internal planner output remains suppressed in the compact preview. No OS Accessibility, Screen Capture or event-post permission was silently granted. Approval actions remained explicit.

## 13. Hygiene

On continuation on 2026-10-07, the owned desktop (1553), supervisor (89459), model (89462), and daemon (89494) PIDs were absent; /tmp/s4-5fwmftkd and its socket were absent. The subsequent open application was a normal user profile and was preserved. Earlier normal-Quit Carbon probing confirmed registration release; post-continuation exclusive probes for both Ctrl+Alt+Space and Ctrl+Alt+S also returned registration/unregister 0/0. No owned supervisor/model/isolated daemon remained. User-profile desktop and daemon processes were preserved. The second completion test had no subsequent human notification observation, so it adds no notification PASS evidence. No logs/screenshots/build output was staged. Existing user-owned Phase 3 and branding work remains intact; no commit or push was performed.

## 14. Commit Readiness

The implementation and closure fixes are reviewable with passing regression. Native notification delivery and exact notification-click routing still require positive current-host evidence. Existing unrelated working-tree changes require deliberate commit scoping. Do not claim Phase 4 PASS solely from source inspection, in-app approval navigation, or tests.

**FINAL VERDICT: PARTIAL — native notification delivery/click gates remain unverified.**


---

# Notification Final Closure — 2026-10-07

Scope: macOS notification authorization, delivery and click routing only. Previous PARTIAL history remains intact. No Quick Panel redesign, Windows/Linux changes, voice expansion, daemon contract changes or unrelated runtime refactor.

## Authorization and Permission Prompt

Actual System Settings observation initially showed Sentinel Desktop notifications on, Desktop/Notification Center on, Temporary style selected. No OS settings were changed. The existing Qt Cocoa path used legacy NSUserNotificationCenter, provided no normal Sentinel authorization flow, and removed delivered messages after 8000ms. The user explicitly reported seeing no notification in the first real completion test.

The scoped replacement uses UNUserNotificationCenter. The initial linker-only test binary reported Not requested. A real authorization request returned didGrant=0 / hasError=1; no normal permission prompt or user denial was observed. The bundle signature identified the executable as sentinel-desktop, without Info.plist binding or sealed bundle resources. Standard local ad-hoc bundle signing with dev.sentinel.Sentinel bound Info.plist and resources; the real API immediately reported **Authorized**. Thus no new user permission decision was needed. macOS permission databases were not edited/reset and OS controls were not bypassed.

Non-Xcode macOS builds now perform this standard local bundle signing automatically after linking. Xcode signing and release Developer ID packaging remain separate. The final automatic-sign build was verified: Identifier=dev.sentinel.Sentinel, Info.plist entries=27, sealed resources present. This is a local development signature, not Developer ID/notarization certification.

Sources corroborating API behavior, not visual acceptance: [Apple authorization status](https://developer.apple.com/documentation/usernotifications/unnotificationsettings/authorizationstatus), [Apple authorization request](https://developer.apple.com/documentation/usernotifications/unusernotificationcenter/requestauthorization(options:completionhandler:)), [Qt 6.11.2 Cocoa tray implementation](https://github.com/qt/qtbase/blob/v6.11.2/src/plugins/platforms/cocoa/qcocoasystemtrayicon.mm).

## Agent Completion Delivery

A real UI-submitted read-file Agent run completed while the main window was hidden, Quick Panel closed, and client alive. Session: b75e3d44-8ce8-4316-8992-2027e063d12e. Run: 794fa784-6517-4b8a-a3f6-b1fbf9cde43f. The observed project was SentinelPhase4Fixture in the disposable CMakeLists.txt. At 19:08:52 Europe/Istanbul, macOS accepted the native notification request with hasError=0. Native title/body are controlled text: Sentinel Agent: completed / Open Sentinel to review this run or approval. Tool output, paths, secrets and internal planner content are not included.

**Native visible delivery: NOT VALIDATED.** The earlier legacy test had explicit negative visual feedback; the final signed implementation has OS acceptance evidence but no positive human visual observation. Automated access to Notification Center/Control Center timed out and was not counted as PASS.

## Approval Delivery and Dedupe

A second real UI-submitted task requested web-fetch against the disposable loopback model health endpoint. No approval was granted through another path. Session: 9ced0df3-04b5-4d58-850b-b7b45bd7c93f. Run: 71d56577-a631-47c7-ab4d-472f79ddfcc1. Approval: e6d5fdb9-b04c-4b0c-9976-49038479eeb6. At 19:13:50 Europe/Istanbul, macOS accepted one native approval notification request with hasError=0.

**Native visible approval delivery/count: NOT VALIDATED.** Signal-level approval dedupe and reconnect coverage pass in Desktop IPC tests. The generic Shell Agent response path is suppressed on macOS because QuickPanelController owns Agent terminal notifications. OS traces showed exactly two accepted requests in the final signed test client: one completion and one approval. This supports emission/request dedupe, not visual acceptance.

## Notification Click Routing

The old mutable latest-notification session could overwrite the target of an older notification. Each new macOS notification now carries its own immutable session/page metadata; its default-click delegate routes that exact session through the existing typed QuickPanelController link path and explicitly activates the app. Completion and approval tests use different sessions to distinguish their identities.

**Completion click: NOT VALIDATED. Approval click: NOT VALIDATED.** No positive actual notification-click observation was received. The earlier user clarification explicitly said their approval action occurred inside the application, which does not prove an OS click route. Source wiring and generic activation were not substituted for acceptance.

## Background and Denied Behavior

The real terminal/approval events occurred with main window hidden, Quick Panel closed and signed client alive. OS accepted both requests. **Background visible delivery remains NOT VALIDATED.**

Final authorization was Authorized, so native user-denial behavior was not exercised. No OS permission switch was changed just to fabricate denial. The scoped backend drops delivery when Denied, reports Denied in macOS Settings, refreshes authorization on Settings opening, and requests permission at most once per client when Not requested. Authorization API failures have a truthful status distinct from user denial. Static behavior is not a denied-state native PASS.

## Fixes and Regression

The new macOS boundary initially exposed a C++ reference-lifetime error in an asynchronous Objective-C block and crashed the test client. Native crash diagnostics identified the dangling QString reference. Blocks now own value copies; subsequent real completion and approval events kept the client alive and normal Quit succeeded. This initial failure is retained rather than hidden.

Final configure/build and automatic bundle signing: PASS. Final affected tests: **4/4 PASS**, 11.51 seconds; Controller **127**, Shell **74**, Desktop IPC **38**, Native integration **3** (Qt init/body/cleanup). Previous full C++ **112/112** and Rust **17**, fmt/clippy baselines remain preserved; unrelated suites were not rerun. Scoped QML lint exit zero with context-property warnings; scoped formatting and git diff --check PASS. Previously documented clang-tidy Apple PCH/toolchain blockage remains unchanged.

## Hygiene

Normal Quit submitted removal for the final client's own two pending/delivered notification identifiers; OS traces record Removing 2 for each category. It does not remove unrelated application/user notifications. Visual Notification Center residue was not independently checked. Owned supervisor, desktop, daemon and model PIDs were absent after cleanup; /tmp/s4-vbqazsq4 profile and socket were removed. The pre-existing user daemon PID23477 was preserved. No approval was granted and no external network fetch executed. Ignored evidence remains under build/notification-native, with nothing staged and no commit/push. User preferences and prior Phase 3/branding work were preserved.

**FINAL VERDICT: PARTIAL — authorization, local bundle identity and notification routing implementation are fixed; actual visible completion/approval delivery and notification-click identity still lack positive current-host observation.**

## Current-host follow-up — 2026-10-08

Historical PARTIAL conclusions above remain valid for their original run. [Deferred technical cleanup](DEFERRED_TECHNICAL_CLEANUP_REPORT.md) adds typed GUI capture/daemon STT dictation and real level projection, Ready distinct from Connected, onboarding focus containment, safe QML fixes, and an executable PCH-free tidy path. Actual login enable/registration/disable and fresh Finder icon acceptance passed. Microphone probe permission was denied and production STT is unconfigured. The focused background approval/completion attempt did not establish visible notification/click acceptance; Agent trials failed final grounding and are not completion successes. Multi-monitor and remaining native icon surfaces remain deferred. No notification artwork/code was guessed into a false PASS.
