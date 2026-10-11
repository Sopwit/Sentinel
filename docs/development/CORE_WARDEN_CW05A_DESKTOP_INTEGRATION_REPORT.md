# Core Warden CW-05A — Desktop integration

Date: 2026-10-11. Scope: native Desktop presentation only. CW-05B has not started.

## Implementation

`Sentinel.Warden` is a native, typed Qt QML module backed by `CoreWardenItem` (`QQuickPaintedItem`). It consumes the canonical animation manifest and shared state contract without changing either. The seven CW-03B candidates are explicitly resolved by the Desktop adapter because their artwork was accepted for this phase; the existing TUI mappings remain untouched.

A QtConcurrent worker reads and validates metadata and decodes only the selected state's 192px frames. There is one outstanding load per item. Generation tokens discard obsolete results; a new request waits for the outstanding worker rather than accumulating jobs. `takeResult()` releases the future's ownership of decoded images. State/configuration changes release the previous active cache immediately. Hidden, minimized and Off presentation stops the timer and releases the cache; a bounded in-flight load can finish and is discarded. Destruction is safe because workers capture values, never the QQuickItem.

The precise Qt timer uses the manifest's validated 8 FPS / 125ms interval. Frame ticks update only the item, reuse decoded QImages and never parse metadata or decode PNGs. Looping wraps; Success/Error stop on the final frame. Smooth image scaling preserves transparent edges. Missing or malformed assets use a safely resolved static Idle image if available, otherwise `[ S ]`, plus a readable artwork-unavailable label.

## Placement and settings

- Empty Chat: centered above the existing greeting, 144 logical pixels, reduced to 96 below 650px surface height. Existing header, rail, composer and transcript widths remain unchanged.
- Active Chat: 48px feedback above the existing runtime strip, only while existing send/Agent busy projection is active and the surface is at least 600px high. No message overlay or permanent panel.
- Appearance settings: Off, Static, Animated and Auto, stored through the existing `DesktopSettingsStore` local presentation policy under `coreWardenMode`. No daemon setting or second production configuration file.
- Existing reduced-motion preference forces static playback. Animated and Auto both animate only authorized activity; inactive/disconnected presentation stays static even when Animated is chosen.
- Explicitly labeled synthetic previews support all ten states. Their selection is local QML presentation, not persisted and never injected into runtime state. Reduced motion also applies to previews. Preview can intentionally be inspected while production presentation is Off.
- Accessible graphic/state labels and existing keyboard-operable combo boxes are used. Unknown connection/readiness is labeled Inactive or Disconnected, never Ready merely because a transport connects.

## Production mapping

Inputs come from existing `DesktopShellViewModel` projections, backed by the existing session/run-filtered `DesktopRuntimeClient`. No new event subscriptions, session manager or backend projection was added.

| Evidence | Presentation |
| --- | --- |
| Daemon unavailable | Idle artwork, static, Disconnected label |
| Existing genuine approval-required projection | Approval |
| Accepted Chat sending/streaming lifecycle | Thinking |
| Actual failed send lifecycle | Error |
| Connected inactive/unknown/queued/completed/other | Idle artwork, static, Inactive label |

Listening, Processing, Tool Running, Speaking, Success and Warning are fully supported artwork states but preview-only in this integration. This patch does not reinterpret active-tool identifiers, Agent busy flags or tool-completed events as proof of inference, accepted tool execution or accepted final success. Voice and further Agent mappings require separately reviewed authoritative evidence. Success artwork is never emitted on tool completion.

## Asset discovery and installation

Order: explicit `SENTINEL_CORE_WARDEN_ASSET_ROOT`; executable-relative macOS `../Resources/core-warden`; executable-adjacent `core-warden`; `../share/sentinel/core-warden`; then up to eight executable ancestors containing `assets/core-warden` for development. No compiled developer home path or current-working-directory dependency. An explicit invalid override intentionally falls back rather than silently selecting different assets.

CMake bundles/installs only canonical PNGs, manifest and state contract. It includes the canonical three sizes without generating duplicate pipelines, Blender sources or previews. macOS uses bundle Resources; Linux uses share/sentinel; Windows uses executable-adjacent assets. Only macOS was executed. Desktop playback currently selects 192px; missing that resolution safely falls back. All reads reject absolute/traversal/backslash paths and canonical paths outside the selected root, including escaping symlinks. JSON is capped at 128KiB, frame count at 32, encoded frame size at 1MiB and decoded dimensions at 192×192. Metadata identity/schema, counts, FPS and loop/hold semantics are checked.

The branding source guard now distinguishes accepted `assets/core-warden/` mascot work from application/tray branding. The FA-3 geometry, inventory and all 69 branding-file hashes remain checked and unchanged. This is not public distribution or artwork-license clearance.

## Measurements and asset evidence

- Observed player: 8 frame events in approximately 1,052ms, unchanged 24-frame active cache.
- Hidden player: zero frame events during a 300ms observation, zero active cached frames.
- Largest active RGBA frame cache: 24 × 192 × 192 × 4 = 3,538,944 bytes (3.375 MiB). Static image: 147,456 bytes. These are frame-cache measurements/accounting, not whole-process RSS. Qt painter/texture and temporary decoder overhead is additional.
- All 462 PNGs validated: ten states, dimensions, RGBA and alpha range; encoded total 10,320,312 bytes. All 154 installed 192px frames matched canonical hashes.
- 155 protected CW-03B input/output/V4-B artifact hashes remain unchanged.
- No production CPU/RSS benchmark is claimed. Actual Desktop native display and previews were observed; no runtime inference benchmark was performed.

