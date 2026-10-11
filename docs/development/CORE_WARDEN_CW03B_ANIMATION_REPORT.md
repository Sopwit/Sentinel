# SENTINEL — Core Warden CW-03B: remaining seven animations

2026-10-11. **Asset production complete; visual approval pending.** No CW-05 or new production event integration was begun. Existing CW-03A outputs, approved masters and V4-B evaluation artifact are preserved.

## Deliverables and behavior

New editable derivative: `assets/core-warden/blender/core_warden_cw03b.blend`. It opens from CW-03A, retains its three Actions and adds seven Actions with eight object slots / 75 F-curves each. No geometry was rebuilt. The 29 meshes, topology, names, local pivots, parenting, collections, Obsidian material, existing Glacier/white palette, hero camera, lighting and Eevee/AgX setup remain. Only established controls/emission properties are animated; scale channels stay constant. No limbs, mouth, new objects, compositor bloom or hue flashing.

| State | Frames per size | FPS | Playback / duration | PNG bytes (three sizes) | Motion |
| --- | ---: | ---: | --- | ---: | --- |
| Listening | 16 | 8 | loop / 2s | 1,071,943 | Attentive small whole-body tilt, subtle head response and soft eye breathing. |
| Processing | 12 | 8 | loop / 1.5s | 806,417 | Two-cycle structured eye/mechanical activity with stronger outward shell travel. |
| Approval | 24 | 8 | loop / 3s | 1,605,310 | Slow, quiet nod/float and open waiting pose; no fabricated approval. |
| Speaking | 16 | 8 | loop / 2s | 1,071,152 | Rhythmic two-cycle nod, shell response and eye activity; no mouth. |
| Success | 13 | 8 | once → final hold / 1.625s | 872,646 | Restrained upward/nod acknowledgment, gentle eye lift, then neutral hold. Visual asset only. |
| Warning | 16 | 8 | loop / 2s | 1,072,812 | Small alternating caution tilt and eye imbalance, without harsh flashing or new warning colors. |
| Error | 13 | 8 | once → final hold / 1.625s | 869,413 | Brief downward bow and softer eyes, then neutral hold; shared error label remains authoritative. |

Success/Error have 13 samples: 1.5 seconds from first to final pose, followed by the final sample's 125 ms display interval; subsequent playback holds the last neutral pose. Their curves have zero endpoint velocity, no Cycles modifier and constant extrapolation. They do not generate Agent outcomes. Long-lived error meaning remains in the label/state; a persistent bowed failure pose was not added.

Five loops use analytic periodic keys and tangents; frame 1 and closure N+1 match pose/velocity. Export omits the duplicate closure key. All seven start neutral; one-shots finish neutral. This provides neutral-boundary transitions, **not arbitrary mid-animation blending**. Existing three state Actions remain selectable through the embedded CW03A helper. New states use embedded `CW03B_select_state.py`: run it in Blender's Text Editor, then `select_state('listening')` etc. `neutral_pose()` restores captured rest transforms and emissions. No automatic script execution or runtime Blender requirement.

## Files and previews

- PNG sequences: `assets/core-warden/animations/{listening,processing,approval,speaking,success,warning,error}/{96,128,192}/frame_000.png` onward.
- New animated lossless alpha WebP: `assets/core-warden/previews/cw03b/{state}.webp`, `{state}_128.webp`, `{state}_96.webp`.
- Convenience GIFs: `previews/cw03b/{state}.gif`, with deliberate dark opaque backing. GIF centisecond timing approximates 8 FPS (13-frame one-shots total 1620 ms); canonical PNG/WebP timing is exact 125 ms per sample / 1625 ms one-shot.
- **Single all-ten review page:** `assets/core-warden/previews/cw03b/approval.html`, native 192/128/96px previews, labels, timing and replay button. It references CW-03A previews without copying or modifying them. It opens offline; the temporary localhost QA server was stopped after review.
- All-ten native-size contact sheet: `previews/cw03b/all_ten_contact_sheet.png`; representative four-pose strips: `{state}_strip.png`.
- Source manifest and validation: `blender/cw03b_manifest.json`, `cw03b_validation.json`, `cw03b_frame_validation.json`, `cw03b_packaging_validation.json`.
- Audit snapshots, logs, 462 frame checksums, browser loaded-image evidence and screenshot: `docs/reviews/core-warden-cw03b/`.

