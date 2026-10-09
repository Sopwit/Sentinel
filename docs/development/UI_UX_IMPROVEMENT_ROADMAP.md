# Sentinel UI/UX improvement roadmap

2026-10-09 · proposal only. No implementation authorized by this audit. [Inventory](UI_UX_SCREEN_INVENTORY.md) · [Issue register](UI_UX_VISUAL_AUDIT.md) · [Evidence](../reviews/ui-ux-audit-2026-10-09/README.md).

## Recommended sequence

| Order | Work package / issue IDs | Reason and scope | Exit evidence |
| --- | --- | --- | --- |
| 0 | Complete critical live evidence — UI-008, 010, 011, 013, 014 | Use isolated production desktop + daemon, separate socket/storage/single-instance identity. Reproduce slow model loading, deletion, approval, Inspector, backup and tray before diagnosing backend failures. Do not change the user's existing profile or waive permission gates. | Safe current screenshots/traces covering loading/empty/error/populated, real mic/STT and native focus. Keep unsupported platforms NOT VALIDATED. |
| 1 | Repair broken visible entry — UI-001 | Mount the existing notification history panel in the actual shell and verify lifecycle/focus; review toast mounting independently. Existing history/service stays authoritative. | Production Open history, empty/populated, DND, read/archive and dismissal tests. |
| 2 | Shared control and icon consistency — UI-002, 003, 007 | Fix themed control adoption and renderer-safe icon tint; add keyboard-readable compact connection explanation. Preserve rail, contextual sidebar, layout and traffic-light intent. | Light/dark/high-contrast screenshots with focus/disabled/hover and supported renderers; no raw dark control islands in light pages. |
| 3 | Responsive Settings and vertical rhythm — UI-004, 012 | Tune shared rows/cards/control widths and category labels. Retain eight categories and full-page Settings. Avoid redesigning each page independently. | 780×640, regular and wide window; every category/control reachable, long text wraps, no resize collisions. |
| 4 | Honest readiness and repair guidance — UI-005, 006, 008, 015 | Distinguish setup completion from runnable inference, voice prerequisites from failures, daemon connection from provider/model readiness, visible pages from discovery batches. Align palette labels/category routes and update stale build guidance. | Real configured/unconfigured/offline scenarios; no invented successful readiness; useful existing setup/retry routes; commands perform their advertised actions. |
| 5 | Safety and diagnostic legibility — UI-010, 011 | Improve approval field labels/long resources and Inspector details only after live traces. Keep Deny initial focus, explicit allow-once, cancel, authoritative completion and tool gateway boundaries. | Real safe approval deny/cancel/allow-once trace; long translated payloads scroll; Inspector list/detail/errors work with keyboard. |
| 6 | Data lifecycle and voice acceptance — UI-013, 006 | Verify existing backup/restore/recovery and real audio flows; fix concrete failures rather than treating them as styling defects. Behavioral response profile already has visible instructions; validate its actual effect/scope. | Export/import isolated round trip; malformed/cancel/conflict/restart checks; mic→transcript and output-device checks; no automatic tool rerun or transcript send. |
| 7 | Translation and accessibility completion — UI-009, 014 | Start high-traffic/error/approval strings early, then complete eight-language review and AA contrast/screen-reader coverage across the stabilized components. | Catalog/placeholder/plural report, native language review, RTL/overflow captures, numeric contrast and VoiceOver/Orca/NVDA primary-flow records. |
| 8 | Installed terminal and platform acceptance | Validate CLI/Ratatui and packages on real Fedora KDE, Windows and macOS hosts after shared UI changes. Retain native adapters, release signing and FA-3 identity. | Install/launch/update/uninstall; tray/Dock/taskbar/notifications/shortcuts; terminal resize/session attach; platform-specific evidence, no inferred PASS. |

Orders are dependencies, not calendar estimates. Backend defects get separate engineering issues after reproduction; they must not be silently fixed by hiding a banner, raising arbitrary timeouts, claiming every download is runnable or suppressing missing metadata.

## Evidence required before broad UI acceptance

