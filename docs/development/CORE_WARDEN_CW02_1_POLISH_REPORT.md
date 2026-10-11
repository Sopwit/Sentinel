# SENTINEL — CORE WARDEN CW-02.1: FINAL VISUAL POLISH

Status: **Ready for visual approval. No animation or TUI integration started.**

## Deliverables

- [Editable CW-02.1 master](../../assets/core-warden/blender/core_warden_cw02_1.blend)
- [Reproducible polish script](../../assets/core-warden/scripts/polish_core_warden_cw02_1.py)
- [Reopen validator](../../assets/core-warden/scripts/validate_core_warden_cw02_1.py)
- [Comparison/readability script](../../assets/core-warden/scripts/compare_core_warden_cw02_1.py)
- 512px: [front](../../assets/core-warden/renders/cw02_1/front.png), [three-quarter](../../assets/core-warden/renders/cw02_1/three_quarter.png), [side](../../assets/core-warden/renders/cw02_1/side.png).
- [768px hero](../../assets/core-warden/renders/cw02_1/hero.png), [96px](../../assets/core-warden/renders/cw02_1/size_96.png), [128px](../../assets/core-warden/renders/cw02_1/size_128.png).
- [Original concept / CW-02 / CW-02.1 comparison](../../assets/core-warden/renders/cw02_1/comparison.png).
- Evidence: `assets/core-warden/blender/cw02_1_manifest.json`, `cw02_1_validation.json`, `cw02_1_readability.json`. Local build and validation logs are ignored by the asset `.gitignore`.

## Visual changes

The script opens the existing CW-02 master directly. It does not invoke earlier builders, reconstruct the character, create replacement objects or add meshes. The original concept board remains the primary visual reference.

| Focus | Change |
|---|---|
| Dark spherical face | Obsidian and face shaders remain exactly unchanged. Larger rectangular key/fill lights, balanced cool rim light, ambient strength 0.50 and exposure 0 make the sphere more readable while retaining a dark center. Draft reflections were softened before final rendering. |
| Eyes | Preserve the diagonal lens topology and conforming depth. Increase width 28%, length 3.5%, and reduce the diagonal angle from 38° to 35.5°. Existing emission controls default to 4.8 rather than 3.2. Glacier base and emission colors remain #A9CAD3. |
| Floating shells | Apply a matching 19° outward turn to each existing shell/liner/trim assembly through vertex coordinates, add 0.085 units of lateral separation and a slight forward offset. More Porcelain is visible from the front and three-quarter views, with clearer floating gaps and dark interiors. |
| Crown | Extend the dominant upper sweep by up to 0.12 units horizontally, with a small forward/upward adjustment. Apply the same mapping to its trim; slightly lower the smaller right crown for stronger asymmetry. |
| Lower V | Widen all three existing V components 10%, deepen them 10%, increase height 5.5%, and reduce the Porcelain bevel from 0.022 to 0.016. The accent remains on the existing emission channel, now defaulting to 1.8. |

No bloom, particles, textures or new decorative components were introduced. Cheek and rear topology/design are retained for this narrowly scoped polish pass.

## Structure and control preservation

The final master retains **29 meshes, 10,256 base vertices and 32,220 evaluated triangles**, matching CW-02. Base vertex/edge counts and edge/polygon connectivity are unchanged. Vertex edits use each object's world/inverse matrices, leaving object transforms, local pivots, parent inverse matrices, names, collections and parenting intact. Core/head and face vertex coordinates remain unchanged.

All eight controls remain: CTRL_Float, CTRL_HeadMotion, CTRL_LeftShell, CTRL_RightShell, three CTRL_Upper_Armor_* controls and CTRL_LowerV. The original eye-left, eye-right and accent property bindings remain functional. New emission defaults are the only intended control-property changes. No keyframes, actions, armature or animation sequences exist.

## Execution and validation

Blender **5.2.2 LTS**, ARM64 macOS, build `d13f752e3b9c`. Personal-Brain remains unavailable on PATH; the supplied workspace and repository instructions were used. All existing application changes were preserved.

