# SENTINEL — LEGACY BRANDING PURGE REPORT

Date: 2026-10-07. Scope: production identity selectors, obsolete assets, provenance gate and current-host branding verification. Earlier Brand Asset Integration and Phase 4 histories remain intact. No commit or push.

## 1. Audit

1,119 existing tracked/non-ignored source files audited before this report (1,120 including it). Inventory lists 157 individual asset/font/package/removal rows and 49 source/configuration reference rows, plus documentation and replaced selectors. Four obsolete application icons were already removed by the preceding integration; four remaining pastel gradient installer bitmaps and four CPack selections were removed here. A notification tray system-theme selection was replaced with the authoritative resource. See `LEGACY_BRANDING_INVENTORY.md`.

## 2. Authoritative Brand Root

`resources/branding/` is the sole production identity source. The supplied 69-file package is unchanged; all 67 manifest-listed hashes match. Qt/install/bundle outputs derive from it. The new dependency-free `tools/branding/check.py` rejects legacy production selectors, independent visual masters, changed package inventory/hashes, changed FA-3, retired app compositions and non-monochrome SVG trays. CTest runs the gate. Negative probes for a retired reference and an independent SVG were rejected and removed. Selected assets are required at CMake configuration time.

## 3. App Icons

macOS: current `app-icon/macos/sentinel.icns`, `CFBundleIconFile=sentinel.icns`; the rebuilt test bundle contains only this ICNS, byte-identical SHA-256 `fc7804aa5725dca5e26d32ee381120e299e9ba3c99cfbd3a37e49b8eb4347f67`. Windows: RC/executable and NSIS/WiX use `app-icon/windows/sentinel.ico`. Linux: scalable SVG and all nine hicolor PNG sizes install under the existing `dev.sentinel.Sentinel` identity; desktop, AppStream and RPM rules agree.

## 4. Tray

macOS template @2x, Windows monochrome raster and Linux fallback/symbolic families remain under `branding/tray`. Qt's `:/branding/tray.png` selects the correct platform input. NativeCompanionAdapter already consumes this alias. Local notification tray icon selection now directly consumes it instead of a system-theme notification glyph. Toggle, activation, message, mask, shortcut and click-routing behavior are unchanged. Current Dock/menu bar identity was positively confirmed by the user.

## 5. QML / Qt Resources

No old artwork alias remains. The compatibility `:/icons/dev.sentinel.Sentinel.png` points to the current Linux 512px application raster; it is not an obsolete source file. `:/branding/sentinel-lockup.svg` and `:/branding/tray.png` select authoritative wordmark/tray assets. Functional Lucide/Tabler icons remain functional. Missing selected inputs produce a configure error; package loss/hash changes fail the gate.

## 6. Product Surfaces

Real isolated onboarding/welcome screenshot shows current Quiet Field header icon and FA-3 wordmark. Onboarding finish, main/title/splash aliases resolve to the same current raster. No separate About page or independent Settings/About logo was found; System settings presents semantic application version/platform information. Settings functional icons and semantic Sentinel labels remain unchanged. No QML presentation edits were needed in this purge.

## 7. Docs / Social

README references the supplied current `docs/readme-header.svg`; avatar/social-preview remain approved package files. Their supplied editorial Graphite fields are preserved; they are not retired app-icon side planes. No independent current documentation screenshot artwork was found. Historical integration/Phase 4 reports and explicit retired-study provenance remain historical. `docs/reference/BRANDING.md` now documents the gate, removed installer art and source policy.

## 8. Packaging

macOS bundle/CPack use the current ICNS; no custom DMG artwork selected. Windows product, executable, installer and shortcuts use the current ICO. Four obsolete decorative bitmap selections are gone; NSIS/WiX standard layout artwork remains without introducing a new Sentinel variant. Linux scalable/hicolor/desktop/AppStream/RPM paths use the authoritative package. Native Windows/Linux installer certification remains deferred.

## 9. Removed Assets

This purge removes `packaging/windows/installer/nsis_header.bmp`, `nsis_welcome.bmp`, `wix_banner.bmp`, `wix_dialog.bmp`, after reference analysis and visual inspection. Their only production selectors were the four removed CPack variables. The preceding four `resources/app-icons/dev.sentinel.Sentinel.{svg,png,ico,icns}` deletions remain preserved. No package manifest asset was removed.

## 10. Retained Historical Assets

No independent historical visual master remains in production resources. Historical prose in development reports and the frozen package's explicit retired-study provenance remains. Functional icons/fonts/licenses are retained. Old ignored build outputs are classified separately below, not counted as current authoritative sources.

## 11. FA-3 Integrity

Exact path preserved: `M16 16H44V32L68 20L84 40L68 52L80 76L56 88L40 64L16 72V52L28 44L16 36Z`.

