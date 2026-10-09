# Sentinel UI/UX screen inventory

Audit date: 2026-10-09. Baseline: `102e602` plus the existing local working tree. This is an evidence inventory, not an implementation or release certification.

## Scope and counting

**50 logical UI surfaces / experiences identified; 18 captured in 22 new JPEGs; 32 NOT CAPTURED.** A surface is an independently useful page, panel, menu, modal, wizard step, or terminal/platform experience. Related native file pickers count as one experience; variants and loading states do not count again. Embedded controlled-task, permission and restore-review flows count separately because they have their own user actions. This is not a QML-file count. Packaging entries are implementation recipes, not proof that an installer was built or published.

Capture means actual production QML rendered by the repository's existing certification application, **not the complete production shell or a live daemon acceptance test**. No production-window screenshot was collected. See the [visual audit](UI_UX_VISUAL_AUDIT.md) for evidence limits and the [capture manifest](../reviews/ui-ux-audit-2026-10-09/README.md) for individual images. A category capture proves its visible viewport, not every subsection below the fold.

Before this audit, two files were already modified: `ui/qml/components/navigation/NavigationRail.qml` (rail width 76 → 56 and spacing) and `tests/helpers/settings_ui_certification.qml` (matching width assertion). Both were left unchanged. No production code, dependencies, user settings, permissions or credentials were changed by the audit. Only the disposable certification profile received onboarding/theme choices. No commit, push or merge was performed. Personal-Brain's `brain` executable was unavailable; the supplied workspace was used.

## Source and authority map

Paths below are repository-relative. QML root: `ui/qml/`; desktop source root: `apps/sentinel-desktop/`.

| Layer | Sources | Responsibility / authority |
| --- | --- | --- |
| Entry and shell | `main.cpp`, `bootstrap/ApplicationBootstrapper.cpp`, `ui/qml/Main.qml` | Bootstrap, translation, standard paths, graphics, single instance; mounts Home, Models, Inspector and full-page Settings |
| Presentation | `src/DesktopShellViewModel.cpp`, `src/viewmodels/ChatViewModel.cpp`, `src/viewmodels/AgentInspectorViewModel.cpp` | QML-safe state/actions; presentation is not execution authority |
| Desktop transport | `src/DesktopRuntimeClient.cpp`, `src/DaemonClient.cpp`, `src/DesktopControllerBridge.cpp` | Connects to authoritative daemon, request lifecycle, state snapshots and events |
| Contract | `protocol/ipc-v1.json`, generated desktop bridge and CLI IPC contracts | Single source of IPC commands/types; future UI fixes must not duplicate contracts |
| Models | `src/DesktopModelHelper.cpp`, `ui/qml/pages/models/ModelCatalog.js`, daemon ModelService | Discovery, normalized families/variants, installation and operation state |
| Backup | `src/DesktopBackupHelper.cpp`, `core/include/sentinel/core/app/BackupService.h`, `RecoveryService.h` | Reviewed JSON import/export, chunked transport, recovery snapshots; not credential backup |
| Execution | `core/include/sentinel/core/agent/`, AgentRuntime/AgentLoop, ModelBinding/IModelRouter, ToolExecutionGateway | Accepted AgentLoop final answer completes a run; tool authorization remains authoritative |
| Native companion | `src/NativeCompanionAdapter.cpp`, `src/QuickPanelController.cpp`, `src/MacNativeNotifications.mm`, `src/MacSystemDictationService.mm` | Tray, activation, focus dismissal, shortcuts, notifications, native dictation |
| Persistence | AppSettings, SQLite conversation/memory/run stores, WorkspaceService | Settings, memory, conversations, runs and grants remain separate |
| Terminal | `cli/crates/sentinel-cli/src/main.rs`, `sentinel-ipc`, `sentinel-tui` | CLI/TUI use daemon authority; do not silently start provider/daemon fallbacks |
| Packaging | `cmake/SentinelCPack.cmake`, `packaging/`, `docs/release/PACKAGING.md` | Platform-specific distribution and integration recipes |

Architecture references: [Architecture](ARCHITECTURE.md), [Agent Runtime](../concepts/AGENT_RUNTIME.md), [Security Model](../security/SECURITY_MODEL.md), [Building](BUILDING.md), [Testing](TESTING.md).

## Reachable implementation inventory

`H` = rendered in disposable harness. `N` = NOT CAPTURED. A source implementation alone never implies native/platform PASS. Source paths in this table are under `ui/qml/` unless otherwise stated.

