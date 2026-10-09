# Sentinel UI/UX visual audit

2026-10-09 · baseline `102e602` + pre-existing local rail changes. Read-only application audit. [Inventory](UI_UX_SCREEN_INVENTORY.md) · [Roadmap](UI_UX_IMPROVEMENT_ROADMAP.md) · [Screenshots](../reviews/ui-ux-audit-2026-10-09/README.md).

## Evidence and limits

The debug desktop and daemon targets built successfully (`cmake --build --preset debug --target sentinel-desktop sentinel-daemon`: no work required). The existing tests-preset certification target also built successfully (`cmake --build --preset tests --target settings_ui_certification_probe`: no work required). No dependencies were installed, configuration changed, or production code edited. Full CTest was not rerun for these documentation-only changes; previous test results are not represented as this audit's visual acceptance.

Actual native Qt/QML was launched using `settings_ui_certification_probe <disposable-directory>`. It loads production QML and real viewmodels with separate disposable SQLite/JSON stores and an in-memory credential store. It is **not** Main.qml or a live authoritative daemon session. Its catalog and Quick Panel objects are fixtures. Fixture readiness text is not evidence of provider health. The four rectangular controls at the top (Settings / Onboarding / Chat / 780 px) and the certification window title belong to the test wrapper: **exclude them from product styling/branding findings**. No mockups, fabricated messages, private conversations, credentials or sensitive file paths appear in the new JPEGs.

Production startup was not exercised against the user's data: explicit preferences/socket options do not by themselves isolate the global single-instance guard and early storage migration. The existing harness provided safe rendered evidence without closing/reconfiguring the user's running app. Interactive wrapper navigation exposes Settings, Chat and Onboarding, but not production Models/Inspector/approval/command palette. This is a concrete coverage limitation, not a claim that those product screens do not exist.

The first sandboxed GUI launch aborted with pasteboard/HiServices connection errors. Launch outside that sandbox succeeded; this is an audit-environment restriction, **not a reproduced Sentinel daemon timeout**. During the successful session Qt logged a missing `Inter, Sans-serif` family and alias lookup cost (~67 ms). Missing local STT produced `ModelUnavailable: STT runtime is not ready`. No real model timeout, IPC authorization failure, conversation deletion failure, or successful inference was reproduced in this audit.

Severity: High = blocks a meaningful flow or obscures consequential state; Medium = material readability/discoverability/consistency issue; Low = minor polish. CONFIRMED includes directly observed rendering or a definitive current source connection defect. SUSPECTED requires live reproduction. NOT VALIDATED records a required acceptance check, not a proven defect.

## Issue register

### UI-001 — Notification history action has no mounted panel

- **Screen:** S22 / notification history. **Severity:** High. **Status:** CONFIRMED.
- **Sources:** `ui/qml/pages/settings/SystemSettingsTab.qml:362`, `ui/qml/components/notifications/NotificationCenterPanel.qml`, `ui/qml/Main.qml`, `apps/sentinel-desktop/src/DesktopShellViewModel.cpp:7113`.
- **Observed:** Open history sets `notificationCenterVisible`; no production QML instance consumes it. Clicking the action in the harness left the AX tree unchanged. Panel/toast component definitions and `qmldir` registrations do not mount them.
- **Evidence:** screenshot 14 shows the action; repository search found panel/toast names only in their registration/definition, not an instantiation. The harness observation alone would not prove a production defect; the source wiring establishes it.
- **Proposed improvement:** after approval, connect the existing panel to the shell property and preserve the existing notification service/history authority. Review toast mounting separately.
- **Acceptance:** Open history displays existing/empty history from every relevant entry; Escape/outside dismissal and focus return work; DND retains history; read/archive updates reflect real service state. Test production Main, not only a wrapper.

### UI-002 — Mixed raw and themed controls in light Settings

