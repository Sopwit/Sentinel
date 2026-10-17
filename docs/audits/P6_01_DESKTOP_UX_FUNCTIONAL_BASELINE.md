# P6-01 — Desktop UI/UX Architecture & Functional Baseline

Audit date: 2026-10-17
Status: **Evidence baseline; no production UI code changed by this task.**
Branch: `audit/p6-01-desktop-ux-baseline`
Base: `origin/main` = `6035bf74cbb3f2556e29fe1bf31163a56eb5b591` (`feat(resources): add native resource telemetry monitor adapters and sampling tests`)
Worktree: an independent worktree of this repository on the branch above (clean; independent of INT-05 / P2 / P3 / P4 / P5 worktrees)

This document is an independent audit. It does not assume existing desktop features are absent
or require rewrite. No commit, push, merge, reset, stash, PR or hosted workflow dispatch was performed.

Related roadmap acceptance criteria: **AC-P6-06** (Desktop/Quick Panel/tray flow), **AC-P6-07**
(KDE activation), **AC-P6-08** (clipboard/application launch) in
[`ACCEPTANCE_MATRIX.md`](../roadmaps/ACCEPTANCE_MATRIX.md); epic **EP-P6** in
[`GITHUB_ISSUE_BACKLOG.md`](../roadmaps/GITHUB_ISSUE_BACKLOG.md). Existing UI findings
[`UI-001..UI-015`](../development/UI_UX_VISUAL_AUDIT.md) and
[`UI_UX_REMEDIATION_PHASE1_REPORT.md`](../development/UI_UX_REMEDIATION_PHASE1_REPORT.md) are
referenced rather than duplicated.

---

## 0. Method, isolation and evidence limits

### 0.1 Isolation verified
| Check | Result |
| --- | --- |
| Branch | `audit/p6-01-desktop-ux-baseline` (existing branch re-used; already checked out at this worktree) |
| HEAD | `6035bf74` (= current `origin/main` after `git fetch`) |
| Worktree status | clean (`git status --porcelain` empty at start and after audit; only this new doc is added) |
| No interference | INT-05, P2, P3, P4, P5 worktrees untouched; no commits/pushes performed |
| AGENTS.md | present at worktree root; read before architecture-sensitive work |

### 0.2 What was executed (real evidence)
| Activity | Command | Result |
| --- | --- | --- |
| Test build | `cmake --build build/tests --target <desktop/daemon/QML tests + app>` (Ninja, Qt 6.12, macOS) | success |
| Desktop/QML test suite | `QT_QPA_PLATFORM=offscreen ctest --test-dir build/tests -R '<desktop+QML+contract set>'` | **23/23 passed, 0 failed** (58 s) |
| Focused C++ slots | `<binary> -functions` on 10 desktop binaries | 73+45+4+1+8+5+2+2+40+3 = **183 slots** available; all executed green in CTest |
| QML lint | `qmllint -I ui/qml -I build/tests/apps/sentinel-desktop` over 72 QML files | **0 errors**, 229 warnings (see §E) |
| App build | `cmake --build build/tests --target sentinel-desktop sentinel-daemon` | both linked |
| Headless startup smoke | `QT_QPA_PLATFORM=offscreen sentinel-desktop --safe-mode --no-daemon-start --preferences-directory <tmp>` | stayed alive; rendered; logged a **112 ms** authoritative-projection/session handshake against the daemon on `~/.sentinel/run/daemon.sock` (read-only attach/queries) |
| Headless no-daemon smoke | same with `--daemon-socket <tmp>/nope.sock` | stayed alive, no crash, no error output |
| Translation coverage | `grep` over `translations/*.ts` | 50–61 % translated (see §C.9) |

### 0.3 Evidence limits (explicitly source-only or unverified)
- **No interactive/graphical acceptance** was possible on this host. Native tray, global shortcut,
  window activation, OS notifications, microphone/STT, and multi-monitor placement are **source-reviewed only**.
- The app smoke run connected to a **pre-existing local daemon** from another worktree; only read-only
  commands (`hello`, `daemon.status`, `session.attach`, `session.messages`, `desktop.settings_service`,
  `voice.state`) were observed. No chat/agent run, approval or mutation was submitted.
- The GUI smoke harness (`settings_ui_certification_probe`) mounts **production components in an
  offscreen engine**, not the complete production root. Its pass does not certify native visuals (§E).
- Fedora KDE/Wayland, Windows and macOS-ARM **native** checks are **not** claimed (AC-P6-06/07/08 remain Blocked).

---

## 1. Actual desktop architecture

### 1.1 Composition and ownership
Sentinel Desktop is a **pure daemon-backed IPC client**. Production owns **no `ApplicationController`**.

