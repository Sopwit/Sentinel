# SENTINEL — BRAND ASSET INTEGRATION REPORT

## 1. Brand package
Source: sentinel-brand-production-candidate.zip. Status: PRODUCTION CANDIDATE — PROVISIONAL LOCK. 72 archive entries; 69 files under resources/branding copied byte-for-byte. Three duplicate root favicon aliases intentionally omitted. Required symbol, tray, app-icon, wordmark, docs/social, guidelines, manifest, report, and favicon families present. No node_modules, build outputs, browser evidence, temporary/Figma files, or retired plane production assets found. Export and validation source scripts retained for package provenance; not executed as instructions.

## 2. FA-3 integrity
Exact path unchanged: `M16 16H44V32L68 20L84 40L68 52L80 76L56 88L40 64L16 72V52L28 44L16 36Z`. Present in all 12 SVGs. No asset normalized or re-exported.

## 3. Quiet Field
Solid Obsidian #151719 with Porcelain #ECEFEE. Master and Linux scalable SVG contain exactly rect/path children; no secondary plane, gradient, filter, glow, or shadow. App PNGs contain no opaque Graphite #303438 pixels. Original package hashes and supplied pixel-reference validation report preserved. Independent audit did not re-render the master to compare antialiased pixels; source composition and original artifact hashes establish provenance.

## 4. Repository integration
69 package files plus integration documentation added. Four obsolete app-icons sources removed after all production references migrated. Exact inventories follow below. Existing Phase 3 work remains in the checkout and is outside this integration's change list. Personal-Brain unavailable: brain command not found; used the user-supplied repository and local architecture/security/build/test instructions.

## 5. Qt/QML resources
CMake qt_add_resources remains the resource pipeline; no standalone QRC added. Existing app PNG alias preserved. Added branding/tray.png and branding/sentinel-lockup.svg aliases. Only one 512px app PNG, one small platform tray PNG, and one wordmark SVG added to runtime resources. ICNS/ICO and other raster sizes remain packaging inputs. Reviewed settings/About: no existing dedicated About branding page found; no new page created. Title/splash reuse the migrated alias; onboarding placeholders replaced. Existing notification tray fallback migrated.

## 6. macOS
ICNS: valid 11 PNG chunks; unchanged source. Built sentinel-desktop.app and installed bundle contain byte-identical sentinel.icns; CFBundleIconFile=sentinel.icns. CMake/CPack/plist wired. Real final bundle launched using isolated temporary preferences and unavailable daemon socket. Onboarding header Quiet Field and welcome wordmark visibly render. Dock/Finder shell icon appearance NOT VALIDATED; Dock accessibility query timed out. Bundle and Qt window-icon configuration validated statically.

## 7. Windows
ICO directory and eight embedded PNG representations validated. Existing generated RC uses new ICO variable and retains version/manifest metadata. NSIS/WiX icons updated. Static only; executable/installer/taskbar native certification pending.

## 8. Linux
Scalable and nine PNG sizes installed under existing dev.sentinel.Sentinel icon name. Desktop and AppStream XML parse and identity remain unchanged. Both RPM file lists updated. Existing Flatpak rename-icon retained. Static only; no Linux package build/runtime certification performed.

## 9. Tray assets
macOS template, Windows black/white PNGs, Linux symbolic and black/white fallback available. Existing runtime resource selection wired and macOS mask flag retained/applied to notification fallback. NOT VALIDATED — native runtime integration review pending. No behavior redesign or theme-selection logic added.

## 10. Wordmark/docs
README header added; welcome hero uses unchanged live-text lockup with theme foreground colorization. Social/avatar available, no remote settings changed. Added reference branding note/migration map; packaging note updated. Technical docs not decorated.

## 11. Fonts/licenses
Existing Inter Variable and IBM Plex Mono OFL provenance reviewed. No new font binaries. Live SVG text preserved; external font/fallback dependency documented. No deterministic external text rendering claim.

## 12. Packaging
macOS bundle build and staged cmake install PASS; installed ICNS/plist checked. Windows RC/installer icon inputs updated statically. Linux install filenames/size tree and both RPM manifests aligned. Installer bitmaps retained as legacy artwork; no installer redesign. No DMG distribution/signing certification performed.

