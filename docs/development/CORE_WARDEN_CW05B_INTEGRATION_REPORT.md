# Core Warden CW-05B — Quick Panel and onboarding integration

Date: 2026-10-11. Scope: Qt presentation only. CW-05A report, shared asset contract, architecture, Agent Runtime, security, building and testing instructions were read first. Personal-Brain was unavailable (`brain: command not found`); the user supplied this repository explicitly. Existing concurrent/uncommitted work was retained.

## 1. Quick Panel implementation

`TrayCompanionWindow.qml` still uses its existing `QuickPanelController`, shared `DesktopRuntimeClient`, submission, approval, voice and native dismissal paths. Its header reuses `CoreWarden.qml` / `Sentinel.Warden` at 48 logical pixels only while the existing snapshot is busy or has an approval. It replaces the header wordmark space during that interval; there is no permanent mascot section. Width is capped at 48px, including fallback text, to preserve controls at compact widths. The original title returns when inactive or appearance is Off.

The artwork state comes from the same DesktopShellViewModel inputs and `CoreWardenItem::productionState` as Home. Snapshot busy controls placement, not inference of an animation state. Agent busy alone therefore shows neutral artwork. The accessible graphic retains its state name even though the compact panel omits a redundant visible mascot label. Existing connection, error and approval text remains authoritative.

`surfaceActive` is explicitly bound to panel visibility and the connection-details popup. The shared wrapper also checks actual window visibility/minimization. Hiding a native Window now stops scheduling and releases active frames even if its QQuickItem hierarchy remains logically visible. Reopening loads the current selected state. The existing bounded loader/generation mechanism discards obsolete work. No controller, runtime, event subscription, tray icon or settings store was added.

## 2. Onboarding implementation

The existing seven-step flow remains. Welcome, Provider, Model and Finish show one shared neutral player in the existing header: 96 logical pixels below 700px height and 128px otherwise. Other steps omit it. Onboarding never uses synthetic previews or an elapsed-time completion animation. Existing appearance and reduced-motion preferences apply; Off removes artwork. Missing/corrupt assets retain the existing static/text fallback and do not gate navigation or saved preferences.

Provider, Model and Finish header copy reads existing daemon/readiness projections through the shared presentation helper. Actual availability checks continue through existing provider/runtime services; no fake model request or new verification action was introduced. Unknown, waiting and unavailable evidence stays neutral/static, with the existing health/readiness detail text. There is no authoritative setup-specific verification-in-flight or accepted-inference signal exposed to this component, so neither Thinking nor Success is fabricated for setup.

Finish is labeled “Finish”, rather than “Ready”. Its available-state summary says to send the first message to verify inference. The existing `inferenceReady` property remains an availability/send-readiness gate for compatibility; it does **not** establish successful inference. Provider/model identifiers remain in the existing summary.

Native visual review found Welcome content extending below its card after allocating the mascot header. Welcome, Provider and Model now scroll their existing content inside the card, following Finish's existing pattern. Navigation stays outside the scrolling content. This is a bounded overflow correction, not a new wizard or a content redesign.

## 3. Desktop greeting correction

The random greeting pools included unconditional “Sentinel Active”, “Ready” and assistance claims. Home now keeps the original morning/afternoon/evening/night boundaries (06:00, 12:00, 18:00, 22:00), with neutral Operator greetings. The subtitle updates from existing inputs through `CoreWardenItem::readinessSummary`, also reused by onboarding:

| Existing evidence | Copy meaning |
| --- | --- |
| Daemon disconnected, even if stale readiness says ready | Application open; connect daemon to use inference |
| Daemon connected, availability unknown/unavailable/missing model | Provider/model availability unconfirmed; review Models & Providers |
| Daemon connected, readiness Ready | Provider/model reported available; send a message to verify inference |
| Daemon connected, readiness Busy | An actual request is running; review status |

Provider/model selection is not successful inference. The runtime's Ready descriptor can be based on endpoint health and catalog membership; there is no persistent accepted-inference verification flag in this boundary. No independent connection tracker or runtime readiness logic was introduced. Regression tests exercise disconnected/stale-ready, connected/unconfirmed, Ready and Busy copy.

