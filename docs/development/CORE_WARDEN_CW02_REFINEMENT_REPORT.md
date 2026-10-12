# SENTINEL — CORE WARDEN CW-02 REFINEMENT REPORT

> SEC-P1.1 privacy edit (2026-10-12): machine-specific paths are shown as role placeholders; recorded measurements and acceptance results are unchanged. Command examples use `SENTINEL_REPO`; set `export SENTINEL_REPO="$(git rev-parse --show-toplevel)"` from a checkout first. This edited report has a new file digest; original raw evidence is unchanged.

Status: **Static review deliverable; visual approval required before CW-03 animation.**

## Deliverables

- [CW-02 editable master](../../assets/core-warden/blender/core_warden_cw02.blend)
- [CW-02 procedural refinement builder](../../assets/core-warden/scripts/build_core_warden_cw02.py)
- [Independent Blender validator](../../assets/core-warden/scripts/validate_core_warden_cw02.py)
- [Comparison/readability script](../../assets/core-warden/scripts/compare_core_warden_cw02.py)
- [Front](../../assets/core-warden/renders/cw02/front.png), [three-quarter](../../assets/core-warden/renders/cw02/three_quarter.png), [side](../../assets/core-warden/renders/cw02/side.png), [back](../../assets/core-warden/renders/cw02/back.png), [hero](../../assets/core-warden/renders/cw02/hero.png)
- [96px](../../assets/core-warden/renders/cw02/size_96.png), [128px](../../assets/core-warden/renders/cw02/size_128.png), [192px](../../assets/core-warden/renders/cw02/size_192.png), [side-by-side comparison](../../assets/core-warden/renders/cw02/comparison.png)
- [Asset structure and controls](../../assets/core-warden/README.md)
- Machine evidence: `assets/core-warden/blender/cw01_scene_inspection.json`, `cw02_build_manifest.json`, `cw02_validation.json`, `cw02_readability.json`.

## 1. CW-01 baseline assessment

The original concept board, CW-01 master, builder, five previews, report, materials, transforms and cameras were inspected. Reference: `assets/core-warden/reference/Core_Warden_Original_Reference.png`. CW-01 established a recognizable spherical guardian, but crown strips resembled a segmented ring, side plates were slab-like, rear plates formed a radial fan, eyes were rectangular, and the V had a tubular section. Large reflections weakened the dark core.

CW-02 **opens the existing CW-01 master**. It retains all 22 original named objects and refines their mesh data in place. Original Core_Head and Face vertex coordinates are retained exactly; hinges retain their geometry with reduced object scale. Seven supporting mesh objects are added. This is a derivative refinement, not a scene reset or an imported replacement mascot.

SHA-256 preservation checks pass for the CW-01 master, builder, validator, original reference and all five CW-01 previews. The baseline master remains:

`d3e8af57b1f9d9ef5e2a44ecdb8c5ea25826b1c06700abb96e6ba3a30a13d570`

Personal-Brain was unavailable on PATH (`command -v brain` returned no executable). The supplied repository was inspected directly; local architecture, runtime, security, building and testing instructions were read. Existing and concurrent application changes were preserved. This work edits only mascot assets, their documentation and this report.

## 2. Exact geometry modifications

| Part | CW-02 refinement |
|---|---|
| Core / face | Preserve smooth ellipsoid topology; add a slender graphite orbital support and reduce hinge prominence. |
| Crown | Replace three radial strips with asymmetric swept blade meshes, a separated diagonal tip, transverse depth, explicit graphite undersides and a fine Glacier edge. |
| Side protection | Replace arc slabs with twisted, tapered closed shells. Cubic interpolation smooths the longitudinal silhouette; cross-section ridges, curled lips, separate graphite liners and luminous front grooves supply depth. |
| Lower face | Refine Lower_Armor into the central chin section; add separate left/right cheeks. Curl inner edges forward to clear the face, with physical thickness and two deliberate seams. |
| Floating V | Replace the tube with a closed beveled hard-surface chevron, defined central point, separate graphite backing and an inset Glacier accent. |
| Rear | Replace the pinwheel with six curved protective sections, a central seam and coherent side/upper/lower panel boundaries. |
| Eyes | Replace rectangular curves with tapered rounded lens meshes conformed to the preserved face ellipsoid. Each has a separate material and emission control. |