- **Screen:** S16, S19. **Severity:** Medium. **Status:** CONFIRMED.
- **Sources:** `ui/qml/pages/settings/SystemSettingsTab.qml:275`, `VoiceSettingsTab.qml:126`, `VoiceSettingsTab.qml:138`; shared `components/base/SentinelButton.qml`, `SentinelComboBox.qml`.
- **Observed:** Quick Panel shortcut selector is a dark rectangular raw ComboBox in a white card, while adjacent themed selectors have pale rounded surfaces. Voice test/detect buttons likewise render as dark rectangles beside rounded Sentinel controls.
- **Evidence:** screenshots 08 and 11. Wrapper toolbar buttons are deliberately excluded.
- **Proposed improvement:** use the established themed controls or shared palette/style treatment while preserving bindings and behavior.
- **Acceptance:** light/dark normal, hover, focus, disabled and popup states use consistent tokens; long labels wrap/elide safely; no change to settings semantics or native file dialogs.

### UI-003 — Main navigation icons lose contrast in dark capture

- **Screen:** S01 rail component, S17/S04 context. **Severity:** High. **Status:** CONFIRMED rendering; cause SUSPECTED.
- **Sources:** `ui/qml/components/navigation/NavigationRail.qml`, `components/base/SentinelIcon.qml`, `theme/SentinelTheme.qml`.
- **Observed:** Home/Models strokes appear black on the dark rail in screenshots 20–22; Inspector/Settings use a different muted appearance. Icons that are primary navigation become hard to distinguish.
- **Evidence:** screenshots 20 and 22. Source uses SVG images plus a colorization effect conditional on graphics API. The active graphics path/effect failure was not instrumented, so do not attribute this conclusively to software rendering.
- **Proposed improvement:** first reproduce in production Metal, Linux OpenGL and supported software fallback; then fix shared icon tint/state rendering.
- **Acceptance:** all rail icons, including selected/disabled/focused states, meet non-text contrast targets in light/dark/high contrast on each supported renderer; tooltip and accessible names remain intact.

### UI-004 — Settings labels are truncated at a regular window width

- **Screen:** S16–S23. **Severity:** Medium. **Status:** CONFIRMED.
- **Sources:** `ui/qml/pages/settings/SettingsPage.qml` (sidebar preferred width, ItemDelegate label/elision, compact breakpoint).
- **Observed:** at 1100×780 logical window size, Workspace & Memory and Privacy & Permissions end in ellipses. Icons and text do not collide in these captures; that does not certify every resize transition or language.
- **Evidence:** screenshots 08, 09, 11, 14. At 780×640, category selection correctly replaces the detailed sidebar (19/20).
- **Proposed improvement:** retain the dual sidebar structure; adjust bounded category widths, accessible full-name exposure/tooltips or wrapping. Review the 760/820 width conditions together.
- **Acceptance:** all eight category names are discoverable without guesswork at supported widths/densities, with long German/Turkish and Arabic; no icon/text overlap during resize; all categories keyboard-reachable.

### UI-005 — Finish copy implies readiness without a runnable provider/model

- **Screen:** S39. **Severity:** Medium. **Status:** CONFIRMED.
- **Sources:** `ui/qml/components/onboarding/FinishStep.qml`, `ModelSetupStep.qml`, `ProviderSetupStep.qml`.
- **Observed:** “You're ready!” / “Your assistant is ready when you are” appears while summary says Provider and Model “Set up later”. Completing preferences is different from inference readiness.
- **Evidence:** 07; preceding model-empty step 04. Fixture provider probe text is not used to claim a connectivity failure.
- **Proposed improvement:** separate setup completion from model readiness; offer the existing setup route when unconfigured.
- **Acceptance:** no-model/offline states explicitly explain that sending requires configuration; configured state uses actual readiness; finishing never starts an install or grants permission automatically.

### UI-006 — Voice failure exposes an internal code without a repair route

