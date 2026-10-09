# Phase 1 native evidence — 2026-10-09

All after captures show real production QML inside the disposable certification harness on macOS/Metal, not the production Main root. Notification content, connection state and onboarding state are fixtures. No personal conversations, credentials or OS notification permissions were used. Harness toolbar buttons are test controls, not Sentinel product styling.

`before-*` are byte-identical copies of the original audit captures, preserved for comparison; they are not new baseline launches. Original evidence remains unchanged.

| Capture | Observation |
|---|---|
| 01-voice-light.jpg | Themed Voice buttons, full category labels, no empty status reservation |
| 02-general-shortcut-popup-light.jpg | Shared shortcut selector, selected popup and focus |
| 03-history-empty-light.jpg | Intermediate empty-history capture; final empty-label placement was subsequently corrected and integration-tested. Do not treat as final visual acceptance. |
| 04-command-search-unavailable.jpg | Search commands visibly unavailable instead of fake success |
| 05-voice-dark-icons.jpg | Dark themed controls and readable shared rail icons |
| 06-history-populated-dnd.jpg | Synthetic unread history retained during DND |
| 07-history-archived-dnd.jpg | Actual mark-read/archive actions; archived item retained |
| 08-daemon-status-details.jpg | Non-color connection detail with fixture daemon/model state; final focus behavior additionally integration-tested |
| 09-onboarding-truthful-finish.jpg | Preferences saved while disconnected; provider/model readiness unconfirmed; scrollable finish |
| 10-voice-compact-dark.jpg | 780×640 category selector and wrapping Voice action layout |
| 11-voice-prerequisite-chat.jpg | Actual missing-engine failure, friendly guidance and working Voice settings route; no microphone capture |

Native observations do not establish Linux/Windows, live-daemon, OS banner, screen-reader or full WCAG acceptance. Dark notification card shadows remain inherited styling and warrant a later visual pass. See the Phase 1 report for test scope. SHA256SUMS covers all JPEG evidence without image manipulation.
