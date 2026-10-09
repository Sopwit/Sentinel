# SENTINEL — NATIVE UI/UX REMEDIATION PHASE 1

Date: 2026-10-09. Verdict: **FIXED — PARTIAL VALIDATION**.

## Baseline and boundaries

Baseline: `102e602` plus existing uncommitted provider, controller, test, navigation-rail and certification-harness changes. These were retained; the baseline patch was saved outside the repository. Personal-Brain CLI was unavailable. Repository architecture, runtime, security, build and testing instructions were consulted. No commit, push, merge, model installation, permission expansion or personal-history operation was performed.

The existing native C++20/Qt6/QML architecture, 56px rail, contextual sidebars, eight-category full-page Settings, theme families and FA-3 logo remain. API and provider authority contracts were not changed by this UI package.

## Isolation and production-root verification

**NOT VALIDATED:** independent production Main launch. Bootstrap storage migration runs before preference-directory parsing. Single-instance locking and `SentinelDesktopIPC` identity are fixed; preferences/socket overrides alone do not prove isolation. The user application was therefore not restarted or controlled.

**INTEGRATION / NATIVE OBSERVATION:** certification application with disposable JSON/SQLite stores, test application identity and in-memory credentials; production QML and desktop viewmodel exercised without production bootstrap/single-instance transport. Fixtures never establish live daemon or inference success.

Read-only process inspection found desktop started at 18:20:48, while daemon started at 17:12:00, before the daemon executable's observed 17:50:02 modification. Rebuilding/restarting the desktop had left an older daemon process running. The user daemon was not stopped. Provider acceptance must be repeated after a deliberate daemon restart; this finding is not proof of successful live inference.

## Issue results

| ID | Root cause and implemented remedy | Sources | Evidence and limits | Status |
|---|---|---|---|---|
| UI-001 | Existing history panel was not mounted. Mount existing history as a focus-managed Popup; Settings/palette entry points, Escape/outside dismissal, focus restoration, mark-read/archive and DND persistence retained. Archive no longer advertises unsupported unarchive. | Main.qml; NotificationCenterPanel.qml; NotificationItemDelegate.qml; SystemSettingsTab.qml | INTEGRATION lifecycle, actual row actions and focus; NATIVE OBSERVATION synthetic empty/populated/archive/DND. Main wiring compiled, not live accepted. | PARTIAL |
| UI-002 | General shortcut selector and Voice actions used raw controls. Reuse SentinelComboBox/SentinelButton with original bindings and handlers. | SystemSettingsTab.qml; VoiceSettingsTab.qml | NATIVE OBSERVATION light/dark and selected popup; INTEGRATION keyboard behavior. Not every control/state/language exhaustively certified. | FIXED |
| UI-003 | SVG sources mixed currentColor and explicit strokes; fallback bypassed shader tint, leaving inconsistent colors. Route shared icons through Qt Controls icon tint instead of renderer-conditional MultiEffect. | SentinelIcon.qml; TablerGlyph.qml; SentinelButton.qml; NavigationRail.qml | INTEGRATION rendered stroke samples ≥3:1 and ≥10 visible pixels in light/dark/high contrast, Metal and software. Exact GPU shader internals not instrumented; Linux/Windows NOT VALIDATED; not full WCAG. | PARTIAL |
| UI-004 | Sidebar widths and different compact thresholds clipped category names. Unify threshold at 820, widen regular sidebar and wrap labels/height. | SettingsPage.qml | INTEGRATION 1100/900/780 widths; NATIVE OBSERVATION regular/780×640. All translated text/RTL NOT VALIDATED. | FIXED |
| UI-012 | Shared row sizing and empty Voice status reserved space; unwrapped actions risked overflow. Bound control width, wrap card titles/actions and hide genuinely empty status. | SettingCard.qml; SettingControlRow.qml; VoiceSettingsTab.qml | INTEGRATION and NATIVE OBSERVATION regular/compact. Other application screens not redesigned. | FIXED |
| UI-005 | Finish conflated saved preferences with inference readiness. Separate daemon/provider/model/runnable/ready states; scroll content and offer existing Models configuration route. | FinishStep.qml; OnboardingScreen.qml; Main.qml | INTEGRATION Ready/Busy/Missing/Unavailable/Unknown/offline matrix; NATIVE OBSERVATION disconnected finish. Live-ready provider NOT VALIDATED. | PARTIAL |
| UI-006 | Raw STT identifiers reached presentation. Add translatable voiceInputMessage mapping for engine/model, permission, device and recognition failures; preserve raw diagnostics and offer Voice settings. | DesktopShellViewModel.h/.cpp; HomeChatSurface.qml; DashboardPage.qml; VoiceSettingsTab.qml; Main.qml | UNIT failure mappings; INTEGRATION and NATIVE OBSERVATION actual missing-engine failure and route. No microphone/STT success, OS permissions or device-failure hardware acceptance. New strings use translation API; catalog completion remains pending. | PARTIAL |
| UI-007 | Color/hover status was insufficient. Keyboard-focusable detail Popup separates daemon connection from active provider/model summary and restores focus. | NavigationRail.qml; Main.qml | INTEGRATION Space/Escape/focus; NATIVE OBSERVATION fixture details. Screen-reader and live state acceptance NOT VALIDATED. | FIXED |
| UI-015 | Palette advertised missing search and fake-success behavior; categories/theme subset were inconsistent. Disable unavailable searches, route existing Settings categories/history, cycle all presets and share theme display names using existing translation context. | CommandPalette.qml; SentinelTheme.qml; AppearanceSettingsTab.qml; Main.qml | INTEGRATION category/history/theme actions; NATIVE OBSERVATION disabled search. Production Main routing source/build verified, not live E2E. | FIXED |
| UI-008 | Build guide described automatic HF continuation although current UI has distinct local pages and explicit remote batches. Correct guide. | BUILDING.md | Source/document review: 40-family pages apply to loaded filtered cards; remote HF previous/next batches are separate, counts/sort refer to loaded data. No newly reproduced discovery bug; backend untouched. | FIXED |