## 13. Asset validation
PASS: 12 SVG XML parses; exact FA-3; 50 PNG dimensions, CRCs, decoded pixels and monochrome tray/favicons; eight ICO representations; 11 ICNS chunks/header lengths; manifest SHA-256 and byte counts; supplied report SVG/PNG counts. All 69 integrated package files match the extracted package byte-for-byte, including manifest/report. Package's original native OS/external/legal limitations remain in force.

## 14. Regression
CMake tests configure and full build PASS (ccache disabled because external cache directory is sandbox-protected). Initial restricted CTest: 95/111; failures involved forbidden sockets/subprocess sandboxing. Approved unrestricted offscreen rerun: 111/111 PASS, 63.25 seconds. ApplicationController 127/0/0, DesktopShell 74/0/0, Desktop IPC 26/0/0. Rust fmt PASS; approved network-enabled clippy PASS; workspace tests 17 PASS. Final QML changes rebuilt; scoped GUI/DesktopShell rerun 2/2 PASS.

Static limitations: qmllint executed on all three changed QML files, reports existing layout/unqualified-access warnings; not a clean lint PASS. clang-format dry-run on both touched C++ sources reports existing formatting violations (first NativeCompanionAdapter line 126), no broad reformat applied. Configured clang-tidy attempted: Apple PCH incompatible with Homebrew LLVM; analysis-only PCH-free retry also hits SDK/libc++ system-header errors. No clang-tidy PASS claim. git diff --check PASS.

Real UI: final bundle shows header/icon/wordmark and unavailable-daemon/empty-model state. Loading transition/About and full native tray/Dock checks incomplete. Existing onboarding focus issue observed: Tab enters chat composer behind overlay. Navigation click focused Next without advancing in observed state. Responsive zoom action attempted; no independently verified small-window/dark-mode visual certification. These findings were not patched in this branding phase.

## 15. Hygiene
No ZIP/extracted temporary tree/logs/build products added as source. Temporary extraction, validation scripts, staged bundle, and logs remain outside repository. No credentials or Figma files introduced. Paths in source are repository-relative/CMake variables. Existing ignored build directory used for compilation. git status/diff stat/check reviewed; mixed checkout includes pre-existing Phase 3 changes.

## 16. Remaining Work
Verify Finder/Dock and all native tray platforms; Windows/Linux real packaging/runtime; complete dark/responsive/loading visual checks; investigate existing onboarding focus/navigation; clean baseline formatting/QML lint and use compatible clang-tidy toolchain; external recognition/similarity/trademark reviews remain deferred. No legal clearance claimed.

## 17. Commit Readiness
Integration implementation is reviewable; full behavioral regression passes. Strict static/native UI gates remain incomplete, so overall verdict PARTIAL. No commit/push performed.

FINAL VERDICT: PARTIAL

## Exact integration inventory

