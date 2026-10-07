# Sentinel — production-candidate brand guidelines

## Status and evidence waiver

**Status: PRODUCTION CANDIDATE / PROVISIONAL LOCK**

Frozen master: current FA-3. Primary app icon: Quiet Field. Production-candidate assets are locked and exported; asset format/geometry validation passed. External evidence was waived, not completed.

### Deferred before final v1.0 release

- External blind-recognition review.
- External similarity/trademark review.
- Native macOS menu-bar validation.
- Native Windows tray validation.
- Native Linux tray validation.

The identity geometry is frozen unless real evidence later proves a material problem. Provisional export does not convert the historical BLOCKED external-evidence gate into PASS.

The owner explicitly waived Round 07 external evidence to continue the asset export. Round 06 raster validation was completed. Native operating-system integration, external recognition testing and similarity research are not thereby approved. Small-size cross/multiplication-adjacent recognition remains an acknowledged risk. FA-3A and FA-3C are rejected; FA-3B is not shipped.

“External blind-recognition and similarity clearance was deferred. These assets are production-candidate identity assets, not trademark/legal clearance.”

The waiver authorizes these deliverables, not a claim of safety or legal approval. Historical Round 06/07 blocked verdicts remain unchanged in the exploration archive.

## Brand principle

80% Quiet Systems / 20% Technical Presence: calm, precise, capable, premium, technically confident. Architectural restraint, deliberate negative space and strong monochrome mass. Standalone FA-3 is the primary identity. No literal object metaphor or lettermark is introduced. Quiet Field means Obsidian + Porcelain FA-3 only.

## Authoritative geometry

SVG is authoritative; every raster is derived directly from SVG at its intended dimensions.

```xml
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
  <path fill="currentColor" d="M16 16H44V32L68 20L84 40L68 52L80 76L56 88L40 64L16 72V52L28 44L16 36Z"/>
</svg>
```

- Base grid: 4 units. One closed contour; 13 vertices. Do not edit this path.
- Visible bounds: x16–84, y16–88; 68 × 72 units. Bounding-box center: (50,52).
- Area centroid: (47.629,48.344). This is a geometric measurement, not a proven perceptual center. Retain the existing canvas placement.
- Paired diagonals: slope −1/2, run/rise 2:1; −26.565° or 153.435° directed angle. Preserve all other edges by exact coordinates.
- Shoulder: 28-unit width, 16-unit drop before the upper diagonal.
- Re-entrant cut: (16,52) → (28,44) → (16,36); 12-unit depth, 16-unit opening.
- Allowed placement operations: uniform scaling and translation in an application field. No rotation, reflection or contour edits.

## Clear space and minimum sizes

Reserve 12 master units outside the visible silhouette. The standard 100-unit SVG provides only 8 units below the silhouette; supply the remaining clear space in layout rather than altering the viewBox or path. A context needing full isolated clear space may wrap this SVG in a larger layout container.

Use at least 16 CSS px for standalone screen placements; 20–24px is preferred. Treat this as a provisional usage policy, not validated small-size recognition. Required 16px platform assets are included without changing the geometry. Do not use 12px as a planned primary placement. Tray/app platform constraints are documented exceptions to external clear space. Print placements should retain an opening large enough to reproduce reliably; verify the printing process rather than promising a universal millimetre minimum.

## Color palette

| Token | Hex | Purpose |
| --- | --- | --- |
| Obsidian | #151719 | Primary dark field |
| Graphite | #303438 | Quiet spatial plane / secondary surface |
| Fog | #929A9F | Secondary information |
| Porcelain | #ECEFEE | Primary light foreground |
| Glacier | #A9CAD3 | Restrained contextual accent |

Pure black and white are monochrome output variants, not new primary accent colors. Master and tray silhouettes are one solid foreground only.

## Typography and secondary lockup

Inter is the primary face; IBM Plex Mono is for metadata, specifications and terminal text. The lockup uses live lowercase `sentinel.` text in Inter Medium with restrained spacing. The symbol remains separate and primary. The SVG records font-family guidance but does not embed any fonts or raster text. Rendering depends on those faces being available; fallback glyphs can change spacing. Install licensed fonts in the consuming application or explicitly outline approved lettering in a later, separately authorized workflow. Do not represent fallback typography as an exact Inter proof.

## Quiet Field app icon

**Quiet Field:** “A solid Obsidian application field containing the Porcelain FA-3 symbol. No secondary plane or structural accent is used.”

`app-icon/master.svg` uses a 160 × 160 viewBox, the unchanged Obsidian rounded field at (2,2) with 156-unit extent and 34-unit corner radius, and Porcelain FA-3 placed with `translate(8.5 5) scale(1.5)`. Only placement and scale have changed; the symbol path itself is unchanged. The smaller scale provides comfortable outer negative space. The visible bounds are x32.5–134.5, y29–137; the transformed area centroid is approximately (79.944,77.515), close to the horizontal center and slightly above the field's vertical center. There is no side plane, edge, inset, stripe, accent, gradient, glow, glass, stroke border, shadow or lettermark.

Use Quiet Field consistently across macOS, Windows and Linux. The original full-height side plane, Narrow Plane, Inset Plane and Minimal Edge are retired historical studies and must never appear in current production assets. Integrated Field is historical secondary composition vocabulary, not an application icon. OS-2 is supporting vocabulary only; no OS-2 primary assets are included. The rounded field is the approved app-icon container, not part of the standalone symbol. Graphite remains a brand palette color for other surfaces, not part of the Quiet Field app icon.