## Manifest and shared contract compatibility

`animations/manifest.json` is extended to CW03B with ten descriptors. Existing Idle/Thinking/Tool Running descriptors compare exactly equal to the before snapshot, including their paths/timing/Actions. The original metrics remain historical CW03A data; new metrics are explicitly under `cw03b.metrics`.

The shared contract stays schema 1. **All ten existing `states` mappings and labels remain exactly unchanged**: the three existing animations stay active; seven others remain `animation: null` with the existing static fallback. Optional `review_only_animations` describes the seven new candidates, timing, loop/hold and zero-based hold index, with activation `pending_visual_and_event_mapping_approval`. It is asset metadata, not a new daemon event or production transition.

The only Rust production-source adjustment is `sentinel-mascot/src/assets.rs`: accept either the legacy exact three-state catalog or the exact extended ten-state catalog, validating known frame counts, 8 FPS, loop flags, canonical bounded paths and existing containment rules. No rendering, player scheduling, outcome selection, Desktop or TUI layout changes. Seven staged entries do not become active because the authoritative mapping remains null. A regression tests extended acceptance, legacy acceptance and invalid staged timing rejection; existing tests verify the seven remain one-frame static fallbacks.

**Deployment boundary:** the protected V4-B package retains its older binary and three-state assets together and was not rebuilt. A source build consuming the extended repository manifest requires this compatible validator; an older binary pointed directly at the new ten-state source catalog can fall back to text. Do not replace packaged assets independently of their matching binary. Future release staging/notice/inventory updates are separate work.

## Size and memory accounting

New PNGs: **330** (110 per resolution), **7,369,693 bytes** (7.028 MiB). Existing PNGs: 132, unchanged. All ten states: 462 PNGs, 154 samples per size. Lossless optimization reduced the new PNG baseline from 9,334,974 bytes by 21.05%; decoded pixels are checked before/after. New preview directory: 7,129,966 bytes. New master: 320,937 bytes. Previews/Blender are not runtime requirements.

Decoded RGBA8 for all 110 new samples at one size (excluding decoder/allocator/texture/protocol overhead):

| Resolution | Raw bytes | MiB |
| --- | ---: | ---: |
| 96 | 4,055,040 | 3.867 |
| 128 | 7,208,960 | 6.875 |
| 192 | 16,220,160 | 15.469 |

All new variants together would use 27,484,160 bytes (26.211 MiB). Decode only one size/state as needed. Largest new clip (Approval, 24 samples) is 884,736 / 1,572,864 / 3,538,944 raw bytes at 96/128/192px; existing TUI active caches remain unchanged. Compressed PNG size is not runtime RSS. No new-state terminal CPU/FPS claim is made.

Offline final export: Blender 5.2.2 LTS arm64, Eevee 32 samples, transparent 192×192 RGBA8. Render calls total **30.130s**, mean 0.274s/frame, range 0.238–0.443s, excluding startup/validation/packaging. This is a local offline sample, not a guaranteed M2 runtime budget.

Temporal revisits produce exact duplicate pixel poses (Processing 1, Success 3, Error 4 in the final render). These are retained as ordered canonical samples for loop/reverse/hold timing, not separate asset pipelines. No CW-03A resources are copied into a second runtime tree.

## Validation results