```
main.cpp (QApplication)
 └─ ApplicationBootstrapper
     ├─ SingleInstanceGuard            QLockFile + QLocalServer  (sentinel:// deep links)
     ├─ GraphicsInitializer            QQuickWindow::setGraphicsApi(per-platform)
     ├─ LoggingInitializer / PlatformInitializer   file logger, crash handler, UI font, translations
     ├─ DaemonClient                   QLocalSocket transport (UDS / Win named pipe)   ← sentinel_desktop_ipc
     ├─ DesktopRuntimeClient           presentation cache over DaemonClient            ← sentinel_desktop_viewmodels
     ├─ DesktopSettingsStore           local UI prefs + daemon-projected runtime config
     ├─ RemoteAgentInspectorService → AgentInspectorViewModel
     ├─ ModeManager / WinTaskbarIntegration
     ├─ DesktopShellViewModel          QML-safe facade over DesktopControllerBridge
     ├─ QuickPanelController           projection + actions for the compact window
     ├─ NativeCompanionAdapter         tray, global shortcut, OS notifications, tray lifecycle
     └─ QQmlApplicationEngine          loads module "Sentinel.Desktop"/"Main"
```

**State-flow rule (verified):** all authoritative state and accepted transitions originate in the daemon.
`DesktopRuntimeClient` is documented and coded as a *presentation cache only*
(`DesktopRuntimeClient.h:13`). `DesktopControllerBridge` states: *"Production supplies a
DesktopRuntimeClient and owns no ApplicationController."* A local `ApplicationController*` path survives
**only as a legacy test seam** (`DesktopControllerBridge.h:9-10`); the bootstrapper never supplies one.

### 1.2 Startup sequence (verified from source)
1. `QApplication` constructed (`main.cpp:10`).
2. `ApplicationBootstrapper` ctor: app metadata + icon; `StorageMigration::migrateLegacyApplicationStorage`; parse flags; optional `--preferences-directory` profile override (`ApplicationBootstrapper.cpp:105-159`).
3. `ensureSingleInstance()` → `QLockFile` in temp; if held, forwards a `sentinel://` URL and exits `1` (`main.cpp:14-16`).
4. `initializeLogging()` → file logger + custom Qt message handler (aborts on fatal).
5. `initializeGraphics()` → `QQuickWindow::setGraphicsApi(defaultGraphicsApiForPlatform())`.
6. `initializePlatformIntegrations()` → crash handler; on Windows protocol/AUMID/autostart; default UI font.
7. `setupQmlEngine()` → builds the object graph, registers `Sentinel.Warden`, **11 `setContextProperty` bindings**, hooks `objectCreationFailed → exit(-1)`, then `loadFromModule("Sentinel.Desktop","Main")`.
8. `QApplication::exec()`; the main window is shown by QML (`Main.qml:19 visible: true`).
9. **Daemon is ensured lazily**: `DaemonClient` schedules a first connect via `QTimer::singleShot(0)`; when the state becomes `Unavailable` the bootstrapper calls `ensureBackgroundDaemon()` unless `--no-daemon-start`/`--safe-mode` (`ApplicationBootstrapper.cpp:258-263`, `ensureBackgroundDaemon` `81-103`). The daemon is `startDetached`ed so it outlives the UI.

### 1.3 Shutdown sequence (verified gap)
- There is **no `aboutToQuit` handler** anywhere in `apps core ui` (grep = 0).
- Exit is `QApplication::quit()` from the tray menu (`NativeCompanionAdapter.cpp:91`); `~ApplicationBootstrapper` is `= default`; the only explicit teardown is `DaemonClient::~DaemonClient()` aborting the socket.
- The desktop **never** calls `daemon.shutdown`; the detached daemon is expected to self-manage via its socket lock.
- `qApp->setQuitOnLastWindowClosed(false)` is set at boot and recomputed as `!background` where `background = companionEnabled && trayAvailable` (`NativeCompanionAdapter.cpp:186-195`). Close-to-tray is implemented in `eventFilter` (`:303-308`).

### 1.4 IPC transport and reconnect
- `DaemonClient` uses `QLocalSocket` (UDS on Unix, named pipe on Windows). Default endpoint `~/.sentinel/run/daemon.sock` (override `--daemon-socket`).
- Handshake `hello` validates major/minor and yields a `server_generation`; `DesktopRuntimeClient` resets its cursor when the generation changes.
- Reconnect is a **fixed 1000 ms** single-shot timer with **no exponential backoff**; request timeout 10 s; **no mutation replay** after timeout/reconnect (`DaemonClient.h:78-79`).
- Errors map to a typed enum (`daemon-unavailable`, `request-timeout`, `incompatible-major`, `unknown-session`, `invalid-approval`, `permission-denied`, `provider-unavailable`, `model-unavailable`, `runtime-busy`, …).
- Forward-compat: unknown projection fields are rejected, but tolerated unknown **properties** are filtered (`validProperties`) — a deliberate anti-drift guard.

### 1.5 C++ → QML boundary
- **Not** `qmlRegisterSingletonType`. View models are injected as **context properties** (`ApplicationBootstrapper.cpp:271-286`):
  `nativeDesktop`, `quickPanelController`, `shellViewModel`, `agentInspectorViewModel`, `ollamaPuller`, `ollamaLibraryFetcher`, `ollamaModelDetailFetcher`, `ggufLibraryFetcher`, `lmStudioLibraryFetcher`, `daemonClient`, `coreWardenPreferences`.