macOS includes 16/32/64/128/256/512/1024 PNGs plus an ICNS with standard and Retina PNG representations. Windows includes 16/20/24/32/48/64/128/256 PNGs and a PNG-entry multi-resolution ICO for modern Windows. Linux includes a scalable SVG and 16/22/24/32/48/64/128/256/512 hicolor-style directories. Native platform masking and chrome remain integration checks.

## Tray/menu bar and state rules

Tray assets contain only the exact FA-3 silhouette, transparent background, and a monochrome foreground. No app tile or wordmark. The original canvas padding is retained; no extra shrinking for status indicators.

- macOS: black-alpha template PNGs at 16/18/20/22pt-equivalent pixels and @2x physical pixels. Set `NSImage.isTemplate = true` in the application; filenames alone cannot activate template behavior.
- Windows: choose explicit black or white PNG matching the chrome; 16/20/24/32px. Shell requested size and 100/125/150/200% scaling require application testing.
- Linux: a currentColor-compatible symbolic SVG plus black/white PNG fallbacks at 16/22/24/32px. Actual tint support depends on the tray host/theme.

Keep the mark unchanged for Normal, Active, Attention, Error and Offline. Reserve a separate companion slot (8px plus 4px gap at a 20px mark) or a surrounding native field treatment. Suggested indicators: none, steady dot, `!`, `×`, `−`, respectively. Do not spin the symbol, insert the indicator in the cut or compress the symbol to fit it. States require accessible text and menu labels; glyphs alone are insufficient.

## Light/dark, monochrome and accessibility

Use Obsidian/black on light and Porcelain/white on dark. Host applications may set `currentColor` for the primary/tray SVGs; external `<img>` usage does not inherit the surrounding page's text color. Use the explicit black/white files when inheritance is unavailable.

Obsidian/Porcelain have strong foreground contrast. Fog is secondary information, not a default reduced-contrast identity mark. Preserve recognizable contour and readable adjacent text. Provide meaningful alt text in documentation and accessible names in controls. Avoid color-only status. Do not treat contrast checks as recognition or trademark clearance.

## GitHub, documentation and favicon

`docs/github-avatar.svg` uses Quiet Field at 1024 × 1024. `github-social-preview.svg` is 1280 × 640; `readme-header.svg` is 1600 × 480. Maintain generous negative space. Provide alt text and keep important content inside the central safe region. SVG live text uses Inter and IBM Plex Mono without embedded font files. Test font availability and avatar circular cropping in the publishing environment.

The favicon files live at the branding root. `favicon.svg` uses the exact standalone FA-3 and adapts between Obsidian and Porcelain via `prefers-color-scheme`. The 16/32px PNG fallbacks are black on transparent, intended for light backgrounds; use the adaptive SVG for dark tabs. Wiring them into the application or copying them to a web public directory is a consuming-project task, not performed by this export.

## CLI/TUI identity

Use quiet monochrome live terminal text, not a complex raster or ASCII reconstruction of the symbol. A suitable terminal splash is:

```text
sentinel.
Desktop workspace ready.
```

Use IBM Plex Mono when the terminal/user chooses it; never require a bundled font. Keep a plain ASCII fallback. Optional status characters `[ready]`, `[working]`, `[attention]`, `[error]`, `[offline]` carry state without a new lettermark. No animated logo or decorative spinner is required. Do not invent an S, X or multicharacter approximation of FA-3.

## Incorrect usage

- Stretching or changing aspect ratio.
- Rotation or reflection of the identity.
- Random recoloring or new primary colors.
- Glow, glass or uncontrolled gradients.
- Shadows or filters baked into the master symbol.
- Adding an S or merging the symbol into text.
- Changing, filling or moving the internal cut.
- Replacing the master with rejected optical variants.
- Using OS-2, Integrated Field or any retired side-plane composition as an alternate app icon.
- Adding a bar, plane, stripe, edge, accent or decoration to Quiet Field.
- Claiming these assets have legal/trademark clearance or that external recognition risk is resolved.

## Reproduction, validation and handoff

From the repository root:

```sh
node resources/branding/tools/export.mjs
node resources/branding/tools/validate.mjs
node resources/branding/tools/validate.mjs --package
```

The existing `sharp` development dependency renders SVG at the exact target pixel dimensions. Validation uses Node, sharp and Python 3's standard XML parser; no font binaries or new external render service are required. The scripts write only into `resources/branding/`, use memory rather than temp files, and preserve the exact path. `manifest.json` records SHA-256 hashes and renderer versions. `validation-report.json` records file-format/geometry checks, not native OS or legal clearance.

ICNS/ICO validation checks container headers, entry types, bounds, offsets and every PNG payload's decoded dimensions. Native OS loading and application integration remain outstanding. Commit the whole package together after review; no commit is made automatically.

The optional `--package` command creates and independently checks `public/sentinel-brand-production-candidate.zip`. Its contents are exactly the complete `resources/branding/` tree plus `favicon.svg`, `favicon-16.png`, and `favicon-32.png` at the archive root, copied from the branding-root favicon assets. The manifest lists their SHA-256 values as root aliases. The public location enables the preview's download link; the archive contains no application source, build output, browser evidence, scratch review images or dependency directories.