Source paths in the table are under `ui/qml` except desktop viewmodel under `apps/sentinel-desktop`, and BUILDING under `docs/development`.

## Notification delivery review

History uses existing service authority and persistence. NotificationToast remains unmounted: it watches broad native-experience updates and historical unread summaries, so mounting it blindly could replay prior notices. NativeCompanionAdapter already owns OS delivery. Toast event semantics and actual OS banners are separate follow-ups, **NOT VALIDATED** here. No OS permission was granted. Inherited dark history-card shadows are visible and remain a visual follow-up.

## Evidence and validation

See [dated evidence and SHA256 manifest](../reviews/ui-ux-phase1-2026-10-09/README.md). Before captures are byte-identical copies of audit images; after captures are actual macOS harness screenshots, never mockups. Intermediate capture 03 is explicitly labelled and is not final visual acceptance.

- Configure: `CCACHE_DISABLE=1 cmake --preset tests` — PASS.
- Build: `CCACHE_DISABLE=1 cmake --build --preset tests -j6` — PASS; debug desktop and daemon build — PASS.
- Final full suite: `QT_QPA_PLATFORM=offscreen CCACHE_DISABLE=1 ctest --preset tests --output-on-failure` outside the restrictive sandbox — **122/122 PASS, 93.65 seconds**. Earlier full suite also passed 122/122 in 97.98 seconds.
- Focused suite: 9/9 PASS before final focus/copy polish.
- Final Metal and software `settings_ui_certification_probe --phase1-smoke` — PASS. Covers notification lifecycle/actions/DND/dismissal/focus, palette routes, category layout, connection keyboard interaction, missing-STT guidance and onboarding fixture matrix.
- QML lint target exits successfully with existing unqualified/context-property warnings; it is not warning-free. QML/C++ build provides type validation; no separate web typechecker applies.
- `git diff --check` — PASS.

Sandbox-only test attempts failed to access local sockets/process/window services; those are environment failures, not accepted product evidence. Final full-suite result must be read with its execution environment. No tests weaken security settings to pass.

## Outstanding scope and Phase 2

UI-009: measure translation coverage, correctness, overflow and RTL across eight languages. UI-010: real Agent approval acceptance. UI-011: Inspector acceptance. UI-013: actual backup/export/import/recovery round-trip. UI-014: full measured WCAG and screen-reader validation. None is silently marked fixed.

Recommended next work: safe production-root isolation first, then actual daemon/provider/model and mounted history acceptance; hardware voice/OS dictation and native notifications; translation/RTL and accessibility; backup recovery and live approval/Inspector flows. Do not begin Phase 2 automatically.

**Final verdict: FIXED — PARTIAL VALIDATION.** Scoped changes are implemented and harness-verified; live production-root, hardware and cross-platform acceptance remain outstanding.
