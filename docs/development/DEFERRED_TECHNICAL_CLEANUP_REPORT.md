# SENTINEL — DEFERRED TECHNICAL CLEANUP REPORT

Validation: 2026-10-08, macOS 27.0.1 arm64 / Apple M2, Qt 6.11.2, Debug/tests build. User-provided workspace was used; `brain` was unavailable on PATH. Architecture, Agent Runtime, security, building and testing instructions were read. No packaging, redesign, model download, cross-platform certification, commit or push was performed.

## 1. Voice / PTT

architecture: GUI-only microphone permission/capture uses the existing `AudioDeviceService`. Qt permissions on macOS require a GUI application; the headless daemon cannot own that request. The daemon's existing `VoiceSessionService` owns STT. Additive typed `voice.state`, `voice.action` and `voice.audio` commands share the canonical IPC source and generated C++/Qt/Rust contracts. This is dictation, with explicit transcript-to-composer review; it never starts Chat/Agent automatically. No second speech engine exists.

Quick Panel: start, stop and cancellation; idle, requesting permission, preparing capture, listening, processing, unavailable and failure projections. Selected input identity and VAD come from daemon settings. Missing devices fail rather than switching silently. Actual PCM is normalized mono 16 kHz signed little-endian 16-bit audio, retained only in bounded memory. Completed VAD segments and the final segment are combined. The GUI stops at 59 seconds; daemon total bound is 1,920,000 bytes. 49,152-byte chunks become at most 65,536 base64 characters and are sent sequentially after acknowledgements. Owner-only leases, malformed-input rejection, disconnect cleanup, generation invalidation and STT configuration-revision checks prevent stale work. Transcripts are returned only to the completing owner with the matching revision, never to a new owner during capture.

microphone permission: actual bundled audio certification probe enumerated three input and two output devices, then returned Qt Denied / `MicrophonePermissionDenied` (exit 3). No TCC permissions were changed. This result belongs to `dev.sentinel.AudioCertification`; production Sentinel capture permission is not falsely inferred from a different bundle identity.

real capture: BLOCKED by probe permission denial; production STT is unconfigured and remains truthfully unavailable. No capture success or end-to-end STT success is claimed on physical hardware.

audio level: existing measured RMS `inputLevel()` only, projected at a maximum 10 Hz during capture; idle state queries run every two seconds. No fabricated waveform. Physical level acceptance remains deferred with capture.

remaining: authorized production-bundle microphone capture with an explicitly configured existing STT runtime, including real selected-device/VAD/meter and cancellation acceptance. TTS/STS are not falsely enabled by this dictation bridge.

## 2. Onboarding Focus

reproduced: actual native helper traversal could escape the wizard or skip its controls. Root cause: ordinary item focus plus implicit Qt traversal across the stack and background controls; direct synthetic QKeyEvent delivery also bypasses the Qt ShortcutOverride path and was not a valid reproduction test.

fix: active FocusScope, explicit initial/step navigation focus, visible/enabled/tab-focus control traversal with forward/reverse wrap, and Window shortcuts enabled only while focus belongs to the onboarding root. Popup focus is not stolen. Back/Next mouse behavior remains intact. Finish exits the wizard through its normal action.

keyboard validation: real CUA Tab/Shift+Tab, Next and modal Escape flows; permanent `test_onboarding_focus` uses QTest keyboard delivery, visits all seven steps and Back, performs 24 forward/reverse traversal pairs per step, checks containment and rejects invisible/disabled focus. Settings Escape restored focus to the named settings button.

## 3. QML Warnings

before: 766 warnings across full source lint. The initial anchored count of 699 missed warnings concatenated after Info text on the same line; the final count scans every `Warning:` occurrence.

fixed: 694 net warnings. Fixed lexical/delegate scope, required model roles after Bound semantics, missing imports/types/theme properties, invalid layout ownership, duplicate bindings, a ListView `populate` name collision, wrong animation parent geometry, invalid screen-coordinate/`winId` access, and the nonexistent GridView column count. Window dragging now uses the supported native system-move API. Disabled Quick Panel shortcut selection now displays and preserves Disabled correctly.