- Only QML-module C++ type registered: `CoreWardenPlayer` (`Sentinel.Warden`, `QML_NAMED_ELEMENT`).
- `DesktopShellViewModel` is a very wide facade: **873 `Q_PROPERTY`**, **160 `Q_INVOKABLE`** across a 2958-line header and 7237-line implementation, forwarding to `DesktopControllerBridge`. This is presentation-only but is a maintainability hotspot.
- `Main.qml` composes a `NavigationRail` + `pageStack` Item with five sibling pages (Dashboard / Models / Settings / ExtensionsHub / AgentInspector), plus overlay children (Splash, CommandPalette, NotificationCenter, TrayCompanionWindow, Onboarding, UpdateProgressModal, recovery modal, **daemon approval modal**).
- **Dead chrome (registered but never instantiated):** `HeaderBar.qml`, `StatusBar.qml`, `BottomDock.qml`, `TitleBar.qml`, `ActiveAgentsPanel.qml`, `CognitionStreamPanel.qml`. `TitleBar` would provide window controls but is unused (`nativeTitlebar: true` is declared with no `TitleBar` instance).

### 1.6 Persistence split (verified)
- **Local UI preferences** → `DesktopSettingsStore` → `JsonSettingsStore` at `settingsFilePath()+".desktop"`; a 28-key allow-list (theme, language, companion/quick-panel, accessibility flags, notification policy JSON, sound, update policy, onboarding, active conversation id, recovery draft). All other reads/writes are routed to the daemon.
- **Runtime config, sessions, runs, approvals, memory, conversations** → daemon (Qt SQL, separate stores).

---

## 2. Feature inventory

Legend: **V** = implemented and verified by automated test or executed smoke in this audit; **S** = implemented, source-reviewed only (native/manual untested); **D** = dead/unwired code; **M** = missing.

| # | Feature | Where | Status |
| --- | --- | --- | --- |
| F1 | Single-instance guard + `sentinel://` deep link forwarding | `SingleInstanceGuard.cpp` | V (contract tests) |
| F2 | Daemon auto-launch (detached) + `--no-daemon-start`/`--safe-mode` | `ApplicationBootstrapper.cpp` | V (startup smoke) |
| F3 | Graphics backend selection (Metal/D3D11/OpenGL + env override) | `GraphicsBackend.cpp` | V (`test_graphics_backend`) |
| F4 | Daemon IPC transport, handshake, generation fencing | `DaemonClient.cpp` | V (`test_desktop_ipc`) |
| F5 | Presentation projection / reconnect invalidation | `DesktopRuntimeClient.cpp` | V (`cor17..cor20`, IPC tests) |
| F6 | Chat send / cancel / regenerate / retry / edit-resend | VM + IPC | V (IPC tests) |
| F7 | Agent run lifecycle + tool/approval projection | VM + IPC | V (approval matrix tests) |
| F8 | Agent Inspector (runs/timeline/evidence/claims/approvals/children) | `AgentInspectorViewModel` | V (IPC history/detail tests) |
| F9 | Conversation store create/switch/rename/pin/archive/delete | VM + IPC | V (IPC conversation tests) |
| F10 | Model library (local LLM/Ollama/LM Studio/HF), pagination | `DesktopModelHelper`, `ModelsPage` | V (helper chunking tests) |
| F11 | Provider selection + readiness projection | VM (`activeRuntimeReadinessState`, …) | V (VM tests) |
| F12 | Settings (8 categories, local + daemon), theme, density | `SettingsPage` + tabs | V (settings smoke) |
| F13 | Onboarding 7-step wizard with focus containment | `onboarding/*` | V (`test_onboarding_focus`) |
| F14 | Notification center + toasts + policy/DND/snooze | `NotificationCenterPanel`, VM | V (phase1 smoke) |
| F15 | Approval dialog (tool/resource/risk, Deny default focus, Allow once) | `Main.qml:444-494` | V (backend+VM); **QML click path S only** |
| F16 | Controlled-task plan/approve/execute/skip/retry UI | `SecuritySettingsTab.qml` | V (VM/IPC) |
| F17 | Persistent permission grants revoke/clear | `SecuritySettingsTab.qml` | V (VM) |
| F18 | System tray + menu + close-to-tray | `NativeCompanionAdapter.cpp` | S (offscreen smoke only) |
| F19 | Quick Panel window (show/hide/anchor/focus) | `TrayCompanionWindow.qml` + adapter | V (offscreen tray smoke) + S native |
| F20 | macOS native notifications (`UNUserNotificationCenter`) | `MacNativeNotifications.mm` | S− (macOS not native-tested here) |
| F21 | Linux/Windows notifications via Qt tray balloon | `NativeCompanionAdapter.cpp:167-182` | S |
| F22 | macOS dictation (`SFSpeechRecognizer`) | `MacSystemDictationService.mm` | S |
| F23 | Local Whisper STT via daemon | VM + `voice.state` | V (VM metadata) |
| F24 | Update check/download modal | `UpdateProgressModal.qml` + VM | V (VM tests) |
| F25 | Backup export/import (daemon-chunked, digest-verified) | `DesktopBackupHelper` | V (`test_desktop_ipc`) |
| F26 | Core Warden mascot (asset-validated animation) | `CoreWardenItem.cpp` | V (`test_core_warden`) |
| F27 | Command palette (13 actions; 2 disabled honestly) | `CommandPalette.qml` | V (phase1 smoke) |
| F28 | Global Quick Panel shortcut | `NativeCompanionAdapter.cpp:397-455` | macOS/Windows S; **Linux M** |
| F29 | Window geometry/state restoration | — | **M** |
| F30 | Light/dark **system-follow** + OS reduced-motion/contrast | — | **M** |
| F31 | High-DPI awareness | — | **M** (no `devicePixelRatio`/`HighDpi` anywhere) |
| F32 | Windows native toast / taskbar progress/JumpList wiring | `WinTaskbarIntegration.cpp` | **D** (only `setWindowHandle` wired) |
| F33 | `ChatViewModel` (standalone) | `viewmodels/ChatViewModel.*` | **D** (compiled, never instantiated) |
| F34 | Global error banner population | VM `setGlobalError` | **D** (never called; not invokable) |
| F35 | Linux app launch / clipboard brokerage | — | **M** (deliberately unavailable) |