- Reopened new master passes: 29 meshes, eight independent controls, topology/parenting/pivots/collections and Obsidian values preserved; eye/accent drivers function.
- **223 half-frame/closure pose samples** across seven states: no head/face versus detached outer-armor crossings and no new pairwise outer-armor crossings. Embedded lenses/internal mechanics are excluded, as in CW03A. This is sampled clearance, not a continuous collision proof.
- Five loops pass position/velocity seam checks. One-shots pass zero endpoint velocity and actual held-curve evaluation beyond end frame.
- **462 PNGs pass** decode, exact RGBA dimensions, manifest/path/count checks, transparency and fully transparent borders. Every 96px frame has two bright separated eye islands; new-state minimum is 15 pixels (Error). This bounded metric supplements visual review and does not certify human recognition.
- All 30 WebP variants pass exact sample count/duration, alpha equality and dark-composite pixel equality against canonical frames. New sequences have distinct 96px temporal pixel signatures; that is not a perception guarantee.
- **155 protected files** match pre-task SHA-256: four earlier masters, CW-03A PNGs/previews and the V4-B archive. Existing manifest descriptors and all shared mappings are unchanged. `frame-sha256.json` accounts for all final PNGs.
- **122 Rust tests pass** (14 CLI, 11 IPC, 22 mascot, 75 TUI). Formatting, workspace/all-target Clippy with `-D warnings` and diff whitespace pass. Restricted socket fixtures ran with authorized local-socket access. No C++/daemon test/build or release-candidate rebuild was needed for this asset/validator scope.
- The offline review page was opened in the in-app browser; 30 images loaded at correct native dimensions, replay was exercised and a screenshot retained. Contact sheet/Warning/Error strips were inspected. This is sampled visual review, not continuous human motion approval.

An initial restricted Blender process crashed during macOS graphics initialization; the dedicated graphics-enabled background rerun succeeded. First small-size QA caught a Success edge halo; only its lift was reduced. A first motion review found weak state distinction; final restrained whole-mascot tilt/nod and Processing shell travel were added and the full new set re-rendered/revalidated. Earlier masters/outputs were not altered to fix these issues.

## Reproduction and remaining integration

Run in dedicated Blender processes, never an unsaved interactive scene:

```sh
/Applications/Blender.app/Contents/MacOS/Blender --background --python-exit-code 1 --python assets/core-warden/scripts/build_core_warden_cw03b.py
/Applications/Blender.app/Contents/MacOS/Blender --background assets/core-warden/blender/core_warden_cw03b.blend --python-exit-code 1 --python assets/core-warden/scripts/validate_core_warden_cw03b.py
/Applications/Blender.app/Contents/MacOS/Blender --background assets/core-warden/blender/core_warden_cw03b.blend --python-exit-code 1 --python assets/core-warden/scripts/export_core_warden_cw03b.py
python3 assets/core-warden/scripts/package_core_warden_cw03b.py
python3 assets/core-warden/scripts/validate_core_warden_cw03b_frames.py
```

Python packaging/validation needs Pillow (12.3.0 verified using the bundled workspace Python). Scripts reuse the CW03A Action/export/optimization approach, with separate CW03B outputs and merged catalog. Re-running only packaging on already optimized sources changes the pre-compression baseline; reproduce build → export → package for fresh metrics. The historical CW03A packager writes its own three-state catalog; use the CW03B merge packager after it if regenerating those earlier sources.

Visual approval must confirm small-size distinctions, restrained motion feel, one-shot neutral hold behavior and the inherited Eevee response versus CW02.1's Cycles static hero. Neutral-boundary pose continuity is provided; arbitrary mid-state transitions need future blending/hold/recovery policy. Future UI work must validate authoritative Voice/Agent event mappings, cancellation/reduced-motion/static semantics and one-shot state ownership before enabling these candidates. Tool completion must never imply Agent success. No new event mappings, AgentRuntime/daemon changes, Desktop/Quick Panel/TUI redesign or FA-3 icon changes occurred.

Personal-Brain was unavailable; the supplied workspace/local instructions were used. Concurrent and uncommitted work was preserved. No commit, push, merge, release publication or CW-05 work. **Stop for visual approval.**