All 29 mesh objects are editable, closed and manifold with positive signed volume. Restrained bevel modifiers remain editable. No billboard, texture-based character substitute, particles, subdivision, armature or simulation is used.

## 3. Material improvements

The official sRGB values are converted to linear shader inputs and numerically validated: Obsidian **#151719**, Porcelain **#ECEFEE**, Glacier **#A9CAD3**, Graphite **#303438**.

Obsidian metalness/roughness: 0.18/0.37; face roughness: 0.32. Porcelain: 0.12/0.29. Graphite: 0.62/0.34. These changes restrain broad specular highlights and separate armor from internal supports. Glacier eye emission defaults to **3.2**, with accents at **1.25**. AgX Medium High Contrast, exposure −0.25, neutral area lights and restrained ambient illumination produce the review renders. No bloom/compositor is required for recognition.

## 4. Animation readiness

Collections separate Controls, Core, Armor, Mechanical, Accents and Stage. Stable axes: +Z up, −Y forward. Eight plain empty controls provide:

- `CTRL_Float`: entire character motion.
- `CTRL_HeadMotion`: core, face, eyes, cheek and rear assembly motion.
- `CTRL_LeftShell`, `CTRL_RightShell`: independent shells with their liner/trim children.
- Three `CTRL_Upper_Armor_*` controls: independent crown segments.
- `CTRL_LowerV`: V, backing and luminous trim together.

On CTRL_Float, `eye_left_intensity`, `eye_right_intensity` and `accent_intensity` drive emission. Changing one eye intensity was verified to leave the other unchanged. Every transform control was moved temporarily in memory, its expected children checked, and its original transform restored. Component world positions survive parenting.

There are no actions, state animations, keyframes, armature or animation sequences. All ten future states remain unimplemented. Rotation limits, expression shape keys, timing and collision clearance during animated poses still require design work after approval.

## 5. Blender script execution

Installed and verified: **Blender 5.2.2 LTS**, ARM64 macOS, build `d13f752e3b9c`. Commands executed from `<repository-root>` include:

```sh
/Applications/Blender.app/Contents/MacOS/Blender --version
/Applications/Blender.app/Contents/MacOS/Blender --background assets/core-warden/blender/core_warden_cw01.blend --python-exit-code 1 --python assets/core-warden/scripts/inspect_cw01.py > assets/core-warden/blender/cw01-inspection.log 2>&1
/Applications/Blender.app/Contents/MacOS/Blender --background --python-exit-code 1 --python assets/core-warden/scripts/build_core_warden_cw02.py -- --views front three_quarter --samples 16 > assets/core-warden/blender/cw02-draft.log 2>&1
/Applications/Blender.app/Contents/MacOS/Blender --background --python-exit-code 1 --python assets/core-warden/scripts/build_core_warden_cw02.py -- --views front three_quarter side back --samples 16 > assets/core-warden/blender/cw02-draft.log 2>&1
/Applications/Blender.app/Contents/MacOS/Blender --background --python-exit-code 1 --python assets/core-warden/scripts/build_core_warden_cw02.py -- --skip-renders > assets/core-warden/blender/cw02-geometry-check.log 2>&1
/Applications/Blender.app/Contents/MacOS/Blender --background --python-exit-code 1 --python assets/core-warden/scripts/build_core_warden_cw02.py > assets/core-warden/blender/cw02-build.log 2>&1
/Applications/Blender.app/Contents/MacOS/Blender --background assets/core-warden/blender/core_warden_cw02.blend --python-exit-code 1 --python assets/core-warden/scripts/validate_core_warden_cw02.py > assets/core-warden/blender/cw02-validation.log 2>&1
python3 assets/core-warden/scripts/compare_core_warden_cw02.py
```

Final build, separate-process reopen validation and comparison/readability script all exited **0**, with `CW02_BUILD_OK`, `CW02_VALIDATION_OK` and `CW02_COMPARISON_OK`. Python source compilation also passed. `--python-exit-code 1` propagates script failures. The builder checks required inputs, saves recovery geometry before renders, uses fixed geometry/cameras/seed, and verifies protected baseline hashes.