remaining: 72 `unqualified` diagnostics, lint exit zero. Exact file/line/column/symbol inventory: [DEFERRED_QML_WARNINGS.md](DEFERRED_QML_WARNINGS.md).

reason: explicit C++ bootstrap context objects cannot be resolved by standalone qmllint. These are documented intentional exceptions, not ignored owned lexical defects. No lint category was disabled. Migrating shared context injection to typed required properties is deferred to the planned QML redesign.

## 4. Clang-Tidy

root cause: Apple LLVM 21 PCH emitted by CMake is incompatible with Homebrew LLVM/clang-tidy 23.1.2. The production database contained `-Xclang -include-pch -Xclang ...cmake_pch.hxx.pch`, forced CMake PCH inclusion and `-Winvalid-pch`. Simply removing the PCH also required explicit macOS SDK/libc++ discovery for Homebrew clang.

solution: `tools/analysis/compile_database.py` produces a separate arguments-based compile database, strips only CMake PCH flags, and adds xcrun-resolved SDK/libc++ include paths on macOS. Production toolchain and PCH configuration are unchanged. Readability/numeric-conversion findings were fixed. One precise Qt QObject-parent ownership analyzer false positive in `VoiceSessionService::speak` is locally annotated; parent destruction and the finished/deleteLater path own the watcher. Existing unrelated NOLINTs are preserved. No global analyzer category was disabled.

command:

```bash
python3 tools/analysis/compile_database.py build/tests/compile_commands.json /private/tmp/sentinel-cleanup-analysis
/opt/homebrew/opt/llvm/bin/clang-tidy -p /private/tmp/sentinel-cleanup-analysis \
  core/src/voice/UnifiedAudioService.cpp \
  apps/sentinel-daemon/service/DaemonIpcServer.cpp \
  apps/sentinel-desktop/src/DesktopRuntimeClient.cpp \
  apps/sentinel-desktop/src/QuickPanelController.cpp \
  apps/sentinel-desktop/bootstrap/ApplicationBootstrapper.cpp
```

result: PASS in this explicit five-source scope; actual final non-fixing analysis completed, exit zero, no displayed warnings/errors. Header/non-user diagnostics remain filtered by the repository configuration; this is not a whole-repository tidy certification.

## 5. Startup Projection

state machine: transport Connected → required global eager projection + settings + session list → successful session attachment → Ready. Required fields come from canonical scope/eager metadata; optional and session-only fields do not block global readiness. Disconnect clears prior-generation values/settings/messages/approvals and cancels local voice. Cold startup and reconnect use this same path.

cold timing: 8,814.83 ms process launch to observed Ready (10 ms log sampling), including 2,469.36 ms handshake to authoritative projection/session. Another cold-client handshake sample was 2,962.8 ms. These Debug measurements ran during compilation/analysis/regression activity and are not benchmark-grade.

reconnect timing: 3,363.42 ms handshake to Ready after a real idle daemon shutdown/restart. CUA observed disconnected/reconnecting with Send disabled, then Ready and the original conversation restored.

changes: concise Loading state, no actions authorized from Connected alone, full required-field completeness, cleared stale projection, and no new provider enumeration/refresh loop. A process sample exposed an eager model-library getter reading an unselected Gemini Keychain credential and blocking IPC. Model-library presentation now uses the existing credential-free `ModelService::providerStatusSnapshot`; actual binding configuration retains authorized credential reads. A focused test verifies both halves. No cloud credential or keychain permission was modified.

## 6. Login Startup

enable: actual Sentinel System setting enabled startup.

registration: generated `~/Library/LaunchAgents/dev.sentinel.desktop.plist` passed plutil; Label, RunAtLoad and authoritative built executable were checked. Supported `launchctl bootstrap gui/501` succeeded and `launchctl print` showed a registered LaunchAgent, executable and one run. No user logout/reboot occurred.

disable: bootout succeeded; Sentinel setting was switched back off.

cleanup: plist absent; final `launchctl print gui/501/dev.sentinel.desktop` reports no service. Original OFF state restored; no hidden persistence added. Registration/helper semantics PASS, actual next-login execution remains untested.

## 7. Multi-Monitor

environment: system_profiler with normal host permissions reports one usable built-in Color LCD, 1470×956 logical / 2940×1912 physical at 60 Hz.