---

## 3. Implemented vs missing functionality

### 3.1 Implemented and verified (this host, automated)
IPC transport/handshake/reconnect; chat send-complete-cancel; agent run + approval allow/deny/cancel;
conversation lifecycle + persistence; model discovery helpers; Agent Inspector history/detail;
settings surfaces; onboarding + focus containment; notifications policy; tray/window smoke;
graphics selection; backup transfer; Core Warden; command palette. See §E for the contract.

### 3.2 Implemented but unverified (source-only on this host)
Native tray icon/menu presentation; Quick Panel anchoring under a real compositor; macOS notification
delivery/authorization; macOS activation quirks; macOS dictation; global shortcut registration;
multi-monitor placement; high-contrast/reduced-motion OS detection; live provider readiness and
real-model completion.

### 3.3 Missing / dead
- **Window geometry + state restoration**: no `saveGeometry`/`restoreGeometry`/`windowState` anywhere; `Main.qml` hard-codes 1320×860, min 780×640 (§C.8, F29).
- **Light/dark system-follow** and OS reduced-motion/high-contrast auto-detection: manual presets only; `Qt.ColorScheme`/`QStyleHints` unused (F30).
- **High-DPI handling**: none (F31).
- **Linux/KDE global shortcut**: reports *"Global shortcut unavailable on this platform"* (`NativeCompanionAdapter.cpp:450`); no portal/DBus/KGlobalAccel code (F28).
- **Linux/Windows native notifications**: only `QSystemTrayIcon::showMessage` (F21); no `org.freedesktop.Notifications`, libnotify or GNotification.
- **Windows taskbar progress/JumpList**: implemented in core, unwired from the app (F32).
- **Dead QML chrome**: `HeaderBar`, `StatusBar`, `BottomDock`, `TitleBar`, `ActiveAgentsPanel`, `CognitionStreamPanel` (F/§1.5).
- **Dead code**: unused `ChatViewModel`; unpopulated global-error banner (F33/F34).
- **Cloud API key / proxy password getters** return empty by design (secret non-exposure) — **not** a defect.

---

## 4. End-to-end verification matrix (Part B)

| # | Behavior | Verified end-to-end? | Evidence / notes |
| --- | --- | --- | --- |
| 1 | **Startup / shutdown** | **Partial** | Startup smoke: app launched, rendered, 112 ms daemon handshake. Shutdown: no `aboutToQuit`, no daemon handshake teardown; daemon intentionally detached → **orderly-shutdown semantics unverified** |
| 2 | **First-run / no-model** | **Partial** | `OnboardingScreen` + `FinishStep` separate *saved* vs *runnable* vs *ready* (`FinishStep.qml:15-16`); smoke `test_onboarding_focus`. Live "no model" acceptance not run |
| 3 | **Provider selection/config** | **Partial** | Readiness states projected (`activeRuntimeReadinessState/Summary`); credential writes only. Live provider round-trip not run |
| 4 | **Chat streaming / cancellation** | **Partial** | Delta accumulation + monotonic event fencing verified; cancellation tests green. **QML sees whole-message replacement (`partial=true`), not token deltas** — no per-token signal |
| 5 | **Agent tool execution status** | **Partial** | Tool/plan/sandbox/execution "latest" strings + counts verified at VM/IPC; live visual status not captured |
| 6 | **Approval / permission dialogs** | **Partial** | Backend+VM allow/deny/cancel green; QML modal click path source-only (matches open **UI-010**) |
| 7 | **Daemon disconnect / reconnect** | **Verified** | `reconnectInvalidatesPendingIds`, `cor17..cor20`, `presentationAdapterChatSessionsCancelRecover`; no mutation replay; generation fencing |
| 8 | **Conversation persistence/restore** | **Verified** | IPC conversation + `historyLoaded()` barrier tests; active id persisted locally |
| 9 | **Error reporting** | **Partial** | Per-channel errors exist (chat lifecycle, notifications on `operationFailed`, attachment, inspector, onboarding). **Global error banner is dead**; no unified error surface |
| 10 | **Model loading / readiness indicators** | **Partial** | Connected ≠ ready is enforced (`NavigationRail.qml:111`, `FinishStep`); live loading/ready not exercised |
| 11 | **Long-running tasks** | **Partial** | Active-run observability + cancel verified; Agent Inspector poll 2.5 s, 500-run cap; no long-duration soak |
| 12 | **Tray / background** | **Partial** | `--tray-smoke` offscreen green; native tray/background untested |
| 13 | **Window restoration** | **Missing** | No geometry/state persistence (F29) |
| 14 | **Loading/empty/failure states** | **Partial** | `EmptyState`/`ErrorBanner`/phase objects present and smoke-covered; failure visuals on the full production root unverified |