Draft inspection caught a parenting transform error, dim initial driver evaluation, a nonmanifold orbital-ring closure, and cheek/core intersections. These were corrected and rechecked. A combined edit/Blender permission review timed out without executing; a split-command retry succeeded. Blender ran with macOS graphics access because sandboxed Metal startup had failed during CW-01. Material/World `use_nodes` deprecation warnings are nonfatal in the installed release.

## 6. Preview validation

Five final views use **640 × 640**, **24 CPU Cycles samples**, seed 2, denoising and orthographic scale **3.65**. Front, three-quarter, side and rear share neutral reference lighting and transparent RGBA backgrounds. Hero uses a dark opaque background. The saved master uses Eevee for interactive previews.

The reopened master passed:

- All required parts, material assignments and exact official base/emission colors.
- 29 manifold editable meshes, valid outward volumes and eight independent transform controls.
- Independent eye emission driver behavior.
- Five distinct camera positions at consistent orthographic scale.
- Eight PNGs decoding at their required sizes; requested transparency preserved and silhouettes clear of the image frame.
- No external texture/file dependencies, actions or armature.
- No detected evaluated outer-armor/head or outer-armor/face surface crossings. Embedded eyes and internal mechanical supports are intentional and excluded from that clearance test.
- Unchanged baseline SHA-256 checksums.

A geometry-only build and the final full build produced the same mesh-audit signature (`5c23ebf7dfbbee9493f51bf5e6121c6c749a297861bc6ae94f5430c05f203284`), covering part counts, volumes and parent relationships. This is semantic audit consistency, not a promise of byte-identical Blender files or renders across platforms.

All eight final renders and the comparison were visually inspected. At 96px, 128px and 192px, both eyes, asymmetric crown, side shells and detached V remain distinguishable. The readability audit counted bright Glacier pixels in each eye region: **24/26**, **41/46**, and **85/91**, respectively. Those counts support visibility; they do not measure aesthetic quality or actual terminal compatibility.

## 7. Concept fidelity assessment

CW-02 is closer in crown sweep, shell depth, dark interior separation, lens shape, chevron cross-section and rear completeness. The approved concept remains the visual authority. The comparison places its main character crop beside CW-01 and CW-02, with four CW-02 angles and native-size tests below. It is a qualitative comparison with different source lighting, not a pixel-aligned reconstruction test.

The procedural model keeps the spherical guardian, diagonal illuminated eyes, asymmetric floating crown, porcelain protection, curved lower mask and detached V without adding human limbs, mouth or hair.

## 8. Performance considerations

Final model: **29 meshes, 10,256 base vertices, 10,600 base faces, 32,220 evaluated triangles**. The `.blend` is approximately **254 KiB** (260,073 bytes at delivery). No texture sets, downloads, simulations or high-resolution subdivision. Final eight renders took approximately **13.67 seconds of recorded render time** on this local run, excluding startup/save/validation; this is not a general M2 performance benchmark.

Normal Sentinel execution remains unaffected: these are offline authoring assets. No daemon, Desktop, Rust/Ratatui, icon, playback or runtime integration changes were made. C++/Qt builds and tests were not run because this work changes only static assets and their generation/validation scripts. Logs and Blender backups are ignored by the asset-local `.gitignore`.

## 9. Remaining visual differences

The crown still has a more angular, planar sweep than the concept's sculpted aerodynamic shell. Side protection is more symmetrical and leaf-like, with a broad dark interior visible in front and three-quarter views. The cheek band remains more regular and segmented; its newly curled interior is geometrically clear but needs a design review against the concept's flowing cheek corners.

Eyes are tapered lenses rather than the reference's fuller rounded strokes, and their cyan halo is restrained without bloom. Rear seams and coverage are inferred from small reference studies; the layout is intentionally coherent but not exact. Graphite crown/V undersides are darker in the rear view. The V is now dimensional but its edge profile and central light remain simpler than the concept.

Automated clearance covers the static outer armor against the head/face, not all possible moving poses or every pair of internal parts. No claim of perfect fidelity or production-final approval is made.

## 10. Recommendation for CW-03

Obtain visual approval of the crown/cheek silhouette, side-shell width, rear layout and eye treatment using the comparison and native-size tests. If revisions are requested, refine those static shapes before motion work. After explicit approval, begin a short low-cost idle/listening motion test using the existing controls, define safe movement ranges and review a single small output before producing the ten state animations.

**Stop here: no CW-03 animation production, playback integration, commits, pushes or merges were started.**