result: DEFERRED; no second usable display, no multi-monitor PASS claim.

## 8. Notifications

Agent delivery: focused background attempt using the already-installed Nemotron model failed to produce an accepted grounded Agent final. Run `be1a5c90-3e4e-4d03-b7f4-d5312d06d178` is failed, not completed. Completion delivery cannot be certified from it.

Agent click: not observed.

approval delivery: a real approved write-file request produced approval `837f553b-54d6-460f-adf7-6b4915bf41f5` on run `f19435b6-51e2-48bf-a141-fce970d7e03a`. TUI reconnect preserved the ID; Allow Once wrote only the disposable fixture. A visible macOS banner was not observed. The model then failed final grounding; it is not claimed as an Agent completion.

approval click: actual macOS click not observed; TUI acceptance is not native click acceptance.

background: main window hidden, Quick Panel closed, Desktop process alive during the attempt. Authorization Authorized; System Settings showed Sentinel allowed, temporary banners and desktop/Notification Center presentation enabled. Global presentation while mirroring/sharing was Off; a screen-sharing indicator was visible. This is a possible suppression explanation, not proof. Focus sharing/status was On; an active Focus mode was not conclusively established. NotificationCenter/control-center surfaces were unavailable to the automation. No OS notification DB or settings were modified.

result: BLOCKED/DEFERRED visible delivery and click. Existing sender/API-acceptance evidence from Phase 4 is preserved historically; no speculative notification code change was made.

## 9. Branding Native Acceptance

Finder: fresh build/tests bundle selected in Finder; canonical current icon visually observed, PASS. No stale artwork mismatch was demonstrated.

Dock: current visual acceptance unobserved.

menu bar: mask resource/identity integrity checked, full native menu-bar image unavailable to the automation; visual acceptance deferred.

notification: identity authorized; visible notification icon unobserved.

result: PARTIAL visual acceptance. Source and bundle ICNS SHA-256 are both `fc7804aa5725dca5e26d32ee381120e299e9ba3c99cfbd3a37e49b8eb4347f67`. No artwork changed.

## 10. Daemon Restart

old daemon: original owned PID 23477 was idle and stopped through typed daemon.shutdown. Intermediate task-owned daemons were replaced during validation; one Keychain-blocked idle instance was stopped by TERM after process sampling identified the wait.

new daemon: final tests-built executable uses the same user profile/socket, protocol 1.1 and rc.8. Final generation `31baec0c-cae3-4ef1-add9-534c9d248921`; final status active_runs zero. User data was preserved.

protocol: additive typed voice commands; installed Rust CLI remains compatible. Rust generator now escapes the reserved `final` field as `r#final`, preserving wire spelling.

CLI smoke: status, doctor, models, sessions all passed after restart. Final existing Ollama/qwen2.5:3b Chat returned completed `READY`, without downloading a model.

TUI smoke: provider/model, workspace/session pickers, slash palette, real cancellation, resume and reconnect passed. One real Allow Once approval/reconnect passed on the earlier current-build daemon; no repeated native notification attempts were made merely to manufacture a result.

## 11. Performance

Desktop Ready: 8,814.83 ms process-to-Ready; handshake samples 2,469.36/2,962.8 ms, reconnect 3,363.42 ms. No comparable historical whole-process sample exists; the observed total is flagged for an idle-host follow-up rather than called a regression-free benchmark. An existing runtime font-alias notice for `Inter, Sans-serif` cost 128–171 ms; it is outside the static QML warning count and remains part of that follow-up.

Quick Panel: native adapter recorded 73.237 ms open-to-first-frame. The bound automation window often returned the main window, so the timing is renderer instrumentation, not a complete visual panel certification.

CLI: 13.89, 10.13, 10.79, 12.07, 10.10 ms status; prior range 6–19 ms.

TUI: 109.43 ms first usable frame with 100 ms sampling; prior approximately 102 ms. No clear terminal regression.

Agent first event: 29.907 ms on the real Nemotron attempt; final failed after approximately 105 seconds. First-event latency is not successful completion.

## 12. Accessibility Pre-Redesign

keyboard: onboarding forward/reverse traversal, Back/Next and popup-aware containment; TUI pickers/palette/cancel/resume. Settings Escape restores the initiating button.