Added:
- `resources/branding/BRAND_GUIDELINES.md`
- `resources/branding/app-icon/linux/128x128/apps/sentinel.png`
- `resources/branding/app-icon/linux/16x16/apps/sentinel.png`
- `resources/branding/app-icon/linux/22x22/apps/sentinel.png`
- `resources/branding/app-icon/linux/24x24/apps/sentinel.png`
- `resources/branding/app-icon/linux/256x256/apps/sentinel.png`
- `resources/branding/app-icon/linux/32x32/apps/sentinel.png`
- `resources/branding/app-icon/linux/48x48/apps/sentinel.png`
- `resources/branding/app-icon/linux/512x512/apps/sentinel.png`
- `resources/branding/app-icon/linux/64x64/apps/sentinel.png`
- `resources/branding/app-icon/linux/scalable/apps/sentinel.svg`
- `resources/branding/app-icon/macos/sentinel-1024.png`
- `resources/branding/app-icon/macos/sentinel-128.png`
- `resources/branding/app-icon/macos/sentinel-16.png`
- `resources/branding/app-icon/macos/sentinel-256.png`
- `resources/branding/app-icon/macos/sentinel-32.png`
- `resources/branding/app-icon/macos/sentinel-512.png`
- `resources/branding/app-icon/macos/sentinel-64.png`
- `resources/branding/app-icon/macos/sentinel.icns`
- `resources/branding/app-icon/master.svg`
- `resources/branding/app-icon/windows/sentinel-128.png`
- `resources/branding/app-icon/windows/sentinel-16.png`
- `resources/branding/app-icon/windows/sentinel-20.png`
- `resources/branding/app-icon/windows/sentinel-24.png`
- `resources/branding/app-icon/windows/sentinel-256.png`
- `resources/branding/app-icon/windows/sentinel-32.png`
- `resources/branding/app-icon/windows/sentinel-48.png`
- `resources/branding/app-icon/windows/sentinel-64.png`
- `resources/branding/app-icon/windows/sentinel.ico`
- `resources/branding/docs/github-avatar.svg`
- `resources/branding/docs/github-social-preview.svg`
- `resources/branding/docs/readme-header.svg`
- `resources/branding/favicon-16.png`
- `resources/branding/favicon-32.png`
- `resources/branding/favicon.svg`
- `resources/branding/manifest.json`
- `resources/branding/symbol/sentinel-symbol-black.svg`
- `resources/branding/symbol/sentinel-symbol-white.svg`
- `resources/branding/symbol/sentinel-symbol.svg`
- `resources/branding/tools/export.mjs`
- `resources/branding/tools/validate.mjs`
- `resources/branding/tray/linux/sentinel-tray-16-black.png`
- `resources/branding/tray/linux/sentinel-tray-16-white.png`
- `resources/branding/tray/linux/sentinel-tray-22-black.png`
- `resources/branding/tray/linux/sentinel-tray-22-white.png`
- `resources/branding/tray/linux/sentinel-tray-24-black.png`
- `resources/branding/tray/linux/sentinel-tray-24-white.png`
- `resources/branding/tray/linux/sentinel-tray-32-black.png`
- `resources/branding/tray/linux/sentinel-tray-32-white.png`
- `resources/branding/tray/linux/sentinel-tray-symbolic.svg`
- `resources/branding/tray/macos/sentinel-tray-16-template.png`
- `resources/branding/tray/macos/sentinel-tray-16-template@2x.png`
- `resources/branding/tray/macos/sentinel-tray-18-template.png`
- `resources/branding/tray/macos/sentinel-tray-18-template@2x.png`
- `resources/branding/tray/macos/sentinel-tray-20-template.png`
- `resources/branding/tray/macos/sentinel-tray-20-template@2x.png`
- `resources/branding/tray/macos/sentinel-tray-22-template.png`
- `resources/branding/tray/macos/sentinel-tray-22-template@2x.png`
- `resources/branding/tray/sentinel-tray.svg`
- `resources/branding/tray/windows/sentinel-tray-16-black.png`
- `resources/branding/tray/windows/sentinel-tray-16-white.png`
- `resources/branding/tray/windows/sentinel-tray-20-black.png`
- `resources/branding/tray/windows/sentinel-tray-20-white.png`
- `resources/branding/tray/windows/sentinel-tray-24-black.png`
- `resources/branding/tray/windows/sentinel-tray-24-white.png`
- `resources/branding/tray/windows/sentinel-tray-32-black.png`
- `resources/branding/tray/windows/sentinel-tray-32-white.png`
- `resources/branding/validation-report.json`
- `resources/branding/wordmark/sentinel-lockup.svg`
- `docs/reference/BRANDING.md`
- `docs/development/BRAND_ASSET_INTEGRATION_REPORT.md`

Modified existing files (branding edits only; two overlap Phase 3 work):
- `README.md`
- `apps/sentinel-desktop/CMakeLists.txt`
- `apps/sentinel-desktop/src/NativeCompanionAdapter.cpp`
- `apps/sentinel-desktop/src/DesktopShellViewModel.cpp`
- `cmake/SentinelCPack.cmake`
- `docs/release/PACKAGING.md`
- `packaging/linux/fedora-kde/sentinel-desktop.spec`
- `packaging/linux/rpm/sentinel-desktop.spec`
- `packaging/macos/bundle/Info.plist.in`
- `packaging/macos/run_macos_tests.sh`
- `ui/qml/onboarding/FinishStep.qml`
- `ui/qml/onboarding/OnboardingScreen.qml`
- `ui/qml/onboarding/WelcomeStep.qml`

Removed:
- `resources/app-icons/dev.sentinel.Sentinel.png`
- `resources/app-icons/dev.sentinel.Sentinel.svg`
- `resources/app-icons/dev.sentinel.Sentinel.icns`
- `resources/app-icons/dev.sentinel.Sentinel.ico`

