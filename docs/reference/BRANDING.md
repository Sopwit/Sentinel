# Branding integration

Status: **PRODUCTION CANDIDATE — PROVISIONAL LOCK**.

The authoritative package lives in `resources/branding/`. The frozen FA-3 path is:

```text
M16 16H44V32L68 20L84 40L68 52L80 76L56 88L40 64L16 72V52L28 44L16 36Z
```

Quiet Field uses a solid Obsidian `#151719` field and Porcelain `#ECEFEE` FA-3. The Graphite side-plane family is retired. Do not re-export the master geometry or introduce decorative planes, gradients, glow, or shadows in these assets. See the supplied `BRAND_GUIDELINES.md`, `manifest.json`, and `validation-report.json` for provenance and file hashes.

## Runtime and packaging

- Qt embeds the Linux 512px app PNG under the existing `:/icons/dev.sentinel.Sentinel.png` alias. Bootstrap/window icon, title bar, splash, onboarding header, and finish step consume that alias.
- The welcome hero embeds the unchanged `wordmark/sentinel-lockup.svg` under `:/branding/sentinel-lockup.svg`. Its currentColor artwork follows the existing foreground theme through QML colorization. Semantic labels remain text.
- Existing native companion and notification tray code use `:/branding/tray.png`. CMake selects macOS 22px template @2x, Windows 32px black, or Linux 32px black fallback. macOS uses QIcon mask semantics. Other monochrome sizes and Linux symbolic SVG remain available. Native tray interaction was accepted in the Phase 4 report; this purge changes asset selection only.
- macOS bundles the sole source `app-icon/macos/sentinel.icns` as `Contents/Resources/sentinel.icns`; CMake and plist agree. CPack uses the same source.
- Windows RC retains its existing manifest/version metadata and consumes `app-icon/windows/sentinel.ico`; NSIS/WiX consume the same ICO. Retired custom installer bitmap artwork and its CPack selectors have been removed; NSIS/WiX retain their standard layout artwork, with the current Sentinel ICO as the product identity.
- Linux installs scalable SVG and 16/22/24/32/48/64/128/256/512px PNGs into hicolor, renamed to `dev.sentinel.Sentinel`. Both RPM file lists match. Desktop ID, AppStream ID, and Flatpak metadata retain their existing identity.
- README uses `docs/readme-header.svg`. `docs/github-avatar.svg` and `docs/github-social-preview.svg` are available for release/repository settings; no remote repository settings were changed.
- Favicons remain in the original package inventory. There is no existing web favicon convention, so the ZIP root aliases are not copied into the repository root.

## Fonts and licenses

Inter and IBM Plex Mono already exist under `resources/fonts/`, with OFL notices in `resources/fonts/LICENSES/`. No font binaries were added. Wordmark/header/social SVG text remains live; external SVG viewers depend on installed fonts/fallbacks. Deterministic text rendering outside the app is not claimed.

## Migration map

| Old asset/surface | Current usage | New asset | Action |
| --- | --- | --- | --- |
| resources/app-icons/dev.sentinel.Sentinel.png | Qt window, splash, title, former tray | app-icon/linux/512x512/apps/sentinel.png; platform tray fallback | REPLACE; old source REMOVE after reference migration |
| resources/app-icons/dev.sentinel.Sentinel.svg | Linux scalable installation | app-icon/linux/scalable/apps/sentinel.svg | REPLACE; old source REMOVE |
| resources/app-icons/dev.sentinel.Sentinel.icns | macOS bundle/CPack | app-icon/macos/sentinel.icns | REPLACE; old source REMOVE |
| resources/app-icons/dev.sentinel.Sentinel.ico | Windows executable/NSIS/WiX | app-icon/windows/sentinel.ico | REPLACE; old source REMOVE |
| Onboarding header/welcome/finish S rectangles | Product placeholders | app PNG / wordmark / app PNG | REPLACE |
| Qt PNG resource alias | Runtime compatibility path | Existing alias points to new PNG | KEEP |
| resources/icons/lucide and tabler | Functional UI icons | Same | KEEP |
| packaging/windows/installer/*.bmp | Retired decorative installer artwork | Standard installer layout; current product ICO | REMOVE (four files and selectors) |
| Existing fonts and OFL notices | Application typography | Same | KEEP |
| ZIP root favicon aliases | Duplicate of branding favicons; no repository consumer | Package originals retained | UNUSED; do not copy aliases |
| docs/avatar and social preview | No automated repository setting consumer | Package assets | UNUSED until explicitly configured |

## Deferred reviews

External recognition review; external similarity/trademark review; native Windows and Linux validation. This status is not legal or trademark clearance. Static Windows/Linux configuration checks on macOS do not constitute native runtime certification.

## Source policy and gate

`resources/branding/` is the sole production identity source. Its frozen manifest hashes cover all 67 listed package files; the manifest and validation report complete the 69-file tree. `symbol/sentinel-symbol.svg` carries the exact FA-3 path; `app-icon/master.svg` carries the two-shape Quiet Field composition. Platform rasters and containers are deterministic derivatives. Keep this validated package intact.

`python3 tools/branding/check.py` (also CTest `test_branding_sources`) rejects retired production paths, independent visual assets outside this root (except functional `resources/icons/`), retired plane selectors, changed hashes and geometry, decorated SVGs, and app-icon or tray color violations. CMake rejects missing selected brand inputs. Compatibility aliases and Linux application IDs name the current artwork; they are not alternate masters. Notification tray selection directly uses the branding resource instead of a system theme icon.

Historical mentions remain in development reports and the package's provenance documents, explicitly describing retired studies. They are not production selections. Do not add historical art to production resources. Ignored build bundles and installed copies must be audited separately; never change the master to compensate for stale output or OS caching.

See `docs/development/LEGACY_BRANDING_PURGE_REPORT.md` and `docs/development/LEGACY_BRANDING_INVENTORY.md` for this purge. `docs/development/BRAND_ASSET_INTEGRATION_REPORT.md` preserves the prior integration history.