---

## 5. UX and accessibility gaps (Part C)

### 5.1 Strong points (do not rewrite)
- Design tokens centralized: `SentinelTheme` (colors/spacing/radii/typography/layout), `MotionTokens` (durations/easings + `reducedMotion`), `InteractionTokens` (hover/press/focus states). 18 named themes with a derived `lightTheme`.
- Reusable base controls with focus policy + focus rings (`SentinelButton`, `SentinelComboBox`, `SentinelSwitch`, `SentinelSlider`, `SentinelSpinBox`).
- Explicit phase/state modeling (`chatSendLifecycleState`, `activeRuntimeReadinessState`, extensions `phase`) instead of boolean guessing.
- Thorough accessibility metadata on notifications (3/3) and `TrayCompanionWindow`.

### 5.2 Visual consistency & tokens
- **Two diverging motion token sets**: `SentinelTheme.durationFast/Normal/Slow` = 90/140/220 ms vs `MotionTokens.fast/normal/slow` = 90/150/230 ms. Components almost always use `MotionTokens`; `SentinelTheme` motion is effectively dead.
- **`SentinelTheme.reducedMotion` is assigned but never read** (only `MotionTokens.reducedMotion` is consulted).
- **Hardcoded hex bypasses tokens** in `ErrorBanner.qml:29-32`, `RuntimeDetailPopup.qml` (`#3b82f6` ×7), `NotificationItemDelegate.qml:63-65`, `NotificationToast.qml:34`, `SentinelSwitch.qml:45`, `SentinelSpinBox.qml:28`, `RuntimeBadge.qml:31`. 37 hex literals in `components/`.
- **`RuntimeDetailPopup.qml` is a token outlier**: defines its own `sfProFont`, fixed 2-column grid and fixed label widths — will not reflow narrow.
- `SentinelTheme.fontFamily` is used in **one** QML place; most text inherits the platform default.

### 5.3 Navigation clarity
- Left icon rail with 5 primary entries + a daemon-status button; Settings opens full-page with 8 categories; command palette (Ctrl/Cmd+K) with 13 actions.
- Command palette honestly disables "Search Chats" and "Universal Search" (`CommandPalette.qml:25,35`) — good, but exposes missing features.
- **Missing shell chrome**: no visible title bar/status bar/bottom dock; `Ctrl+1/2/4` skip a `Ctrl+3` binding; the `TitleBar` traffic-light intent is unimplemented.

### 5.4 Responsive layout
- Breakpoints exist (`Main.qml compact <1080`, `wideLayout >= 1120`; per-component `compact` thresholds 520–900).
- Window minimum 780×640; settings sidebar collapses to a combo below 820.
- **Inconsistent thresholds** across components (`SettingsPage` 820 vs `HomeChatSurface` 760/900 vs `AgentInspectorPage` 900) risk mismatched states at intermediate widths.
- Fixed geometry in `RuntimeDetailPopup` and `RuntimeBadge` won't reflow.

### 5.5 Animation/transitions
- Motion tokens used consistently for page crossfade, popups, buttons; reduced-motion honored in decorative loops (Atmosphere, Shimmer, Splash, ModelsPage, onboarding).
- **`SentinelOrb.qml` and `GlowSurface.qml` expose a local `reducedMotion` but never read the singleton** → reduced-motion leak.
- `SplashScreen.qml:20` uses a literal `200` ms duration.

### 5.6 Typography & spacing
- Density-scaled spacing (`spaceXs…space5Xl`) and font tokens exist and are broadly used.
- Weights are applied ad hoc (`font.bold`/`font.weight`) with no tokens.

### 5.7 Keyboard shortcuts & focus
- Shortcuts: `Ctrl/Cmd+K`, `Ctrl+1/2/4`, `Ctrl+L`, `Ctrl+,`, `Ctrl+Shift+C`, `Esc` cascade. Approval modal focuses **Deny** first (`Main.qml:449`). Onboarding focus containment smoke-tested.
- `activeFocusOnTab` used in only **6** places; tab-order correctness at the root is smoke-only.
- Dead shortcuts risk: `HeaderBar`/`StatusBar` (which might carry additional bindings) are unused.

### 5.8 Accessibility
- `Accessible.*` appears in **20 of 72** QML files, 166 usages total (`Accessible.name` ×101). **None of the six core base controls** set `Accessible.name/role`.
- `SentinelOverlayModal` (and thus recovery + **daemon approval modal**) sets **no accessible role/name**.
- Only **two** native accessible-name assertions exist in tests (daemon status button, DND switch).
- No screen-reader (VoiceOver/Orca/NVDA), role/state/action, or full focus-order certification (matches **UI-014**).

### 5.9 Light/dark, high-DPI, small window
- Themes: 18 presets, manual selection; **no** "Auto/System" entry and no OS color-scheme listener.
- High-DPI: **none**; relies on Qt defaults and vector/SVG icons. `SentinelTextField` uses `FontMetrics` to avoid clipping — the only DPI-adjacent care.
- Small window: min 780×640; modals clamp to parent; extensive `WordWrap`/`Elide`; a few fixed-geometry outliers (§5.2/§5.4).