| ID | Surface / entry | Actual source | Backend / connection | Evidence or exact capture limitation |
| --- | --- | --- | --- | --- |
| S01 | Production window, fixed icon rail and page host | `Main.qml`; `components/navigation/NavigationRail.qml`, `ShellPanel.qml` | DesktopShellViewModel / DesktopRuntimeClient | N: only rail component appears in harness; complete production root not isolated/launched |
| S02 | Home / empty conversation | `pages/DashboardPage.qml`; `components/chat/HomeChatSurface.qml` | ChatViewModel, conversation runtime | H: 16, 18, 21; empty only |
| S03 | Conversation history sidebar | `components/chat/HomeChatSurface.qml` | Conversation store through viewmodel/daemon | H: 17; disposable history, no real-user transcript |
| S04 | Composer / Chat-Agent mode / voice error | `components/chat/HomeChatSurface.qml` | Provider/model binding, attachments, voice runtime | H: 16, 18, 21, 22; no successful inference |
| S05 | Conversation overflow menu | `components/chat/HomeChatSurface.qml` (`convItemMenu`) | Rename/archive/delete actions | N: no populated conversation row exercised |
| S06 | Message overflow menu | `components/chat/HomeChatSurface.qml` (`messageMenu`) | Message actions, exports, conversation lifecycle | N: no user/assistant message pair generated |
| S07 | Attachment action menu | `components/chat/HomeChatSurface.qml` (`attachMenu`) | Image/file attachment validation | N: menu not opened |
| S08 | Chat image/document file picker | `components/chat/HomeChatSurface.qml` | Platform FileDialog; extracted text/vision attachment pipeline | N: user files deliberately not opened |
| S09 | Conversation deletion confirmation | `components/chat/HomeChatSurface.qml` (`deleteConfirmDialog`) | Pending deletion, active-run guard | N: no deletion performed; cannot certify prior deletion bug fixed |
| S10 | Embedded Agent progress / execution status | `components/chat/HomeChatSurface.qml`, `RuntimeStateStrip.qml` | AgentRuntime events, tool/runtime state | N: no safe live Agent/model configured |
| S11 | Agent Inspector list and selected run | `pages/AgentInspectorPage.qml` | AgentInspectorViewModel / RemoteAgentInspectorService | N: harness does not mount Inspector; no empty/populated/live capture |
| S12 | Models library, filters and pagination | `pages/models/ModelsPage.qml`, `ModelCatalog.js` | DesktopModelHelper, provider/HF discovery | N: interactive harness has no route to Models; no current live timeout reproduction |
| S13 | Model family/variant detail popup | `components/dialogs/ModelDetailPopup.qml` | Artifact metadata, README, install/activation actions | N: S12 unavailable in interactive harness |
| S14 | GGUF import picker | `pages/models/ModelsPage.qml` (`ggufImportDialog`) | Model import/registration | N: import would modify model state |
| S15 | Runtime setup modal | `pages/models/ModelsPage.qml` (`runtimeSetup`) | Explicit llama.cpp setup; provider download links | N: no installer/build operation started |
| S16 | Settings: General | `pages/settings/SettingsPage.qml`, `AppearanceSettingsTab.qml`, `SystemSettingsTab.qml` | Local presentation + native companion | H: 08 |
| S17 | Settings: Appearance | `pages/settings/AppearanceSettingsTab.qml` | Theme/accessibility preferences, MotionTokens | H: 09 light, 20 dark/compact |
| S18 | Settings: Models & Providers | `pages/settings/ModelSettingsTab.qml` | Provider readiness, capabilities, routing/settings | H: 10; disabled/unconfigured state |
| S19 | Settings: Voice & Audio | `pages/settings/VoiceSettingsTab.qml` | Voice runtime, devices, dictation service | H: 11; microphone acceptance not attempted |
| S20 | Settings: Workspace & Memory | `pages/settings/WorkspaceSettingsTab.qml` | WorkspaceService, memory/RAG, response profile | H: 12; upper viewport only |
| S21 | Settings: Privacy & Permissions | `pages/settings/SecuritySettingsTab.qml` | Policy/grants/controlled-task gateway | H: 13; no grants or policy changed |
| S22 | Settings: Notifications | `pages/settings/SystemSettingsTab.qml` (`notificationsOnly`) | Notification policy/history viewmodel | H: 14; Open history produced no visible panel |
| S23 | Settings: System | `pages/settings/SystemSettingsTab.qml` | Backup helper, recovery, updater, diagnostics | H: 15, 19; disabled export / healthy empty recovery |
| S24 | Voice runtime/model file picker | `pages/settings/VoiceSettingsTab.qml` (`voicePathDialog`) | Platform FileDialog, validated runtime paths | N: no paths selected |
| S25 | Workspace folder picker | `pages/settings/WorkspaceSettingsTab.qml` | Platform FolderDialog / WorkspaceService | N: no user workspace selected |
| S26 | Knowledge document picker | `pages/settings/WorkspaceSettingsTab.qml` | FileDialog / local ingestion & RAG | N: no document ingested |
| S27 | Controlled task describe/review/approve/run flow | `pages/settings/SecuritySettingsTab.qml` | ControlledTaskService / AgentRuntime | N: below captured viewport; no task execution |
| S28 | Persistent permission review/revocation | `pages/settings/SecuritySettingsTab.qml` | Persistent permission grants | N: below captured viewport; no grants touched |
| S29 | Backup manifest/domain selection and restore review | `pages/settings/SystemSettingsTab.qml` | DesktopBackupHelper / BackupService | N: no backup inspected/imported; entry controls only visible in S23 |
| S30 | Backup save/open picker experience | `pages/settings/SystemSettingsTab.qml` | FileDialog / bounded local JSON | N: no file transfer |
| S31 | Saved draft recovery overlay | `Main.qml` (`recoveryModal`) | Local saved draft, no automatic execution | N: production-root-only overlay; no recovery draft seeded |
| S32 | Daemon tool approval overlay | `Main.qml` (`daemonApprovalModal`) | Pending approval ID, allow once/deny/cancel | N: no pending real tool approval; no permission created |
| S33 | Onboarding: Welcome | `components/onboarding/WelcomeStep.qml` | OnboardingScreen / viewmodel | H: 01 |
| S34 | Onboarding: Processing preference | `components/onboarding/ProcessingModeStep.qml` | Local/cloud preference | H: 02; disposable defaults |
| S35 | Onboarding: Provider setup | `components/onboarding/ProviderSetupStep.qml` | Provider endpoint/readiness/setup | H: 03; fixture status is not a daemon probe |
| S36 | Onboarding: Model setup | `components/onboarding/ModelSetupStep.qml` | Model selection/inference temperature | H: 04; no model selected |
| S37 | Onboarding: Optional voice | `components/onboarding/VoiceSetupStep.qml` | STT/TTS readiness, device list | H: 05; no microphone recording |
| S38 | Onboarding: Privacy summary | `components/onboarding/PrivacyConsentStep.qml` | Informational boundaries | H: 06 |
| S39 | Onboarding: Finish | `components/onboarding/FinishStep.qml` | Saved setup summary | H: 07; provider/model set up later |
| S40 | Quick Panel window | `components/dialogs/TrayCompanionWindow.qml` | QuickPanelController, same desktop runtime client/history | N: fixture window not activated; live dismissal/history continuity unverified |
| S41 | Native tray/context menu | `apps/sentinel-desktop/src/NativeCompanionAdapter.cpp` | Platform tray actions/activation | N: no OS menu screenshot; not inferred from QML |
| S42 | Command palette | `components/navigation/CommandPalette.qml` | Main navigation/actions | N: production-root component absent from harness |
| S43 | Update progress modal | `components/dialogs/UpdateProgressModal.qml` | Update check/download/relaunch signals | N: no network update/download/relaunch triggered |
| S44 | Splash screen | `components/dialogs/SplashScreen.qml` | Bootstrap shell-ready lifecycle | N: transient production-root startup not captured |
| S45 | CLI text / JSON / streaming output | `cli/crates/sentinel-cli/src/main.rs` | sentinel-ipc / daemon | N: no new terminal execution evidence |
| S46 | Ratatui TUI chat/pickers/session attach | `cli/crates/sentinel-tui/src/lib.rs`, `editor.rs`, `picker.rs` | sentinel-ipc / daemon | N: no interactive terminal capture |
| S47 | Native desktop integration experience | `apps/sentinel-desktop/src/NativeCompanionAdapter.cpp`, `MacNativeActivation.mm`, `MacNativeNotifications.mm` | Dock/taskbar, startup, OS notifications, shortcuts | N: isolated harness cannot certify installed app identities/permissions |
| S48 | Linux installation/integration | `packaging/linux/`, `cmake/SentinelCPack.cmake` | RPM/DEB, Fedora, Flatpak/Snap, desktop/AppStream/systemd | N: macOS audit host; no Fedora/KDE install session |
| S49 | Windows installation/integration | `packaging/windows/`, `cmake/SentinelCPack.cmake` | CPack installer, manifest, signing/silent lifecycle | N: no Windows host or installer run |
| S50 | macOS installation/integration | `packaging/macos/`, `cmake/SentinelCPack.cmake` | Bundle/plist/privacy/notarization/Homebrew/update recipes | N: development build only; no signed release install/uninstall |