| Blocking evidence | Current limitation | Safe next procedure |
| --- | --- | --- |
| Production root isolation | Early storage migration and global single-instance guard mean preferences/socket flags alone are insufficient isolation | Establish an independently isolated test process environment or dedicated test account; confirm paths/lock/socket first. Do not close the user's app or reuse their history for screenshots. |
| Model discovery/error/paging | Interactive certification wrapper cannot navigate to Models; fixtures are not upstream discovery | Production library with >40 families; slow/failed HF, Ollama and LM Studio independently; capture loaded counts/pages/batches, retry and maintained chat responsiveness. |
| Model operations/metadata | No install/download/activation performed | Public, small compatible artifact in disposable model storage; record exact expected bytes/provenance/format and actual verified result. Do not fabricate unavailable publisher information. |
| Real conversation lifecycle | No successful provider response or deletion attempted | Synthetic non-personal conversation, actual provider response, active-run delete guard, archive/unarchive and explicit delete confirmation; verify persisted history and tray continuation. |
| Agent and approval | No model-bound safe Agent run or pending daemon approval | Read-only bounded task in disposable workspace, explicit tool permission review, capture plan/tool/deny/cancel and accepted final answer. No heuristic shell execution. |
| Inspector | Not mounted in interactive harness | Empty/loading/failure/populated production Inspector, parent/child run and long timeline at small/wide sizes. |
| Backup/recovery | Entry controls only; daemon absent from harness | Selected-domain export/import into separate profile, preview scope/conflicts, malformed file, transfer cancel, interrupted run/download/draft; demonstrate no automatic execution. |
| Audio | Missing STT shown; no permission/recording flow | User-authorized test microphone and installed local/system engine; record consent/device/error handling and transcript/result without private audio. |
| Native tray dismissal | Source handles focus loss/outside click, no live evidence | Open panel from tray, click main app/another app, Escape, open selector popup; verify dismissal/ownership and shared conversation persistence. |
| Updates/installation | Development build is not signed package acceptance | Platform release artifact and disposable install environment; capture update states and native identity/license visibility. |
| Accessibility/localization | Partial keyboard/AX inspection; no contrast/reader certification | Numeric semantic color pair measurements and actual target readers, long languages/Arabic RTL, reduced-motion animation checks. |

## Implementation guardrails after approval

Keep the current C++20/Qt6/QML modular monolith and presentation-only QML. Improve existing reusable components; preserve current primary/context navigation, window layout, light/dark theme families and FA-3 official assets. Core Warden is separate Agent/Voice mascot work, not replacement app branding. No React/web frontend, Figma dependency or unrelated backend.

Daemon remains authoritative. Extend the existing `protocol/ipc-v1.json` contracts and generated types only if a proven requirement needs it. Preserve IModelRouter/ModelBinding, IChatProvider, AgentRuntime/accepted AgentLoop completion, ToolExecutionGateway, IFileSystemService and ProcessExecutor boundaries. Keep settings, memory, conversations, runs and grants separate. Workspace context and response-style instructions do not grant permissions.

For approved code changes, run available QML lint and relevant build/tests; broad shared work requires the repository's tests configure/build/CTest sequence. Verify actual primary user flow and loading/empty/error, keyboard focus and responsive states. CTest/source existence alone cannot certify visuals, microphone operation, OS notifications or installer behavior.

## Review decision

Approve individual work packages or adjust their order after reviewing the evidence. **The audit ends with reports and safe screenshots; application changes, new dependencies and commits remain unperformed.** Pre-existing local rail/test modifications are neither reverted nor committed by this task.


## Phase 1 implementation addendum — 2026-10-09

Original findings and evidence above are preserved. UI-001–008, UI-012 and UI-015 received scoped remediation or documentation corrections. Detailed per-ID status, source changes, validation boundaries and evidence are in [Phase 1 report](UI_UX_REMEDIATION_PHASE1_REPORT.md). Verdict: **FIXED — PARTIAL VALIDATION**. UI-009, UI-010, UI-011, UI-013 and UI-014 remain follow-ups. Production Main, live inference, hardware voice, OS banners, Linux/Windows and full accessibility are not accepted by harness evidence. No automatic Phase 2 work is authorized by this addendum.