### 5.10 Long-conversation performance
- `ChatMessageListModel` uses a real diff: full reset on first set; **`beginInsertRows` fast path** for prefix-matching appends; per-row `dataChanged` on equal-size updates. Roles include `partial`, `stateNotice`, `contentParts`.
- Caveats: the model re-scans the **whole** `chatHistory()` on every `chatMessagesChanged`; `dataChanged` is emitted **without a role vector** (invalidates every role); no pagination/`fetchMore`/virtualization. `AgentInspectorViewModel` rows are whole `QVariantMap`s and its list model **resets wholesale** on each poll.

### 5.11 Platform desktop conventions
- macOS: template tray icon, explicit `NSApp` activation for the panel, `UNUserNotificationCenter`. Windows: `RegisterHotKey`, AUMID, protocol handler, autostart. Linux: `.desktop`/metainfo present.
- Deviations: no native macOS window menu/standard title bar (frameless shell), no Windows toast, no Linux portal.

---

## 6. Platform-specific risks (Part D)

| Area | Fedora KDE / Wayland | macOS | Windows |
| --- | --- | --- | --- |
| Tray | `QSystemTrayIcon` needs a StatusNotifier host; no KDE/AppIndicator-specific code (Flatpak only whitelists `org.kde.StatusNotifierWatcher`) | template icon, no attached context menu (manual popup) | generic Qt tray |
| Window mgmt | **High risk**: Quick Panel uses `QWindow::setPosition` (`NativeCompanionAdapter.cpp:246-247`) which Wayland compositors generally ignore → anchoring may not work | `activateMacQuickPanelApplication()` handles NSApp activation | standard |
| Notifications | **Qt tray balloon only**; no `org.freedesktop.Notifications` | real native backend (unsigned/ad-hoc bundles may suppress identity) | **no native toast** |
| Global shortcut | **Missing** — reports unavailable (AC-P6-07) | Carbon `RegisterEventHotKey` (2 chords) | `RegisterHotKey` (2 chords) |
| Filesystem access | isolated behind `IFileSystemService`; FileDialog via QML | sandbox/codesign matters for notifications/dictation | standard |
| Rendering | OpenGL requested; `QSG_RHI_BACKEND`/`QT_QUICK_BACKEND` honored; **no automatic software fallback** | Metal | Direct3D11 |
| Native dictation | unavailable (stub) | on-device `SFSpeechRecognizer` | unavailable (stub; suggests Win+H) |
| Clipboard / app launch | headless clipboard unavailable; Linux app launch unsupported (AC-P6-08) | — | — |

**No native platform compatibility is claimed for any of the above without tests.** Linux is the
*primary target* yet has the **least** native shortcut/notification coverage — the central P6 risk.

---

## 7. Priority-ranked defects

Severity scale: **P0** correctness/truthfulness; **P1** high user-visible or acceptance-blocking; **P2** polish.

| ID | Sev | Defect | Evidence | Repo ref |
| --- | --- | --- | --- | --- |
| D-P6-01 | P1 | **Linux/KDE Wayland global shortcut missing** — primary target cannot open the Quick Panel from the OS | `NativeCompanionAdapter.cpp:450` | AC-P6-07, UI/A11 |
| D-P6-02 | P1 | **No window geometry/state restoration** | grep `restoreGeometry|windowState` = 0; `Main.qml:13-19` | AC-P6-06 |
| D-P6-03 | P1 | **Global error channel is dead** — `setGlobalError` never called and not invokable; failures surface only as scattered per-channel strings | `DesktopShellViewModel.cpp:6563`; grep callers = 0 | §4.9 |
| D-P6-04 | P1 | **Accessibility metadata absent on the six core base controls and the overlay modal** (approval/recovery have no accessible role/name) | §5.8; `SentinelOverlayModal.qml` | AC-P6-06, UI-014 |
| D-P6-05 | P1 | **Linux/Windows notifications are only Qt tray balloons** | `NativeCompanionAdapter.cpp:167-182` | AC-P6-06 |
| D-P6-06 | P2 | **No light/dark system-follow or OS reduced-motion/contrast detection** | grep `ColorScheme|QStyleHints` = 0 | §5.9 |
| D-P6-07 | P2 | **No high-DPI handling** | grep `devicePixelRatio|HighDpi` = 0 | §5.9 |
| D-P6-08 | P2 | **Two diverging motion token sets; `SentinelTheme.reducedMotion` dead; `SentinelOrb`/`GlowSurface` ignore reduced motion** | §5.2/§5.5 | — |
| D-P6-09 | P2 | **Dead shell chrome** (`HeaderBar`/`StatusBar`/`BottomDock`/`TitleBar`/`ActiveAgentsPanel`/`CognitionStreamPanel`) and dead `ChatViewModel` | §1.5, F33 | — |
| D-P6-10 | P2 | **Token bypass / hardcoded hex & fixed geometry** in `RuntimeDetailPopup`, `ErrorBanner`, notification colors | §5.2 | — |
| D-P6-11 | P3 | QML lint noise: 95 `UnqualifiedAccess` warnings; 119 structural `import` warnings | §E | — |
| D-P6-12 | P3 | `test_gui_components` QML tests are placeholders (`verify(true)`) | `tests/gui/components/tst_*.qml` | — |
| D-P6-13 | P3 | Translation coverage ~50–61 % (UI-009) | §E | UI-009 |
| D-P6-14 | P3 | No automated window-restoration / approval-QML-click / screen-reader coverage | §E coverage gaps | UI-010/011/014 |