## 4. Shared state-mapping matrix

All ten assets and explicitly labeled Desktop synthetic previews remain supported by the existing native player. CW-03B candidate resolution remains the explicit CW-05A adapter policy; the historical CW-03A JSON mappings and Rust behavior are unchanged. Asset presence does not enable production events.

| Canonical state | Asset / preview | Available Qt production evidence | Desktop production | Quick Panel production | Onboarding production |
| --- | --- | --- | --- | --- | --- |
| Idle | Yes / Yes | Disconnected or inactive/unknown projection | Static neutral | Static neutral only during placement gate; otherwise absent | Static neutral on selected steps |
| Listening | Yes / Yes | No reviewed mapping into shared Warden projection | Disabled | Disabled, including existing voice controls | Disabled |
| Thinking | Yes / Yes | Accepted Chat sending/streaming lifecycle | Enabled during genuine pending Chat | Same mapping when panel busy | Disabled; setup activity is not Chat submission |
| Processing | Yes / Yes | No authoritative mapped event | Disabled | Disabled | Disabled |
| Tool Running | Yes / Yes | No authoritative tool.running mapped into shared projection | Disabled | Disabled | Disabled |
| Approval | Yes / Yes | Existing genuine approval-required projection | Enabled | Same projection while approval/busy placement gate applies | Disabled |
| Speaking | Yes / Yes | No reviewed TTS playback mapping | Disabled | Disabled | Disabled |
| Success | Yes / Yes | No explicit accepted-result signal mapped here | Disabled | Disabled | Disabled |
| Warning | Yes / Yes | No reviewed warning event mapping | Disabled | Disabled | Disabled |
| Error | Yes / Yes | Existing failed Chat-send lifecycle | Enabled | Same mapping while placement gate applies; terminal inactive panel may omit artwork | Disabled; setup failures retain neutral plus existing status text |

All enabled activity animation is capped by the canonical 8 FPS manifest and the existing mode policy. Static, Off and reduced motion override it; Animated/Auto do not animate neutral Idle. Tool completion, connection, configuration saved and catalog discovery never produce Success. Approval controls/modal precedence and actual run ownership remain with existing services. No AgentRuntime changes were made to force missing states.

## 5. Available and unavailable events

Available: daemon connectivity, readiness/health summaries, selected provider/model, Chat send/stream/failure lifecycle, approval-required projection, panel busy/approval count, and onboarding step/active state. These are existing view-model/controller projections.

Unavailable in this reviewed shared mascot mapping: setup verification-in-flight, successful inference verification history, authoritative tool execution, accepted Agent final success, microphone/TTS animation mapping, and a general Warning event. Existing voice controls are preserved but not promoted to a new cross-surface mascot authority. Agent running without accepted Chat lifecycle evidence remains neutral. This deliberately limits animation while retaining accurate text.

## 6. Performance and automated evidence

The existing player still decodes one selected 192px clip, with one outstanding worker per item and generation rejection. No image copies, alternate asset root, new cache or playback engine were introduced. Maximum selected clip cache remains 24 × 192 × 192 × 4 = 3,538,944 bytes (3.375 MiB); a static frame is 147,456 bytes. Each visible surface owns its own bounded player instance, so simultaneous surfaces can have additive caches. These bounds exclude Qt renderer/decoder overhead and whole-process RSS.

The player timing test measures 8 FPS and zero hidden frame events; the panel regression additionally hides/shows the actual QML Window, verifies zero cached frames/no playback hidden, then verifies Off clears it. No production CPU/RSS benchmark is claimed. Canonical source is 192px, scaled for logical 48/96/128/144px placement; Retina interpolation remains the CW-05A limitation.

All 462 PNGs were decoded and checked for RGBA, exact 96/128/192 dimensions and nonconstant alpha. The branding guard validates all 69 protected branding files, including FA-3. Evidence is under `docs/reviews/core-warden-cw05b/`.