Actual commands executed from `<LOCAL_HOME>/Desktop/Projects/Sentinel`:

```sh
/Applications/Blender.app/Contents/MacOS/Blender --background --python-exit-code 1 --python assets/core-warden/scripts/polish_core_warden_cw02_1.py -- --draft > assets/core-warden/blender/cw02_1-draft.log 2>&1
/Applications/Blender.app/Contents/MacOS/Blender --background --python-exit-code 1 --python assets/core-warden/scripts/polish_core_warden_cw02_1.py > assets/core-warden/blender/cw02_1-build.log 2>&1
/Applications/Blender.app/Contents/MacOS/Blender --background assets/core-warden/blender/core_warden_cw02_1.blend --python-exit-code 1 --python assets/core-warden/scripts/validate_core_warden_cw02_1.py > assets/core-warden/blender/cw02_1-validation.log 2>&1
<LOCAL_HOME>/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/bin/python3 assets/core-warden/scripts/compare_core_warden_cw02_1.py
```

The draft, final build, independent reopen validation and comparison audit exited **0**. Source compilation passed for all three new scripts. Blender used macOS graphics access, as earlier sandboxed Metal initialization had failed. A nonfatal `Material.use_nodes` deprecation warning remains under the installed API.

Verified in the reopened file:

- Exact preservation of CW-02 object/mesh names, edge/polygon topology, pivots, transforms, parent inverses, parenting and collection membership.
- Exact preservation of all recorded Obsidian Principled shader input values; official colors remain numerically valid.
- Unchanged head and face vertex coordinates.
- Manifold meshes, positive signed volumes and no missing external texture dependencies.
- Translation **and rotation** tests on every control affect only the expected descendant meshes. Test changes were restored in memory and never saved.
- Independent left/right eye emission and the accent emission channel respond to their existing properties.
- No detected evaluated outer-armor/head or outer-armor/face surface intersections. Intentional embedded eyes and mechanical supports are excluded from that clearance test.
- Six valid RGBA PNGs at exactly the requested sizes, all with transparent backgrounds, opaque character pixels and clear image borders.
- SHA-256 checks confirm both earlier masters, their protected source files, all earlier preview PNGs and the original reference remain unchanged.

All six final previews and the comparison were visually inspected. At 96px, bright Glacier-colored eye pixels increased from **24/26** to **35/36** across the two eye regions: **42% more combined**. At 128px, the counts increased from **41/46** to **58/60**. Crown, side shells and V remain distinguishable at native sizes.

In a matched front-view face region excluding eyes, mean display luminance rose from **14.75 to 22.03 / 255**, while the 95th percentile rose from 67.86 to 77.65. This supports increased dark-face visibility without a shader color change; it is a lighting proxy, not an aesthetic score. The comparison uses original before/after outputs; the concept's viewpoint and lighting are not pixel-aligned.

## Performance and remaining differences

The master is **259,822 bytes** (about 254 KiB). Geometry density is unchanged. Saved viewport engine: Eevee. Offline renders: CPU Cycles, 24 samples, fixed seed 2, denoising. The six final images took **9.631 seconds of recorded render time** on this run, excluding startup/save/validation; this is not a general M2 benchmark. No new texture memory, simulation or runtime processing is introduced.

The crown remains more angular than the concept's sculpted sweep. Side shells retain their simplified, comparatively symmetric leaf profile; cheek contours and rear layout remain the CW-02 interpretation. Eyes are fuller but still tapered lenses, with paler Glacier highlights and a much subtler halo than the concept. Broad face reflections remain more apparent than in the approved illustration. The V is clearer at small sizes but has simpler edge detailing. Static clearance does not certify future animated poses.

No AgentRuntime, daemon, Desktop, CLI, TUI, application icons or playback code was modified. C++/Qt builds were not run because the change is confined to authoring assets and their scripts/documentation. No downloads, commits, pushes or merges were performed.

**Stop for visual approval. Further geometry changes or animation production require the next user instruction.**