The original validator was executed on a disposable byte-for-byte package copy, preserving the repository package. PASS: 12 SVGs, 50 PNGs, 26 monochrome PNGs, eight ICO representations, eleven ICNS entries, exact two-shape Obsidian/Porcelain application raster reference, tray light/dark alpha parity, geometry and manifest hashes. No export or new artwork was produced. The repository gate also passes.

## 12. Real macOS

The actual rebuilt test app launched with isolated temporary UI preferences and no daemon auto-start; onboarding/welcome were visually verified. Dock and menu bar: user confirmed current FA-3. Finder: current-run observation pending; automated inspection was rejected because Finder held unrelated private document content. No bypass attempted; user-assisted observation requested. Prior Finder acceptance remains recorded in the integration history but is not substituted for this run.

User clarified that the last old symbol was seen in a notification. Current macOS notification delivery uses UNUserNotificationCenter and contains no independent legacy icon or attachment selector: the OS obtains the icon from the sender application identity. All audited bundles use `dev.sentinel.Sentinel`. Old ignored outputs contain the former ICNS:

- `build/debug/apps/sentinel-desktop/sentinel-desktop.app`: stale old ICNS retained; plist selects current name, which is absent in that stale output.
- `build/no-ccache/apps/sentinel-desktop/sentinel-desktop.app`: stale old ICNS and plist explicitly selects the old filename.
- `build/apps/sentinel-desktop/sentinel-desktop.app`: both current and old ICNS; plist selects the current file.
- `build/tests/apps/sentinel-desktop/sentinel-desktop.app`: sole current ICNS and current plist, newly built/signed.

No matching sentinel-desktop.app was found under system/user Applications in the bounded scan. The stale bundles are plausible registration/cache ambiguity sources, not proof of which bundle produced the user's historical notification. No macOS icon/notification databases were edited and artwork was not changed to compensate. New visible native notification icon acceptance is NOT VALIDATED; prior notification closure remains PARTIAL.

## 13. Static Platform Check

Windows and Linux static selectors: PASS. No retired asset paths remain in production source/configuration; selected application icons derive from the root. Native platform certification remains deferred.

## 14. Regression

Configure and full build PASS. CTest 113/113 PASS, 85.53 seconds, including the new gate; Controller 127, Shell 74, desktop IPC 38 and native integration 3 PASS. Rust workspace 17/17 PASS; fmt/clippy PASS. Initial restricted Rust socket tests were retried with required local-socket access and passed. Both IPC generator checks PASS. Full QML lint executed with exit 0 and existing warnings; no clean-warning lint claim. No QML changed here. clang-tidy remains BLOCKED by previously documented Apple PCH/SDK incompatibility; it was not rerun or claimed PASS. `git diff --check` PASS.

Evidence logs are ignored `build/branding-purge-*`; the package validator wrote only its disposable copy. The initial restricted GUI launch could not access macOS GUI services; the normal GUI-enabled launch succeeded. This was a sandbox limitation, not an application regression.

## 15. Hygiene

Nothing staged; no commit/push. No ZIP, screenshot, cache, temporary export, bundle or dependency was added to tracked production sources. Inventory/report contain repository-relative paths. Temporary probes, inspection PNGs, disposable validation copy and isolated UI preferences were removed. The isolated client was quit and the normal user profile relaunched. Existing dirty Phase 3/branding/Phase 4 changes and user data were preserved. Stale ignored build bundles were documented rather than modifying their binaries or clearing global OS caches.

## 16. Remaining Work

Obtain current-run Finder confirmation and a newly displayed native notification showing the current sender icon. If a fresh notification still uses old art, identify the registered sender bundle before any cache-specific remediation. Rebuild or retire stale ignored desktop bundles before using them; they are not validated current builds. Native Windows/Linux acceptance and prior notification delivery/click acceptance remain deferred. No redesign or new identity work is needed.

## 17. Commit Readiness

Scoped source purge/gate is implemented and regression passes. Review the existing large dirty worktree before staging; this task staged nothing. Complete the remaining native observations before declaring complete visual closure.

**FINAL VERDICT: PARTIAL — production source purge and FA-3 integrity pass; current notification sender artwork and Finder observation remain unconfirmed.**

## Native closure continuation — 2026-10-07

The preceding PARTIAL verdict is preserved. This continuation changes no production artwork, FA-3 geometry, application logic, platform configuration or tests.

### Exact current bundle and sender