focus: existing focus visuals preserved; active initial/navigation focus and no invisible wizard targets are tested. No product UI redesign.

roles/names: named New Chat, Show/Hide sidebar, Open settings and Voice/PTT; real accessibility tree exposes button/combobox/text-field roles and disabled Send states. Basic existing modal/button semantics remain intact.

modals: actual settings Escape and return focus observed; onboarding containment tested across all seven steps. Full screen-reader/contrast/reduced-motion/DPI certification remains outside this phase.

## 13. Security

AgentRuntime/AgentLoop accepted-final authority, IModelRouter and immutable ModelBinding remain unchanged. ModelService still owns provider identities, configuration and runtime readiness. ToolRegistry/descriptor, ToolArgumentValidator, ToolExecutionGateway, permission/resource authorization, sandbox, ContextEngine and IFileSystemService/ProcessExecutor boundaries remain authoritative. Voice accepts only bounded normalized PCM and produces reviewed dictation; it cannot bypass tool approval or turn natural language into a shell command. Captures, old generation callbacks and transcript ownership are isolated. No fallback, Rust authority, secret logging, credential migration or raw-audio file persistence was introduced. Regression suites exercise the existing boundaries and new ownership/revision/late-cancellation cases.

## 14. Regression

Final configure/build passed. Full C++ CTest: **114/114 PASS**, 111.75 seconds. Controller **127**, Shell **74**, Desktop IPC **41**, Native **3**. C++ tests are preserved; the new onboarding executable raises the target count from 113 to 114. Desktop IPC grows from 38 to 41 QtTest cases with readiness, PCM lease/privacy/configuration and credential-free model-library projection coverage. Three focused PCM lifecycle tests were added.

Two timing assumptions exposed during full runs were repaired without deleting coverage: conversation creation now checks metadata-based recency and ties rather than assuming the same millisecond; the cancellation/provider-binding test uses an explicit bounded completion gate so the fake reply cannot finish while selection assertions are running. The gate is released on scope exit, and production cancellation/recovery assertions remain.

Rust: 36 passed (CLI 14, IPC 11, TUI 11). fmt and strict clippy with all targets/features passed. Sandboxed socket-denied runs were rerun under normal local permissions; those environment failures are not hidden. QML full lint exit zero, 72 inventoried exceptions. Generated IPC/Desktop checks, branding integrity and diff check pass. Tidy five-source final analysis PASS with the narrow documented Qt ownership exception.

## 15. Hygiene

One final daemon and one Desktop client remain. No temporary audio capture, certification helper, notification helper or task-owned llama-server remains; pre-existing Ollama was preserved. User provider/model restored to ollama/qwen2.5:3b, llama endpoint to localhost:8080, workspace to personal, active Desktop conversation to its original ID, login and shortcut to Disabled. Three test conversations were archived via the supported action; user conversations/stores were not deleted.

The owned isolated profile and temporary acceptance workspace were removed after saving fixture evidence. The workspace catalog was restored to its original empty value through typed settings, after confirming its only custom entry was this test workspace. Analysis database and logs are retained for review, without running processes or hidden persistence. Native registration is removed. Evidence is copied under ignored `build/tests/cleanup-evidence`; no unrelated sockets/processes were cleaned.

## 16. Remaining Work

Production-bundle authorized microphone/STT/meter acceptance; visible native approval/completion delivery and click routing; Dock/menu-bar/notification icon visual acceptance; physical multi-monitor validation when hardware exists; idle-host Desktop process-start timing and existing font-alias notice follow-up. The 72 explicit bootstrap-context lint exceptions need a typed injection migration during redesign. Final accessibility, Linux/Windows native certification, release packaging and broader whole-repository tidy are future phases, not claims of this report.

## 17. Commit Readiness

Changes are organized for review: typed voice/runtime/tests; startup/model-library safety; onboarding/accessibility; safe QML cleanup; analysis tooling and documentation. No commit or push was created, per this task's explicit instruction. Technical gates are green and the implementation is reviewable for deliberate scoped commits. Native acceptance remains PARTIAL and must not be relabeled as fully certified.

FINAL VERDICT: PARTIAL