---

## 8. Recommended design-system improvements

1. **Single motion source of truth.** Delete `SentinelTheme.duration*`/`easingStandard` (or make them
   aliases of `MotionTokens`); remove `SentinelTheme.reducedMotion`; have `SentinelOrb`/`GlowSurface`
   read `MotionTokens.reducedMotion`. *Reason: prevents the two systems from drifting and closes a reduced-motion leak.*
2. **Token-completeness pass.** Add `scrim`, `onAccent`, `info`/priority colors and a shadow color token;
   replace the 37 component hex literals with tokens. *Reason: theme/high-contrast correctness.*
3. **Accessibility contract on base controls.** Require `Accessible.role` + `Accessible.name` on
   `SentinelButton`, `SentinelTextField`, `SentinelComboBox`, `SentinelSwitch`, `SentinelSlider`,
   `SentinelSpinBox`; give `SentinelOverlayModal` an `Accessible.Dialog` role and a title-derived name.
   *Reason: approval/recovery safety surfaces must be announced.*
4. **System-appearance adapter (presentation-only).** Add an "Auto (System)" theme entry and an
   OS color-scheme/reduced-motion/contrast probe (via `QStyleHints`) exposed as a *view-model* signal;
   keep the decision in C++, not QML. *Reason: platform conventions without moving business logic into QML.*
5. **Unify responsive breakpoints** into `SentinelTheme` tokens and consume them everywhere
   (currently 760/820/900/1080/1120 coexist). *Reason: consistent layout state transitions.*
6. **Remove dead chrome** or wire it. Decide explicitly whether HeaderBar/StatusBar/TitleBar/BottomDock
   are the intended future shell or should be deleted; the `nativeTitlebar` intent is currently unmet.
7. **Path-resolution unification.** `ui/qml/qmldir` duplicates the CMake-generated module; `ModelCatalog.js`
   is excluded from qmldir. Prefer the generated module as the single truth to reduce lint/import noise.

---

## 9. Reuse vs refactor recommendations

**Reuse as-is (do not rewrite):**
- The daemon-backed IPC architecture, `DaemonClient`/`DesktopRuntimeClient` split, generation fencing and no-replay semantics.
- `DesktopShellViewModel` + `DesktopControllerBridge` presentation-only facade and `ChatMessageListModel` diffing (streaming-append fast path).
- `SentinelTheme`/`MotionTokens`/`InteractionTokens` and all 25 base components.
- `AgentInspectorViewModel`/`RemoteAgentInspectorService`, `DesktopBackupHelper`, `CoreWardenItem`.
- The onscreen smoke harness (`settings_ui_certification_probe`) and its offscreen CTest wiring.

**Extend (bounded changes):**
- `NativeCompanionAdapter` — add an **XDG portal / KGlobalAccel global-shortcut backend** behind the
  existing platform seam (keeps macOS/Windows intact). *AC-P6-07.*
- `ApplicationBootstrapper` — add a **window-state persistence** hook (QSettings local prefs) and an
  explicit **orderly shutdown** path; keep daemon detachment. *D-P6-02.*
- `DesktopShellViewModel` — wire one **unified error projection** to the existing banner. *D-P6-03.*

**Refactor only if adopted by a work package:**
- Split the 7237-line `DesktopShellViewModel.cpp` along its existing concerns (it is already a forwarding
  facade); do not change the C++/QML contract during P6.
- Collapse the two motion token sets (low risk, high clarity).

**Do not do:** second runtime, in-process fallback, QML business logic, heuristic shell execution,
clipboard polling, or a broad visual redesign (roadmap P6 "out-of-scope").

---

## 10. Concrete P6-02 implementation scope (proposed)

Recommended **first** implementation task (highest value × lowest architectural risk):

> **P6-02 — Keyboard-first desktop activation and truthful shell state**
> 1. **Window state persistence** (`saveGeometry`/`restoreState` for the main window; last page and
>    optional panel size) persisted through `DesktopSettingsStore` local keys. *Closes D-P6-02, part of AC-P6-06.*
> 2. **Unified error surfacing**: route `operationFailed`/lifecycle failures into the existing global
>    banner via a new invokable/projected error, with severity + dismiss. *Closes D-P6-03.*
> 3. **XDG-portal global-shortcut backend on Linux** behind `NativeCompanionAdapter`, reporting
>    registered / denied / unsupported truthfully; explicit "tray absent" handling. *Closes D-P6-01, AC-P6-07.*
> 4. **Accessibility contract** on the six base controls + `SentinelOverlayModal` (role/name), with a
>    native accessible-role assertion added to the existing probe. *Closes D-P6-04, part of UI-014.*
>
> **Acceptance evidence:** offscreen `ctest` for window-restore round-trip and error projection;
> a Linux shortcut registration/rejection unit test; an accessible-name/role test for the approval modal;
> the existing smoke harness re-run at 780×640 and 1320×860.