- **Screen:** S04. **Severity:** Medium. **Status:** CONFIRMED.
- **Sources:** `ui/qml/components/chat/HomeChatSurface.qml` voice button/error area, `ui/qml/pages/settings/VoiceSettingsTab.qml`, `apps/sentinel-desktop/src/SystemDictationService.cpp`.
- **Observed:** clicking Voice with no STT runtime shows `ModelUnavailable: STT runtime is not ready` in the composer and tooltip. The missing engine is an expected runtime prerequisite; the raw code and lack of an adjacent setup action are UX issues.
- **Evidence:** screenshot 22 and AX error text. No microphone permission was accepted or recording created.
- **Proposed improvement:** user-facing localized error plus an existing Voice & Audio setup route; keep technical details available in diagnostics. Do not pretend recognition succeeded.
- **Acceptance:** missing model, permission denied, unavailable device and recognition failure have distinct messages/actions; local Whisper and supported system dictation complete real mic→transcript flows; no automatic sending of the transcript.

### UI-007 — Connection light depends on hover and color

- **Screen:** S01. **Severity:** Medium. **Status:** CONFIRMED source limitation.
- **Sources:** `ui/qml/components/navigation/NavigationRail.qml:67`, `Main.qml`; compare focusable `providerStatusLight` in `pages/settings/ModelSettingsTab.qml`.
- **Observed:** the rail's status is a non-focusable Rectangle with hover tooltip and accessible name; there is no visible non-color label or keyboard action to inspect/retry. In the wrapper its default red does not represent a live connection and must not be read as a daemon failure.
- **Evidence:** source and rail in 08/20. Provider-status AX name does expose “Runtime status: disabled”, showing a useful existing pattern.
- **Proposed improvement:** retain the compact traffic-light design with keyboard-accessible explanation/status details and a non-color cue.
- **Acceptance:** keyboard and screen-reader users can determine connecting/disconnected/ready without color or pointer hover; actual daemon health is distinguished from model/provider readiness.

### UI-008 — Two model paging levels need validation; build guide is stale

- **Screen:** S12 and developer launch guidance. **Severity:** Medium. **Status:** CONFIRMED documentation mismatch; SUSPECTED usability issue.
- **Sources:** `ui/qml/pages/models/ModelsPage.qml`, `ModelCatalog.js`, `apps/sentinel-desktop/src/DesktopModelHelper.cpp`, `docs/development/BUILDING.md:26`.
- **Observed:** source provides 40-card pages plus separate Hugging Face catalog batches. BUILDING still says continuation is automatic. No fresh rendered Models evidence was obtained; do not infer that the old 40-count/timeout regression persists.
- **Evidence:** current source button labels and pagination logic versus build guide. User's earlier timeout image is historical context, not a fresh reproduced result.
- **Proposed improvement:** clarify loaded versus discoverable counts and distinct page/batch operations; align documentation after approved implementation review.
- **Acceptance:** with >40 families, filters/sort/page changes behave predictably; slow/offline sources cannot block chat/delete IPC; pending, partial success, source failure and retry preserve useful data; counts never imply the entire upstream corpus has been loaded.

### UI-009 — Translation catalogs remain materially unfinished

- **Screen:** all translated surfaces. **Severity:** Medium. **Status:** CONFIRMED catalog completeness; language quality NOT VALIDATED.
- **Sources:** `translations/sentinel_{en,tr,ar,de,es,fr,ja,zh}.ts`, `ui/qml/pages/settings/SettingsPage.qml`.
- **Observed/evidence:** measured active TS entries below. Non-English fallbacks will occur. TS flags do not measure correctness, runtime string coverage or English usability; English source fallback is intentional.
- **Proposed improvement:** complete high-traffic/error/security strings first, then native review, overflow and RTL verification. Preserve eight supported languages until evidence supports a different scope.
- **Acceptance:** no unfinished high-traffic or approval/error strings; placeholder/plural integrity checked; real RTL navigation/text entry and long-label layouts captured; language reviewer signs off on meaning.

| Catalog | Active messages | Finished, non-empty | Ratio |
| --- | ---: | ---: | ---: |
| English | 1213 | 598 | 49.3% |
| Turkish | 1213 | 644 | 53.1% |
| Arabic | 1213 | 478 | 39.4% |
| German | 1213 | 478 | 39.4% |
| Spanish | 1213 | 478 | 39.4% |
| French | 1213 | 478 | 39.4% |
| Japanese | 1213 | 478 | 39.4% |
| Chinese | 1213 | 478 | 39.4% |