| Verification | Result |
| --- | --- |
| `cmake --preset tests` | PASS |
| `CCACHE_DISABLE=1 cmake --build --preset tests -j6` | PASS; final QML/probe updates also rebuilt with scoped Desktop/probe targets |
| Full CTest, final source, local socket/GUI permissions, 60s per-test timeout | **123/123 targets PASS**, 109.07 seconds |
| Focused `test_core_warden` | 10 passed, 0 failed including setup/cleanup; 8 frames in 1,051ms, 3,538,944 active RGBA bytes, zero hidden events |
| `sentinel-desktop_qmllint` | Exit 0; existing/context-property unqualified-access warnings remain; not warning-free |
| Asset decode validation | 462/462 PNGs; 154 per resolution; RGBA/dimensions/alpha PASS |
| Branding guard | PASS: 69 protected branding files, FA-3 hashes/selectors |
| Native Quick Panel harness | PASS: idle/error/response/approval fixtures, 420/320 widths, keyboard submission/focus/popup, window hide/show/cache release, Off |
| Native onboarding harness | PASS: seven steps, forward/reverse/Back/Space focus, light/dark captures at 1100×780 and 780×640 |
| Missing-assets onboarding harness | PASS: all seven steps and keyboard navigation with explicitly absent asset root |
| Scoped C++ format / diff whitespace | PASS |

An initial CTest invocation overlapped relinking and also lacked local socket/window-server access; its failures are not final validation. Native macOS execution initially aborted inside the sandbox due unavailable GUI services, then passed with permitted window-server access. Final native captures replace the initial clipped Welcome captures. Offscreen structural results are not used as visual rendering evidence.

## 7. Native GUI observations

Automated native macOS harness captures are distinct from production/backend verification. Both light and dark themes were rendered with transparent mascot edges. Onboarding captures cover all seven steps; compact captures use 780×640 at 2× backing scale. Panel fixtures cover idle, error and synthetic approval UI, plus 420/320px structural layout checks. Synthetic fixture labels do not establish real approval/inference events.

The built production Desktop was separately launched with `--no-daemon-start` and an intentionally absent socket. Computer-use accessibility and screenshot inspection observed Disconnected artwork, disabled Send, “Good Night, Operator”, and “Sentinel is open. Connect the daemon to use inference.” No live model request was sent. The launched smoke-test process exited normally. Existing other application instances were not changed.

## 8. Remaining limitations

Live connected provider verification, actual Chat streaming and approval across production surfaces still require physical backend acceptance. Native Linux/Fedora KDE and Windows execution, platform accessibility, production long-running CPU/RSS, and broader translations remain pending. A health/catalog Ready descriptor cannot prove successful inference, and no such claim is made. Setup stays static until an authoritative setup activity signal is exposed through the existing boundary. Preview availability remains separate from production availability.

## 9. Recommendations for UX-C1

Review compact panel placement against live pending Chat and approval; verify popup and keyboard focus with real IPC. Evaluate a single existing-runtime projection for setup verification activity and accepted inference, with session/run provenance, before enabling additional states. Keep success tied to accepted final-result evidence. Check Fedora KDE, Windows and prolonged multi-surface memory behavior. Consider higher-resolution artwork only through the canonical asset pipeline if Retina quality warrants it. Complete translations and contrast/accessibility checks for the new neutral copy.

CW-05B stops at this report. Desktop D2.1 and UX-C1 were not started. No commit, push, merge, publication, Rust TUI, Blender, daemon behavior, AgentRuntime behavior or FA-3 change was performed by this phase.

## Changed source scope

CW-05B edits: `CoreWardenItem.h/.cpp` (shared presentation copy), `CoreWarden.qml` (surface visibility/neutral/compact options), `HomeChatSurface.qml` (greeting), `TrayCompanionWindow.qml`, `OnboardingScreen.qml`, `WelcomeStep.qml`, `ProviderSetupStep.qml`, `ModelSetupStep.qml`, `FinishStep.qml`, `test_core_warden.cpp`, and `settings_ui_certification_probe.cpp`. Other dirty files present at entry belong to pre-existing work. No CMake or settings-storage change was needed for CW-05B.