Evidence: `docs/reviews/core-warden-cw05a/asset-validation.json`, native screenshots and copied validation/build logs in the same review directory.

## Verification

- `cmake --preset tests`: PASS.
- `CCACHE_DISABLE=1 cmake --build --preset tests -j6`: PASS. Initial ccache attempt failed because its external cache directory is sandbox-restricted; disabling ccache resolved it.
- Full CTest with local socket/process permissions: **123/123 targets PASS**, 32.06 seconds. Initial sandbox run failed in IPC/subprocess tests; those failures disappeared with the permitted execution. The remaining initial branding classification failure was corrected as described above.
- Focused Qt player tests cover all ten states, loop/hold, active-cache replacement, stale loads, static/off/reduced motion, hide/show, timing/no hidden wakeups, preferences, missing/invalid/traversal inputs, corrupt PNG/JSON and escaping symlinks. Final focused results: 9 passed, 0 failed (including QtTest setup/cleanup); recorded in `player-tests.log`.
- Native Qt/Metal disposable Chat flow: PASS after replacing an early fixed-delay message-row assertion with a bounded readiness condition. Empty conversations, conversation switching, failed-response bubbles, composer alignment and keyboard focus passed. Native images were captured at 1100×780, 780×640 and 1600×960. The 560×640 exploratory fixture is below the production window's unchanged 780×640 minimum and exhibits clipping; it is not an acceptance PASS.
- Offscreen Chat flow: PASS structurally, but its software renderer did not render existing MultiEffect-backed panels. Blank software-rendered captures were discarded; native captures provide the visual evidence.
- Qt QML lint target: exit 0. It reports existing context-property/unqualified-access warnings, including new references to the injected preference object. The native player import is resolved through generated QML type metadata; no unresolved player import remains. This is not a warning-free lint claim.
- Scoped clang-format and diff whitespace checks: PASS.

## Actual GUI observation and limitations

The built production macOS Desktop was launched with `--no-daemon-start` and a deliberately absent socket. Native computer-use inspection observed Core Warden immediately with Disconnected labeling, before any keyboard entry. Actual Settings → Appearance navigation, all ten labeled synthetic state previews and keyboard selection of Off/Static/Animated/Auto were observed. Off hid the Chat mascot. Auto was restored and the synthetic preview cleared. Screenshots include `production-disconnected.png`, `production-off.png`, `production-settings.png` and `production-preview-*.png`. Normal application exit returned status 0.

Connected live Chat/Agent/voice activity, all overlay combinations and long-running production CPU/RSS remain physical acceptance work. Unit tests and a disposable UI fixture do not establish those production backend flows. Linux/Windows native execution and platform accessibility remain pending. 192px source images can be interpolated at 144 logical pixels on Retina; no new higher-resolution artwork was generated. Existing translations may fall back to English for new labels.

## Modified files

- `apps/sentinel-desktop/CMakeLists.txt`
- `apps/sentinel-desktop/bootstrap/ApplicationBootstrapper.cpp`
- `apps/sentinel-desktop/include/sentinel/desktop/CoreWardenItem.h`
- `apps/sentinel-desktop/src/CoreWardenItem.cpp`
- `apps/sentinel-desktop/include/sentinel/desktop/DesktopSettingsStore.h`
- `ui/qml/components/base/CoreWarden.qml`
- `ui/qml/components/chat/HomeChatSurface.qml`
- `ui/qml/pages/settings/AppearanceSettingsTab.qml`
- `tests/CMakeLists.txt`
- `tests/desktop/test_core_warden.cpp`
- `tests/helpers/settings_ui_certification_probe.cpp`
- `tools/branding/check.py`
- This report and `docs/reviews/core-warden-cw05a/` evidence.

Pre-existing concurrent changes were preserved. This phase did not modify AgentRuntime, daemon, Rust TUI, Quick Panel/Onboarding integration, Blender/artwork, official icons or the V4-B archive. No install, commit, push, merge, tag or release was performed. Personal-Brain was unavailable (`brain` not on PATH); the user explicitly supplied this repository and its local instructions were followed.

## Physical acceptance steps

From any working directory, launch:

```sh
<LOCAL_HOME>/Desktop/Projects/Sentinel/build/tests/apps/sentinel-desktop/sentinel-desktop.app/Contents/MacOS/sentinel-desktop --no-daemon-start --daemon-socket /tmp/sentinel-cw05a-no-daemon.sock
```

1. Touch no keys: verify artwork and Disconnected label appear, with composer unchanged.
2. Settings → Appearance: select Off and return Home; image should disappear. Repeat Static, Animated and Auto; disconnected presentation must remain static.
3. In Appearance, select each Synthetic artwork preview. Confirm labels, transparent edges and distinct motion; Success/Error hold their last pose. Enable the existing reduced-motion preference and verify static previews, then restore your preferred setting.
4. Resize to the supported 780×640 minimum and larger windows, switch views and open/close dialogs. Verify no clipped armor, message overlap or stale artwork.
5. With your existing daemon and configured model, launch normally, send a real Chat request and verify Thinking only during genuine pending work. Inspect actual approval when available. Tool completion must not produce Success; unavailable voice mappings must remain neutral.
6. Verify multiline Turkish draft/focus behavior while moving between views, then exit normally.

## CW-05B recommendation

The reusable player is ready for review and reuse, conditional on the remaining live-backend physical checks. CW-05B should reuse `Sentinel.Warden` and the canonical root policy, explicitly bind surface visibility/reduced motion, and separately validate each authoritative state source before adding mappings. Do not infer readiness or successful outcomes from connection/tool completion. Quick Panel and Onboarding integration were not begun.