Method: exclude `obsolete`/`vanished`; count a translation complete only when not `unfinished` and its text (including plural children) is non-empty. Plural completeness and semantic equivalence were not individually certified. Settings mirrors Arabic in source; that is not proof of global RTL.

### UI-010 — Approval readability and accessible modal behavior need live evidence

- **Screen:** S32 / S27. **Severity:** High acceptance priority. **Status:** NOT VALIDATED.
- **Sources:** `ui/qml/Main.qml:421`, `components/dialogs/SentinelOverlayModal.qml`, `pages/settings/SecuritySettingsTab.qml`.
- **Observed:** source defaults focus to Deny and keeps explicit Cancel/Deny/Allow once; resource/domain/access and risk are formatted largely as raw values. Long real resource lists have not been rendered in the fixed preferred height.
- **Evidence:** source only; no pending approval screenshot. No security defect or unintended grant is asserted.
- **Proposed improvement:** measure long/translated approvals before changing presentation; retain explicit scope and execution boundaries, expose readable field labels where needed.
- **Acceptance:** no clipping of scope/tool/risk; scroll and keyboard focus stay in modal; Deny is initial safe focus; allow-once affects only the pending request; completion is reported only after authoritative AgentLoop acceptance.

### UI-011 — Inspector density and error states lack rendered acceptance

- **Screen:** S11. **Severity:** Medium. **Status:** NOT VALIDATED.
- **Sources:** `ui/qml/pages/AgentInspectorPage.qml`, `apps/sentinel-desktop/src/viewmodels/AgentInspectorViewModel.cpp`, `src/RemoteAgentInspectorService.cpp`.
- **Observed:** source has run filters, recent runs/load more, selected details, parent/child runs, timeline, context/grounding diagnostics. Readability with long IDs, tool payloads, empty/error states and small windows was not observed.
- **Evidence:** source inventory only. It would be incorrect to mark this view visually acceptable or defective from its existence.
- **Proposed improvement:** first gather safe empty/loading/error/populated traces; then unify typography/spacing and disclosure with existing shared cards.
- **Acceptance:** diagnostic content wraps or scrolls without overlap at small/large widths; failed fetch has retry; filtering/selecting remains keyboard-accessible; IDs and technical state are copyable and accurately labelled.

### UI-012 — Voice/settings vertical rhythm consumes avoidable space

- **Screen:** S19 and long Settings categories. **Severity:** Low. **Status:** CONFIRMED in Voice; broader scope SUSPECTED.
- **Sources:** `ui/qml/pages/settings/VoiceSettingsTab.qml`, `SettingsPage.qml`, `components/base/SettingCard.qml`, `SettingControlRow.qml`.
- **Observed:** the microphone-test card reserves a large blank area after its two actions, while lower engine/path controls require scrolling. Heading/subheading/card hierarchy is repeated. Long sections and varied trailing-control widths are source-visible but not all rendered.
- **Evidence:** screenshot 11; category widths in 08/14/19. The spacious empty-chat greeting is intentional and is not classified as the same defect.
- **Proposed improvement:** adjust shared row/card sizing around actual content and status visibility; keep the eight-category structure and existing light/dark identities.
- **Acceptance:** no blank reserved status area when absent unless needed to avoid measured layout shift; consistent trailing-control bounds; path/error text wraps; essential setup controls remain discoverable at 780×640.

### UI-013 — Backup/recovery flows exist but lack end-to-end visual proof