- Fresh current-source build: `cmake --build --preset tests`; fresh install into `/private/tmp/sentinel-native-closure/sentinel-desktop.app` (the `/tmp` spelling resolves to the same directory).
- Bundle identifier and signing identifier: `dev.sentinel.Sentinel`.
- `CFBundleIconFile`: `sentinel.icns`.
- Actual bundled icon: `/private/tmp/sentinel-native-closure/sentinel-desktop.app/Contents/Resources/sentinel.icns`.
- Bundled SHA-256: `fc7804aa5725dca5e26d32ee381120e299e9ba3c99cfbd3a37e49b8eb4347f67`.
- Authoritative `resources/branding/app-icon/macos/sentinel.icns` SHA-256: `fc7804aa5725dca5e26d32ee381120e299e9ba3c99cfbd3a37e49b8eb4347f67`.
- Standard local ad-hoc bundle signing and `codesign --verify --deep --strict` succeed. Install-time executable adjustment was followed by signing; filename matching alone was not used.
- Desktop sender PID 44252 is verified by `lsof` to execute this exact fresh bundle. macOS usernoted records an audit-token PID 44252 and a modern notification connection for `dev.sentinel.Sentinel`. No older desktop process or alternate Sentinel notification helper was active.
- macOS notification icons come from the signed application/bundle identity; the current UNUserNotificationCenter path has no separate icon/attachment selector. No new notification logo was created.

### Stale bundle and process classification

Four owned stale build/test desktop bundles removed safely, after confirming no desktop process used them:

1. `build/debug/apps/sentinel-desktop/sentinel-desktop.app` (selected ICNS missing; retained old ICNS).
2. `build/no-ccache/apps/sentinel-desktop/sentinel-desktop.app` (selected legacy ICNS).
3. `build/apps/sentinel-desktop/sentinel-desktop.app` (older executable, both current and legacy ICNS).
4. `build/certification/phase3-closure-oct5/cold-ui/SentinelCold.app` (old certification copy using the production identifier).

The user's existing daemon PID 23477 was preserved. Its argv contained an old desktop path with parent traversal, but `lsof` proves its actual executable is `build/apps/sentinel-daemon/sentinel-daemon`, outside the removed desktop bundle.

LaunchServices also retained absent test-copy registrations at `/private/tmp/sentinel-cert.BtzLP1/Sentinel.app`, `/private/tmp/s3cold-zkp2qxwt/SentinelCold.app`, and `/private/tmp/sentinel-brand-closure-install/sentinel-desktop.app`. Unregister-by-path for absent bundles returned 1. The supported `lsregister -gc -f` operation then garbage-collected those old build/test records and registered the fresh bundle. No database was manually edited, deleted or reset; Finder was not restarted.

Final production-identifier registrations are the current test build, the fresh installed bundle, and a historical `/Users/emir/.Trash/Sentinel Desktop.app` registration. The Trash copy was not removed or inspected beyond targeted registration metadata; it is not an active process or the proven sender. Inactive settings/audio certification probes have separate identifiers (`dev.sentinel.SettingsCertification`, `dev.sentinel.AudioCertification`), were not notification senders, and are retained as test outputs. No system/user Applications installed Sentinel bundle was found in the bounded scan.

### Real notification test and visual acceptance

A disposable daemon, preferences, local model and fixture were used; user data and daemon were untouched. The first file-reading Agent run failed and is not counted as completion acceptance. A second real tool-free Agent run reached terminal Completed: session `e217cd14-0bc3-4559-98fa-81e2c6e80f39`, run `8e9bdeb5-eaf8-4aa1-9070-443cb2d133c7`. At 20:19:14 Europe/Istanbul, usernoted accepted notification `sentinel-ced80ff4-a240-4694-afc7-3e6c284fd8a1` for `dev.sentinel.Sentinel` and recorded banner presentation. This establishes current sender identity/API processing, not user-visible delivery or artwork acceptance.

The user reported no system notification before the terminal completion test. A separate observation request was made for the new completion banner and Notification Center; no positive observation was received before this report. Finder observation for the exact fresh bundle also lacks current-run confirmation. Prior user-confirmed Dock/menu bar acceptance remains preserved; fresh-bundle confirmation was requested. The earlier private-document Finder rejection was respected by using user-assisted observation instead of reading unrelated content.

Root cause classification: stale builds and dangling registrations were concretely found and cleaned; there is no current resource/hash mismatch or alternate notification artwork selector. The cause of the user's historical old notification icon is not proven. An OS cache/stale-registration explanation remains plausible, not a confirmed causal finding.

### Scoped validation and hygiene

Fresh build/install, signature verification, branding integrity, both generators and `git diff --check` PASS. Affected CTest checks: branding sources and native integration 2/2 PASS (native 3 Qt cases). Prior full baseline remains C++ 113/113; Controller 127, Shell 74, IPC 38, native 3; Rust 17/17, fmt/clippy PASS. No broad baseline rerun was needed without source edits. clang-tidy remains previously BLOCKED by Apple PCH/SDK issues.

Nothing staged, committed or pushed. Evidence and helper scripts remain ignored under `build/branding-native/`. The isolated client was normally quit, its owned model/daemon/supervisor and log stream were stopped, and its disposable profile/socket were removed. The user daemon remains untouched. The fresh bundle is retained for inspection and relaunched with the normal user profile; it is a native acceptance output, not a production source or staged artifact.

Continuation verdict: **PARTIAL — exact current sender and icon integrity proven; fresh Finder and notification-icon visual observations are pending.**
