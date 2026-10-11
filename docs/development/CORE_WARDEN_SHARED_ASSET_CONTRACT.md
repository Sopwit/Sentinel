# Core Warden shared asset contract

`assets/core-warden` is the canonical source. CW-04 adds `state-contract.json`, schema version 1, without changing the approved CW-03A animation manifest, PNGs or Blender masters. This is a data/presentation contract, not a new Agent authority or cross-language runtime.

## Lookup and timing

1. Resolve one explicit, trusted installation asset root. Development examples default to the repository's `assets/core-warden`; installed clients should receive the installed shared-data root, for example `share/sentinel/core-warden`. Do not recursively search the filesystem.
2. Read `state-contract.json`; require the supported schema and `sentinel_core_warden` asset ID.
3. Resolve `animation_manifest` relative to that same root. Each animation manifest frame path is also relative to that root, **not** relative to the manifest's `animations/` directory.
4. Select the state mapping. A non-null `animation` names an entry in the manifest's `states`. A null value explicitly means no animated assets exist. Use the declared `static_frame` and status label, or plain status text.
5. Select exactly one resolution. CW-04 uses 96px normally and 128px for roomy empty screens. 192px remains canonical source data but is not enabled in the initial TUI player.
6. Use manifest frame order, FPS, frame count and loop behavior. CW-03A frame selection is `floor(elapsed_seconds * 8) % frame_count`; the last frame is followed by frame 000. Do not duplicate the closing neutral frame or queue delayed timer ticks.

| State ID | CW-03A animation | Explicit missing-state behavior |
| --- | --- | --- |
| idle | idle: 16 frames, 8 FPS | Text if asset/capability unavailable |
| listening | none | Neutral static frame + Listening label |
| thinking | thinking: 16 frames, 8 FPS | Text if asset/capability unavailable |
| processing | none | Neutral static frame + Processing label |
| tool_running | tool_running: 12 frames, 8 FPS | Text if asset/capability unavailable |
| approval | none | Neutral static frame + Approval required label; modal takes priority |
| speaking | none | Neutral static frame + Speaking label |
| success | none | Neutral static frame + Completed label, only after authoritative accepted result |
| warning | none | Neutral static frame + Attention required label |
| error | none | Neutral static frame + Unavailable label |

All missing states reference the existing `animations/idle/96/frame_000.png`. No speculative filenames or new image copies are introduced. Static display mode uses that neutral frame even when an animated clip exists. Disconnection selects the static unavailable presentation; it does not play Thinking.

The Rust implementation validates the current CW-03A phase's three clip counts/FPS, all 132 required filenames, nonzero bounded file sizes and all ten mappings. It rejects absolute paths, traversal, symlinks escaping the asset root, oversized JSON/PNGs, wrong dimensions and non-RGBA8 decoded frames. Later CW-03B assets require an explicit contract/validator update; adding files alone must not silently enable states.

## Application-state ownership

The existing client must authenticate and filter events by session, run, generation and sequence before calling `Projection::event`. This small projection performs no IPC, starts no tasks, runs no tools and changes no Agent policy.

- Connected/ready selects Idle.
- A real Chat/Agent model submission pending, or `run.started`, selects Thinking.
- Only an authoritative, already-filtered `tool.running` selects Tool Running. Neither model prose, `output.delta`, `agent.activity` nor `tool.requested` indicates execution.
- `tool.result` returns an active presentation to Thinking. It is not run completion.
- Approval selects the Approval label/static mapping, while the modal hides the graphic entirely.
- Failure/cancellation and disconnection clear active presentation. A late tool result cannot reopen a completed presentation.
- `run.completed` currently returns to Idle. The player does not claim accepted Agent success. Reserve Success for a future explicitly authoritative accepted-result signal supplied by the existing client.
- Listening and Speaking stay reserved until actual microphone/TTS integrations exist. Do not map generic network traffic to these states.

## Rust TUI foundation

`cli/crates/sentinel-mascot` is a Rust workspace member and production TUI dependency, using Ratatui 0.29, Crossterm 0.28 and ratatui-image 8.0.2. Dependencies resolve through the main workspace lockfile.

`Assets` reads bounded metadata; `Player` owns one background loader and one selected clip cache; `Projection` owns only visible state; `DisplayMode` supports Off/Static/Animated/Auto. `SENTINEL_TUI_MASCOT` extends the existing environment-based presentation preference mechanism. `SENTINEL_TUI_REDUCED_MOTION`, ASCII and NO_COLOR are respected. The production `/settings mascot <mode>` handler changes the process-local mode; `/settings` displays its effective value and launch source. No second settings file or command registry was created.

## Desktop, Quick Panel, Onboarding and Voice

- **Qt/QML Desktop:** resolve the same installed root and JSON mapping through the existing C++ presentation/view-model boundary. Expose a QML-safe state label, ordered URLs and frame interval. Keep QML presentation-only; retain the existing daemon-backed architecture. Read only the selected clip and stop timers/decode work when hidden. Do not embed another Blender runtime or copy sprite assets into another source tree.
- **Quick Panel:** prefer a neutral static frame; if justified later, use a low-rate selected clip only while visible. Share Desktop's runtime client and accepted state, not a parallel authority.
- **Onboarding:** Idle can use today's assets. Success/Listening remain static/text until both the assets and real application state exist. Cosmetic timers must not imply completed setup steps.
- **Voice:** reserve Listening for actual microphone capture and Speaking for actual TTS playback. Do not implement microphone/TTS state in this phase.

FA-3 remains Sentinel's application and tray icon. Core Warden is a presentation mascot; neither the shared contract nor the preview replaces icons. Blender is an authoring dependency only, never a normal client playback dependency.