- **Screen:** S23/S29–S31. **Severity:** High acceptance priority. **Status:** NOT VALIDATED.
- **Sources:** `ui/qml/pages/settings/SystemSettingsTab.qml`, `apps/sentinel-desktop/src/DesktopBackupHelper.cpp`, `core/include/sentinel/core/app/BackupService.h`, `RecoveryService.h`, `ui/qml/Main.qml`.
- **Observed:** actual UI includes domain switches, file inspection, merge/replace review and explicit restore; disabled export is correctly observable with no daemon. This supersedes an assumption that only backend contracts exist. Success/conflict/cancel/corrupt-file/restart recovery have not been exercised here.
- **Evidence:** 15/19 show entry controls; helper validates bounded local JSON and manifest. A disabled export in this harness is not a production defect.
- **Proposed improvement:** certify real isolated daemon round trips before redesign. Preserve credential/model exclusions and explicit overwrite review.
- **Acceptance:** exported selected data reimports into a separate profile; malformed/oversized files fail safely; merge conflicts/replace scope are visible; cancellation is truthful; interrupted work never executes automatically.

### UI-014 — Accessibility certification remains incomplete

- **Screen:** all, especially S01/S17/S32. **Severity:** High acceptance priority. **Status:** NOT VALIDATED.
- **Sources:** `ui/qml/theme/{SentinelTheme,MotionTokens,InteractionTokens}.qml`, `components/base/`, `pages/settings/SettingsPage.qml`.
- **Observed:** category keyboard selection (Home/Down/Return), theme popup keyboard selection and Tab→composer worked in the harness; visible focus rings appear in 09/20/22. Some theme preset buttons and history actions appear unnamed as parent AX nodes. This is partial inspection, not a screen-reader pass.
- **Evidence:** real AX observations and screenshots; no numeric WCAG contrast measurements or VoiceOver/Orca/NVDA runs performed.
- **Proposed improvement:** measure actual foreground/background pairs and focus indicators; test reduced motion/high contrast across animated components and native popups, plus accessible names/order/announcements.
- **Acceptance:** WCAG AA text/non-text contrast targets verified with measured colors; no color-only status; reduced motion removes nonessential motion; controls announce name/role/state; complete primary flows with keyboard and target-platform screen readers.

### UI-015 — Command palette promises search and category routing it does not perform

- **Screen:** S42. **Severity:** Medium. **Status:** CONFIRMED source behavior; visual state NOT VALIDATED.
- **Sources:** `ui/qml/components/navigation/CommandPalette.qml` (`actions`, `filteredActions`, `runAction`), `ui/qml/Main.qml` palette callbacks.
- **Observed:** Search Chats/Universal Search change status copy, while filtering searches only command titles/subtitles/kinds. Workspace, Notifications and Switch Model all use the same generic settings action without a category payload. Open Settings still describes floating preferences, although Settings is a full page. Theme cycling uses six legacy theme keys and exposes their names rather than the 14 display names shown in Appearance.
- **Evidence:** source inspection; no palette screenshot. These are concrete advertised/action mismatches, not a claim of a failed daemon search endpoint.
- **Proposed improvement:** accurately describe supported command filtering and route existing category actions explicitly; align theme display names and cycle scope with the shared presentation model. Do not add a search backend without separate scope approval.
- **Acceptance:** every command label matches its result; search either returns the advertised entities or clearly describes command-only filtering; category shortcuts land in the intended existing section; theme feedback uses current user-facing names.

## Cross-area assessment

