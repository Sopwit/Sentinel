# SENTINEL — CORE WARDEN CW-03A ANIMATION REPORT

CW-03A provides three editable Blender Actions and complete transparent sprite sequences derived from the approved CW-02.1 master. The 29 mesh objects, original geometry, topology, names, pivots, hierarchy, collections, eight controls and official Obsidian shader are preserved. CW-01, CW-02 and CW-02.1 masters pass SHA-256 preservation checks. This phase ends at visual approval.

## Deliverables

- Editable master: `assets/core-warden/blender/core_warden_cw03a.blend` (281,028 bytes).
- Playback asset manifest: `assets/core-warden/animations/manifest.json`. Frame paths resolve relative to `assets/core-warden/`; every resolution lists every real filename in playback order.
- Source/build evidence: `assets/core-warden/blender/cw03a_manifest.json`.
- PNG sequences: `assets/core-warden/animations/{idle,thinking,tool_running}/{96,128,192}/frame_000.png` onward.
- Animated previews: `assets/core-warden/previews/cw03a/{idle,thinking,tool_running}.webp`, with `_96.webp` and `_128.webp` native-size variants. Convenience GIFs have an opaque dark approval background; WebP and authoritative PNG assets retain alpha.
- Contact sheet: `assets/core-warden/previews/cw03a/contact_sheet.png`.
- CW-02.1 comparison: `assets/core-warden/previews/cw03a/comparison.png`.
- Offline approval page: `assets/core-warden/previews/cw03a/approval.html`; open locally in a browser to play all three states and resolutions.

## Implementation and editing

`CW03A_IDLE`, `CW03A_THINKING` and `CW03A_TOOL_RUNNING` are separate real Blender Actions. Each has eight object slots and 75 F-curves: location, Euler rotation and scale for all controls, plus the three existing root emission properties. Mesh objects are not directly animated. Fake-user retention keeps all Actions in the file.

Periodic sine and one-minus-cosine offsets are added to captured original rest values. Each curve has explicit Bezier tangent handles from its analytic derivative and a Cycles modifier. Frame 1 and closing frame N+1 match the same neutral pose and velocity. Export includes frames 1 through N only, so the closing key does not create a duplicate pause. Positions and first derivatives match at the seam; this establishes smooth periodic interpolation rather than inferring continuity from file counts.

The master opens with Idle selected. In Blender's Text Editor, run embedded `CW03A_select_state.py`, then call `select_state('idle')`, `select_state('thinking')` or `select_state('tool_running')` in the Python Console. The helper assigns all eight corresponding Action slots, sets the frame range and returns to frame 1. Call `neutral_pose()` to detach Actions and restore every captured rest transform and emission property. Executing the helper does not enable automatic file execution. Each Action remains normally editable in Blender's animation editors; assign the complete state using the helper rather than changing only one object's slot.

## Motion

- **Idle:** ±0.025 scene-unit vertical float, very slight head rotation, shared gentle eye breathing, minimal shells and secondary V motion. No spin or bounce.
- **Thinking:** gentler float, subtle head tilt, opposing eye emission changes, small outward/upward crown movement and restrained shell motion. The first crown rotation test crossed the head; the final Action uses a slight lift instead and passes clearance checks.
- **Tool Running:** stronger outward shell travel and mechanical rotation than Idle, stabilized head, a two-cycle eye activity pattern and controlled V lift/rotation. No task data, logs or fabricated progress are displayed.

All states share their neutral start, allowing future state switching at neutral. Blended transitions from arbitrary mid-loop poses are future playback work.

## Export and frame statistics

| State | Frames per size | FPS | Loop duration | PNG bytes across three sizes |
| --- | ---: | ---: | ---: | ---: |
| idle | 16 | 8 | 2.0s | 1,071,235 |
| thinking | 16 | 8 | 2.0s | 1,074,395 |
| tool_running | 12 | 8 | 1.5s | 804,989 |

44 source frames were rendered at 192×192, then downsampled with Pillow Lanczos to 128×128 and 96×96. Total: 132 PNGs, 44 per resolution. No small frame was enlarged for asset production or the comparison sheet. All states use the existing `Camera_hero`, a mild front three-quarter view, orthographic scale 3.65, identical lights and AgX color settings. Both eyes and the detached shells/crown/V remain recognizable at 96px. No compositor bloom or background geometry was introduced.

Reproduce from the repository root in dedicated Blender background processes:

```sh
/Applications/Blender.app/Contents/MacOS/Blender --background --python-exit-code 1 --python assets/core-warden/scripts/build_core_warden_cw03a.py
/Applications/Blender.app/Contents/MacOS/Blender --background assets/core-warden/blender/core_warden_cw03a.blend --python-exit-code 1 --python assets/core-warden/scripts/validate_core_warden_cw03a.py
/Applications/Blender.app/Contents/MacOS/Blender --background assets/core-warden/blender/core_warden_cw03a.blend --python-exit-code 1 --python assets/core-warden/scripts/export_core_warden_frames.py
python3 assets/core-warden/scripts/package_core_warden_cw03a.py
python3 assets/core-warden/scripts/validate_core_warden_frames.py
```