Follow-on P6-03 candidates (in priority order): Linux/Wayland Quick Panel **anchoring** decision
(portal/`layer-shell` vs documented limitation) → native notification portal backend →
system-appearance + high-DPI handling → dead-chrome decision → long-conversation model pagination.

---

## E. Verification appendix

### E.1 Test build and run
```
cmake --build build/tests --target \
  test_desktop_shell_view_model test_desktop_ipc test_agent_reliability_ipc \
  test_native_integration test_core_warden test_extensions_hub test_graphics_backend \
  test_chat_view_model test_chat_message_recovery test_daemon_ipc test_mcp_server \
  settings_ui_certification_probe test_gui_components          # → built / linked

QT_QPA_PLATFORM=offscreen ctest --test-dir build/tests -R '<desktop+QML+contract set>'
# → 100% tests passed, 23/23, Total Test time (real) = 58.09 sec
```
Tests included: `test_desktop_shell_view_model` (73 slots), `test_desktop_ipc` (45 slots, 72 runtime
cases), `test_agent_reliability_ipc` (4), `test_native_integration` (1), `test_core_warden` (8),
`test_extensions_hub` (5), `test_chat_view_model` (2), `test_chat_message_recovery` (2),
`test_daemon_ipc` (40), `test_graphics_backend` (3), plus GUI smoke tests
(`test_ui_phase1`, `test_onboarding_focus`, `test_navigation_settings`, `test_chat_surface`,
`test_models_surface`, `test_settings_surface`, `test_tray_surface`, `test_extensions_surface`,
`test_ui_phase1_failure_exit`, `test_gui_components`) and contract tests
(`test_branding_sources`, `test_desktop_generated_contract`, `test_ipc_generated_contract`).

> Note: an initial `test_desktop_ipc` failure (`extensionsDesktopMcpRealProcessDisconnectAndRecovery`)
> was **not** a product defect — the `test_mcp_server` helper target had not been built, so the test's
> real MCP process could not start. After building `test_mcp_server`, the test passed (3/3 slots).

### E.2 QML lint (`qmllint 6.12.0`)
```
qmllint -I ui/qml -I build/tests/apps/sentinel-desktop  (72 files)
→ exit 0; 0 errors; 229 warnings
  UnqualifiedAccess 95 · import 119 (structural) · index 3
```
The `import` warnings are a source-vs-generated qmldir artifact; CI runs the same command set.

### E.3 Startup / disconnected smoke
```
QT_QPA_PLATFORM=offscreen sentinel-desktop --safe-mode --no-daemon-start \
   --preferences-directory <tmp>            → stays alive; renders;
     log: "Desktop handshake to authoritative projection/session (ms): 112.446"
QT_QPA_PLATFORM=offscreen sentinel-desktop --safe-mode --no-daemon-start \
   --daemon-socket <tmp>/nope.sock --preferences-directory <tmp>  → stays alive, no crash
```

### E.4 Coverage gaps (no automated coverage)
- Window restoration/geometry (none anywhere).
- Approval **QML** click/deny/allow-once rendering (backend/VM covered only).
- Screen-reader roles/states/actions; full focus-order; VAST majority of `Accessible.*` unchecked.
- `ChatViewModel` class itself; `Main.qml` root, `SplashScreen`, `UpdateProgressModal`, `TitleBar`.
- OS tray icon/menu, native notifications, global shortcuts, microphone/STT hardware.

### E.5 Translation coverage
| Catalog | Messages | Untranslated | Coverage |
| --- | --- | --- | --- |
| en | 1493 | 615 | 58 % |
| tr | 1493 | 569 | 61 % |
| ar / de / es / fr / ja / zh | 1487 | 735 | 50 % |

---

## F. Final summary

- **Branch:** `audit/p6-01-desktop-ux-baseline` at **`6035bf74`** (= `origin/main`), worktree clean.
- **Implemented:** a genuine, daemon-backed Qt 6/QML desktop with real IPC, chat, agent runs + approvals,
  conversation persistence, model discovery, an Agent Inspector, onboarding, notifications, tray/Quick
  Panel, backup, and a token-based design system — all **source-verified** and, for the headless-testable
  subset, **test-verified (23/23)**.
- **Major gaps:** Linux/KDE Wayland global shortcut; no window-state restoration; dead global-error
  channel; sparse accessibility on core controls/approval modal; Linux/Windows notifications limited to
  tray balloons; no system-follow/high-DPI handling; dead shell chrome and `ChatViewModel`; ~50–61 %
  translations; placeholder QML component tests.
- **Test evidence:** 23/23 desktop/QML/contract CTest tests pass offscreen; app + daemon build; headless
  startup and disconnected smokes pass; `qmllint` 0 errors.
- **Recommended first P6 task:** **P6-02 — keyboard-first desktop activation and truthful shell state**
  (window-state persistence; unified error surfacing; Linux portal global shortcut; base-control +
  approval-modal accessibility), with the offscreen harness as acceptance evidence.

No commit, push, merge, reset, stash, PR or hosted workflow dispatch was performed. No production UI
code was modified; this report is the only file added.