| Area | What the evidence supports | Remaining validation |
| --- | --- | --- |
| Home/chat | Empty greeting and bottom composer; left/right grouped controls; compact provider elision; error remains visible | Real user-right/assistant-left messages, streaming/stop/retry, attachment interpretation, archive/delete, Agent completion |
| Navigation | Fixed narrow rail + contextual sidebar preserved; compact Settings selector works | Production resize/focus transitions, dark icon tint, status action and long-language labels |
| Models | Source groups variants, separates card pages/discovery batches and has source errors | Fresh loading/timeout/retry, metadata/download size truth, installability, operations/activation and source isolation |
| Providers | Provider-adjacent runtime status has an AX name; no model selected is explicit | Actual daemon↔provider state consistency; no-fixture readiness; endpoint failure differentiation |
| Settings | Full-page eight-category implementation and readable heading/card hierarchy | Lower sections and advanced controls, consistent widths/raw controls, locale/density/accessibility matrix |
| Profile/workspace/memory | Visible behavioral instructions exist; source explicitly separates permissions/model and scheduled tasks | Real instruction effect and scope, workspace switching, embedding/knowledge failures and persistence |
| Permissions/tasks | Existing controlled-task workflow and safe Deny focus in source | Long resource lists, real review→approval→run trace; no execution or authorization changes tested |
| Notifications | Delivery/channel/event controls visible | History mounting defect; toast/OS delivery and DND/read/archive behavior |
| Tray | Minimal icon actions/selectors and outside-focus handling implemented in source | Actual menu bar activation, dismissal, transient selector popups and same-history persistence |
| Onboarding | Seven real steps rendered; FA-3 application identity intact | Dark/small wizard, truthful setup completion and readiness consistency |
| Updates/about/licenses | System version/platform/update workflow exists | Modal states, signed package identity, license discoverability; no separate About/license UI |
| Terminal/platforms | Rust CLI/Ratatui and cross-platform packaging sources exist | Real terminal sizing/key handling and installed macOS/Windows/Fedora acceptance |

## Reuse map and change leverage

| Existing component/token | Current use and inconsistency | Preferred future scope |
| --- | --- | --- |
| `SentinelTheme.qml` | Palette, typography, radii, density, accessibility; some direct colors/fonts remain | Preserve light/dark families; measure semantic colors, avoid per-screen replacement palettes |
| `MotionTokens.qml`, `InteractionTokens.qml` | Motion and hover/focus/pressed contracts | Verify all animations/focusable controls consume them; shared fixes before page-specific effects |
| SentinelButton / ComboBox / Slider / SpinBox / Switch | Shared themed controls alongside raw Button/ComboBox/Slider | Standardize product controls while preserving native dialogs and behavior |
| SettingCard / SettingControlRow / SettingToggleRow / SectionTitle | Repeated setting hierarchy and trailing controls | Fix minimum sizes/wrap/control width centrally before editing eight pages |
| SentinelIcon / TablerGlyph / raw SVG Image | Multiple icon paths; dark rail tint problem | Common tint/fallback/focus behavior; preserve FA-3 brand assets separately |
| EmptyState / ErrorBanner / ShimmerEffect / StatusChip / RuntimeBadge | Existing state vocabulary; some screens use inline raw codes/custom busy UI | Consistent loading/error/empty/retry patterns without hiding backend errors |
| SentinelOverlayModal | Recovery/model/runtime/approval overlays | Shared bounds, scroll, focus and dismissal; security-specific no-auto-close remains deliberate |
| HomeChatSurface / legacy ChatPanel | Duplicate conversation presentation source; only HomeChatSurface currently mounted | Improve mounted shared chat components first; do not rewrite both as independent pages |
| Model cards + ModelDetailPopup | Repeated actions/metadata; grouped variants and paging already implemented | Shared operation/readiness/status treatment after real metadata/download evidence |
| NotificationCenterPanel / Toast | Existing reusable views are unmounted | Connect existing implementation after approval rather than inventing a new notification design |

The official FA-3 logo remains the application identity. Core Warden remains separate Agent/Voice mascot work; no mascot should replace application, tray or installer branding. Native Qt/QML can implement the proposed fixes with existing components and platform adapters; no web/frontend/Figma migration is warranted by this evidence.


## Phase 1 implementation addendum — 2026-10-09

Original findings and evidence above are preserved. UI-001–008, UI-012 and UI-015 received scoped remediation or documentation corrections. Detailed per-ID status, source changes, validation boundaries and evidence are in [Phase 1 report](UI_UX_REMEDIATION_PHASE1_REPORT.md). Verdict: **FIXED — PARTIAL VALIDATION**. UI-009, UI-010, UI-011, UI-013 and UI-014 remain follow-ups. Production Main, live inference, hardware voice, OS banners, Linux/Windows and full accessibility are not accepted by harness evidence. No automatic Phase 2 work is authorized by this addendum.