## Settings subsection inventory

All eight categories share `SettingsPage.qml`, a searchable category list and compact category selector. Search filters category keywords, not individual setting values. Category screenshots do not certify controls outside their viewport.

| Category | Existing subsections and controls |
| --- | --- |
| General | Language; Desktop: sounds, companion service, start at login, global Quick Panel shortcut, system notification authorization status |
| Appearance | Active Theme and 14 visual presets; Accessibility: reduced motion, high contrast, reduced transparency, UI density |
| Models & Providers | Active runtime/provider light and model; advanced provider catalog; resolved/override model capabilities; endpoints & routing; cloud provider and credentials; advanced inference presets, temperature, top-p, max tokens, request timeout |
| Voice & Audio | Microphone test; detect files; audio input/output and VAD; auto detection; TTS engine, Kokoro model/voice, Piper executable/model/file output; STT source (local Whisper/system), Whisper executable/model/transcription |
| Workspace & Memory | Visible response instructions/templates/save; Brain & Memory/continuity; workspace selection/name/root; local knowledge ingestion and semantic provider/embedding model; export format/attachment behavior; timestamps/citations/anonymization/model metadata |
| Privacy & Permissions | Retention for chat, runs, diagnostics, model cache and operation history; execution safety/default policy/autonomy; persistent permissions; controlled task describe/plan/approve/run; proxy protocol/host/port/auth; web search provider/key/results |
| Notifications | Do Not Disturb; open history/mark read/clear archived; Tasks, Security, Workspace, Brain channels; notification policy; model download/removal, agent response and system update event switches |
| System | Backup selected domains and restore review/merge/replace; recovery status and interrupted downloads; updates/release state; developer diagnostics mode/details; update policy/version/platform information |