Packaging and image validation require Pillow (verified with 12.3.0). macOS background rendering required graphics access outside the restricted shell sandbox. These scripts replace only CW-03A generated outputs, not earlier phase masters or renders. Run build → export → package to reproduce the stated pre/post compression measurement; packaging already optimized sources again gives a different pre-compression baseline.

## Measured offline performance

- Blender **5.2.2 LTS**, ARM64, build `d13f752e3b9c`.
- **Eevee**, 32 temporal render samples, 192×192 transparent 8-bit RGBA, no ray tracing.
- Total wall time inside the 44 render calls: **11.814s**; mean **0.268s/frame**, range **0.245–0.334s**. This includes each PNG write but excludes Blender startup, Action construction, validation and packaging. It is a local offline measurement, not a guaranteed M2 budget or terminal playback benchmark.
- PNGs before lossless optimization: **3,736,945 bytes**. Baseline: Blender compression 15 for sources and Pillow compression level 1 for downsampled variants.
- PNGs after optimization: **2,950,619 bytes** (2.814 MiB).
- Saved **786,326 bytes**, **21.04%**. Decoded pixel hashes match before/after; no alpha or visible quality loss.
- Exact duplicate frames within each state: **0**. The common neutral image appears once in each independent state; it is retained to keep each sequence self-contained and its timing straightforward.
- Preview directory: **3,283,403 bytes** including WebP/GIF previews, representative strips, contact sheet, comparison and HTML. Authoritative PNG asset size is reported separately above. Master: **281,028 bytes**.

Decoded RGBA8 memory differs from compressed disk size:

| Selected resolution, all 44 frames | Raw decoded bytes | MiB |
| --- | ---: | ---: |
| 96×96 | 1,622,016 | 1.547 |
| 128×128 | 2,883,584 | 2.750 |
| 192×192 | 6,488,064 | 6.188 |

All three resolution variants decoded together would occupy 10,993,664 bytes (10.484 MiB), before allocator, decoder, texture and playback overhead. One Idle/Thinking state requires 589,824 / 1,048,576 / 2,359,296 bytes at 96/128/192; Tool Running requires 442,368 / 786,432 / 1,769,472 bytes. A future player should select one size and decode only what it needs. No Rust TUI FPS, CPU or actual runtime-memory claim is made. Normal future playback needs the PNGs and manifest, not Blender.

## Validation evidence

`blender/cw03a_validation.json`, `blender/cw03a_frame_validation.json` and `blender/cw03a_packaging_validation.json` record:

- Master reopens successfully; three retained editable Actions, eight slots per Action and all eight independent translation/rotation controls work.
- Exact 29-mesh geometry, topology, local pivots, transforms, parenting and collection membership preserved. Obsidian shader unchanged. Earlier masters match their protected hashes.
- All loops match neutral pose and seam velocity; all three eye/accent driver channels remain assigned. Both eye drivers evaluate correctly across animation poses.
- 91 evaluated poses total, including half-frames and the closure pose: 33 Idle, 33 Thinking, 25 Tool Running. No outer-armor/head or outer-armor/face surface crossings, and no pairwise outer-armor crossings. Evaluated modifier geometry is used. Embedded eye lenses and internal mechanical pieces are intentionally excluded from detached outer-armor clearance checks. This is discrete pose sampling, not a continuous collision proof.
- All 132 files decode as RGBA with exact dimensions, complete counts and transparent, unclipped borders. Manifest frame paths exist, and there are no zero-byte/missing frames.
- In every 96px frame, two separate bright Glacier eye islands remain in the bounded face region; minimum measured island area is 17 pixels. This supplements native-size visual review, rather than claiming a general perception metric.
- Lossless animated WebP files preserve correct frame counts and durations: 2000ms / 2000ms / 1500ms. GIF's 120/130ms alternating frame delays approximate 8 FPS because GIF uses centiseconds; WebP and PNG timing remain 125ms per frame.
- Animated WebP playback was opened in the in-app browser and inspected through snapshots at multiple live playback phases at 192/128/96px, alongside the contact sheet. This is not uninterrupted human video observation; final motion feel and seamless-loop visual approval remain with the user.

Pixel difference at last→first, measured as average absolute luma difference on a neutral dark composite (0–255 scale), is 2.434 Idle, 1.679 Thinking, 2.076 Tool Running. Each lies within the full loop's adjacent-step range. The last source frame intentionally differs from the first; periodic closing keys supply the next sample. Combined with tangent/pose checks, this supports continuity without claiming identical first/last exported images.

## Remaining issues and approval boundary

The Eevee response is slightly different from the approved static Cycles hero, notably in broad reflections and shell shading; the comparison labels both engines. Materials and lights were not redesigned to disguise that difference. Eye breathing is deliberately restrained and less obvious at 96px; Thinking and Tool Running favor quiet, professional motion over exaggerated state signaling. Approval should confirm those distinctions are sufficient.

Personal-Brain's `brain` command was unavailable on PATH, so the supplied workspace and local instructions were used. The Git tree was inspected before production and contains unrelated existing/concurrent application changes; this task did not edit them. C++/Qt application builds were not run for these asset-only additions. No application, daemon, AgentRuntime, Rust/Ratatui, Desktop, CLI or FA-3 icon was modified by this work. No remaining states, runtime integration, commit, push or merge was performed.

The assets are ready for visual approval before CW-03B or a separately authorized TUI playback phase. Stop here.
