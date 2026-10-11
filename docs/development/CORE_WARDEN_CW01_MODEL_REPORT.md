# SENTINEL — CORE WARDEN CW-01 MODEL REPORT

## Environment and scope

Blender **5.2.2 LTS**, build `d13f752e3b9c`, macOS ARM64. Reference inspected: `assets/core-warden/reference/Core_Warden_Original_Reference.png`; the requested lowercase filename was absent. Personal-Brain was not available on PATH (`command -v brain` returned 1), so the supplied workspace was inspected directly. Repository architecture, runtime, security, building and testing instructions were read.

Existing changes in ObservationPolicy.cpp, its test, assets and the review directory were preserved. No application source, icon, daemon, TUI or Qt/QML changes. No commits, pushes, downloads or animation renders.

## Deliverables and method

- Master: `assets/core-warden/blender/core_warden_cw01.blend`
- Builder: `assets/core-warden/scripts/build_cw01.py`
- Validator: `assets/core-warden/scripts/validate_cw01.py`
- Previews: `assets/core-warden/renders/preview/cw01_front.png`, `cw01_three_quarter.png`, `cw01_side.png`, `cw01_back.png`, `cw01_hero.png`
- Asset documentation: `assets/core-warden/README.md`

Procedural UV spheres, closed beveled panel meshes, and converted beveled curves create actual editable geometry. No billboards, textures or supplied renders are used in the model. Panel face normals are recalculated consistently. The collection `CW01 | Independently animatable parts` contains 22 meshes / 7,808 base vertices. Lights and five named cameras occupy the scene's stage collection. Mesh bevels remain non-destructive.

Independent parts include Core_Head, Face, Left_Eye, Right_Eye, three Upper_Armor segments, Lower_Armor, Left_Protective_Plate, Right_Protective_Plate, six Rear_Armor segments, Floating_V_Component, its light accent, two hinges and two plate accents. Object-centered pivots allow future transform animation; no armature, actions or state animation is authored. Eye emission is keyframeable; each eye has a separate Glacier material for independent emission channels. All ten requested states remain future animation work.

## Materials and preview configuration

Official sRGB hex colors are converted to linear shader values: Obsidian #151719, Porcelain #ECEFEE, Graphite #303438, Glacier #A9CAD3. Principled surfaces use roughness 0.23–0.30, metalness 0.15–0.70, and Glacier emission strength 5. No bloom/compositor or particles. Eyes are mesh surfaces.

Previews: CPU Cycles, 16 samples, 640 × 640, PNG RGBA, transparent film. All five cameras use orthographic scale 3.65 and the same aim point, with distinct front, three-quarter, side, rear and hero positions. Saved master uses Blender Eevee for interactive previews. No large texture sets, simulations or subdivision modifiers. M2 runtime performance has not been benchmarked; these are modest offline previews.

## Commands and execution

Executed from `<LOCAL_HOME>/Desktop/Projects/Sentinel`:

```sh
command -v brain
git status --short
/Applications/Blender.app/Contents/MacOS/Blender --version
/Applications/Blender.app/Contents/MacOS/Blender --background --python-exit-code 1 --python assets/core-warden/scripts/build_cw01.py > assets/core-warden/blender/build.log 2>&1
/Applications/Blender.app/Contents/MacOS/Blender --background --python-exit-code 1 --python assets/core-warden/scripts/build_cw01.py -- --skip-preview-renders > assets/core-warden/blender/material-finalization.log 2>&1
/Applications/Blender.app/Contents/MacOS/Blender --background assets/core-warden/blender/core_warden_cw01.blend --python-exit-code 1 --python assets/core-warden/scripts/validate_cw01.py > assets/core-warden/blender/validation.log 2>&1
```

Initial sandboxed background startup exited 139 during Metal device detection, before Python execution. The same command succeeded outside the sandbox. Subsequent builds corrected inconsistent panel normals and refined taper, plate depth, rear coverage and V shape. A first mesh-validation pass failed on disconnected curve-cap vertices in Left_Eye; the builder now welds converted curve vertices and recalculates normals. The final geometry-only rebuild also separates eye materials; existing previews retain the same visible geometry and material values. Logs are local ignored artifacts. Blender emitted a nonfatal USD architecture warning and a `Material.use_nodes` deprecation warning; future Blender 6 API compatibility is not promised. `--python-exit-code 1` ensures Python exceptions yield failure; process crashes retain native failure codes.

## Fidelity and corrections before animation

This is a first procedural interpretation, not a pixel-perfect reconstruction or an approved final character. It preserves the dark spherical core, black face, inward-slanted luminous eyes, segmented porcelain crown, detached side protection, lower curved mask and floating V. Distinct camera views expose real volume.

The concept's sculpted asymmetric crown, faceted elongated fins, complex wraparound cheek shells, recessed rear seams and broad side armor require further sculpting. Current armor uses simplified radial strips; the side silhouette is less organic, eyes have simpler ends, and the V has a tubular rather than blade-like cross-section. Rear coverage is inferred from the small concept study and does not reproduce its precise panel layout. Mechanical attachments intentionally meet the core; no automated collision certification is claimed. Lighting makes the Glacier eyes appear paler than the concept's cyan glow.

Before animation: review silhouette and thickness at all five angles, reshape crown and cheeks, refine the floating V profile, tune eye shape/emission, and decide a common head-motion parent. No animation should begin before visual approval.

## Validation

Validation results are recorded after reopening the final saved master in a separate Blender process. Scoped asset validation is appropriate here; C++/Qt tests are not run because no application code or build configuration changed.

Final build and independent reopen validation both exited **0**. `CW01_BUILD_OK`: 22 mesh objects, 7,808 base vertices. `CW01_VALIDATION_OK`: required independently named meshes exist, material slots are populated, eye materials are independent, every part's base mesh has manifold edges, five cameras have distinct positions, and all five PNGs decode as 640 × 640 RGBA with both transparent and opaque pixels. All five views were visually inspected. No missing components were observed; exact collision-free clearance and reference fidelity remain manual design-review items. The side view especially exposes the simplified slab-like plate profile. No animation or product integration was started.