There is no independent About page or in-app license browser mounted in `Main.qml`; version/platform information lives in System. Repository and installer license files are not an About screen.

## Source present but not mounted as current production screens

These are **excluded from the 50 reachable/packaged experiences**. Registration in `qmldir` is not instantiation.

| Source-only component/domain | Finding |
| --- | --- |
| `components/notifications/NotificationCenterPanel.qml`, `NotificationItemDelegate.qml`, `NotificationToast.qml` | Panel/toast definitions and notification viewmodel exist, but no production QML instantiation was found. The Settings history action changes a property with no visible consumer. UI-001. |
| `pages/ChatPanel.qml` | Separate chat component definition; Dashboard currently mounts HomeChatSurface instead. Do not audit it as a second visible chat page. |
| `components/chat/ActiveAgentsPanel.qml`, `CognitionStreamPanel.qml` | Defined/registered, no production instantiation found. Do not claim active-agent board or cognition screen. |
| `components/dialogs/RuntimeDetailPopup.qml` | Definition exists, no mounted caller found. Runtime setup modal S15 is a different component. |
| `components/navigation/BottomDock.qml`, `HeaderBar.qml`, `StatusBar.qml` | Legacy/source components, not the current Main shell layout. |
| Tools/MCP/plugins/skills | Core services exist (`core/include/sentinel/core/{mcp,plugin,skill,extension}/`); no dedicated current QML management screen found. Diagnostic text/tool approval is not an extension manager. |
| Tasks/scheduler | Controlled task workflow exists in Privacy & Permissions; core task services do not prove a standalone task dashboard/scheduler UI. |
| Projects | Workspace selection/root/name and scoped knowledge exist; no independent project management page found. |
| Memory browser | Workspace & Memory settings/status exist; no separate record browser/editor page found. |
| Core Warden | Preserve as the separate Agent/Voice mascot concept. No separately mounted Core Warden screen was found; SentinelOrb is not evidence of a shipped mascot integration. |

## Model state and pagination interpretation

Current `ModelsPage.qml` groups variants into model-family cards and pages the loaded filtered set in 40-card slices. Hugging Face discovery also has explicit previous/next catalog batches. These are two different operations: a card page changes the visible slice; a catalog batch requests another bounded discovery result. Sorting is scoped to the loaded catalog. This avoids treating 40 as a global count, but the two navigation levels need live usability evidence. `BUILDING.md` still describes automatic Hugging Face continuation and is stale relative to this baseline (UI-008).

Downloaded does not mean runnable: GGUF may be registered for llama.cpp; safetensors/ONNX/PyTorch files need compatible complete runtimes/repositories. Metadata absence must be represented honestly, not invented. Neither download success nor metadata completeness was certified in this audit.

## Capture and acceptance gaps

Current captures cover light, dark, empty, configured-control/populated-choice and voice-unavailable error presentations; 780×640 and 1100×780 logical window sizes; partial keyboard flow. They do **not** cover real loading, successful populated chat, live model timeout/retry, active Agent, pending approval, populated Inspector, backup restore success/conflicts, OS notification delivery, tray dismissal, or installed platform workflows. Those gaps remain explicit in the roadmap; none is marked PASS.