## Final closure — 2026-10-05

The preceding **FINAL VERDICT: PARTIAL** is retained as the historical integration result. This closure uses the user's explicitly narrowed acceptance scope: native tray behavior and Windows/Linux native runtime certification are deferred and do not block Brand Asset Integration.

### 1. macOS bundle identity

The exact staged install inspected in Finder and launched for closure was `/private/tmp/sentinel-brand-closure-install/sentinel-desktop.app`, produced by `cmake --install build/tests`. The running process path was verified directly. The bundle identifier remains `dev.sentinel.Sentinel`; executable is `Contents/MacOS/sentinel-desktop`; `CFBundleIconFile` is `sentinel.icns`. Its sole ICNS matches `resources/branding/app-icon/macos/sentinel.icns` byte-for-byte, SHA-256 `fc7804aa5725dca5e26d32ee381120e299e9ba3c99cfbd3a37e49b8eb4347f67`.

An unused `dev.sentinel.Sentinel.icns` had survived in the incremental build bundle from the old resource configuration. It was removed from ignored build output before creating the fresh staged install. No old ICNS remains in the inspected install. Production artwork and CMake configuration were not modified for shell caching. No old icon was observed in Finder, and the user reported no old icon in Dock. Build and installed Mach-O `__text` sections match (SHA-256 `2752c035e8a7a8a8b9aef2bf60ccdfecc0a3a0719589b45543e9b1487abd7256`); full file hashes differ because installation modifies Mach-O packaging metadata. No alternate Sentinel Desktop process was used for the closure smoke.

### 2. Real native macOS appearance

- Finder: **PASS — agent-observed native rendering**, showing Porcelain FA-3 on the dark Quiet Field icon at the exact staged install location.
- Dock: **PASS — user-observed native rendering**. While the verified installed process was running, the user answered: “Quiet Field: white FA-3 on a dark field.” The computer-use tool repeatedly timed out on Dock itself; this result is explicitly human-observed, not an agent screenshot or an inference from ICNS contents.
- App switcher: **NOT VALIDATED**; optional check not captured.
- About/window identity: no dedicated About branding page exists. The running app title remains Sentinel Desktop Alpha; the new onboarding header and wordmark were observed with no obsolete S placeholder. Native About/icon surface is not separately certified.

### 3. Onboarding focus and blocking navigation

`introduced_by_brand_integration: no`.

The Tab focus leak reproduced in the installed app: focus can enter the chat composer underneath onboarding. Comparison with `HEAD`'s pre-branding `OnboardingScreen.qml` confirms the same root `Item`, visibility/enabled logic, absence of modal focus containment, and unchanged Next/Back handlers. Brand edits replaced non-focusable artwork only. Classification: **pre-existing/unrelated product defect**, `user_visible_severity: non-blocking` for the remaining focus leak after the navigation fix below. The focus containment defect is documented for a later UI task; no redesign applied.

Separately, Next initially remained on Welcome even with the isolated daemon connected. The pre-branding Phase 3 `DesktopSettingsStore` omitted `onboardingFlowJson` from its local presentation keys. `OnboardingService::save` therefore routed progress through runtime settings, which does not project this UI progress key. This was an unrelated, **blocking** persistence defect. The closure request explicitly permits addressing unrelated issues that block normal onboarding use.

Minimal fix: added only `onboardingFlowJson` to the local presentation-settings key set. Provider/model authority remains daemon-owned; no IPC contract or runtime setting authority changed. Added `DesktopIpcTest::onboardingProgressPersistsWithoutDaemon`, which verifies Welcome → ProcessingMode, persisted reopening, and Back without a connected daemon. Real installed UI smoke now advances Next to Step 2 and Back to Step 1. No focus containment patch was necessary for normal button navigation.

### 4. Tray / 5. Windows / 6. Linux

Tray production families remain present and unchanged. CMake selects the macOS template @2x, Windows black raster, and Linux black fallback; existing companion and notification fallback references use `:/branding/tray.png`. No old app artwork remains selected as a tray fallback. Generic platform notification theme lookup remains existing behavior.

Native tray appearance: **NOT VALIDATED — deferred to Quick Panel / Native Desktop Integration phase**.

Windows: **STATIC INTEGRATION PASS / NATIVE VALIDATION DEFERRED**.

Linux: **STATIC INTEGRATION PASS / NATIVE VALIDATION DEFERRED**.

These deferred checks are not blockers under the closure acceptance criteria.

### 7. Warning/toolchain classification

Comparisons used the same installed tools, repository `.clang-format`, QML import paths, and configured `.clang-tidy`. NativeCompanionAdapter and the three changed onboarding QML files were compared with their `HEAD` versions. DesktopShellViewModel's formatting baseline was reconstructed by reversing only the known branding tray edits, preserving the pre-existing Phase 3 changes; comparing against HEAD alone would incorrectly attribute Phase 3 changes to branding.

| Check | Pre-branding baseline | Integration before closure | Final current | Delta/classification |
| --- | --- | --- | --- | --- |
| NativeCompanionAdapter format | 8 formatting findings | 8 | 8 | 0; pre-existing formatting debt |
| DesktopShellViewModel format | 0 findings | 3 | 0 | Branding shortened the tray path, changing line-wrap requirements; focused correction applied; final delta 0 |
| DesktopSettingsStore format | 0 | 0 | 0 | New local key introduces no formatting finding |
| Changed onboarding QML lint | 27 warnings, 0 errors | 26 warnings, 0 errors | 26 warnings, 0 errors | -1 unqualified warning; no new category or error; remaining layout/unqualified warnings are baseline debt |
| Configured clang-tidy on NativeCompanionAdapter | Fails on incompatible Apple PCH | Same failure | Same failure | Host LLVM/Apple toolchain incompatibility; no source diagnostic comparison possible beyond this identical compiler failure; NOT a tidy PASS |

The original report incorrectly grouped the three DesktopShell formatting findings as existing debt. Closure identified and fixed that branding delta rather than masking it. No unrelated warning cleanup was performed. Prior PCH-free attempts also encountered SDK/libc++ headers; no toolchain installation or configuration redesign was attempted.

### 8. Brand integrity

Rechecked integrated repository files directly: **PASS** for exact FA-3 path, flat Obsidian/Porcelain SVG composition without retired plane, manifest hashes/byte counts, 12 SVG XML parses, 50 PNG CRCs/dimensions/decoded pixels, monochrome tray/favicons, eight ICO PNG entries, and 11 ICNS PNG chunks. All 69 branding files remain unchanged. `git diff --check` PASS. Status stays **PRODUCTION CANDIDATE — PROVISIONAL LOCK**; no legal/trademark clearance claimed.

### 9. Regression

Full integration baseline remains **111/111 CTest PASS**, ApplicationController **127/0/0**, DesktopShell **74/0/0**, Desktop IPC **26/0/0**, Rust fmt/clippy PASS and **17 tests PASS**. Closure rebuilt affected targets and reran Controller, Shell, GUI components, and Desktop IPC: **4/4 PASS**. Controller remains **127/0/0**, Shell **74/0/0**, and IPC is now **27/0/0** due to the one added meaningful onboarding persistence test. No CTest executable count changed. Rust was not rerun because closure does not change Rust or the contract. No unrelated expensive matrix rerun.

### 10. Hygiene / 11. Commit readiness

Closure source changes: scoped tray-call formatting in `apps/sentinel-desktop/src/DesktopShellViewModel.cpp`; one local presentation key in `apps/sentinel-desktop/include/sentinel/desktop/DesktopSettingsStore.h`; focused regression in `tests/desktop/test_desktop_ipc.cpp`; this report update. Existing Phase 3 edits remain intact. No production brand asset or source build configuration changed during closure.

Screenshots were inspected through the computer-use tool and not added to the repository. Temporary staged install, profiles, socket directory, comparison sources, and logs stayed under the OS temporary directory. Generated old ICNS was removed from the incremental build bundle. `git status --short`, `git diff --stat`, and `git diff --check` reviewed. No ZIP, cache, credentials, screenshots, or unrelated files added as source. No commit/push performed.

Current-host acceptance is satisfied: actual Finder/Dock identity confirmed, no remaining branding warning regression, asset/build integrity preserved. The unrelated focus leak and explicitly deferred native tray/platform checks are documented and do not constitute brand regressions.

**FINAL CLOSURE VERDICT: FIXED + PASS**
