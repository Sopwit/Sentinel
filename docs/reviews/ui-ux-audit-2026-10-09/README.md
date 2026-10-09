# UI/UX audit evidence — 2026-10-09

22 original JPEG captures from the existing native `settings_ui_certification_probe` running production QML in a disposable profile. Captured via the computer-use screenshot API; no mockups, compositing or image edits. No credentials, private transcripts or sensitive file paths were used. All theme/onboarding changes were confined to the disposable profile. Normal user profile untouched.

**HARNESS evidence, not production Main.qml or live-daemon certification.** The four top toolbar buttons and certification window title are wrapper UI; exclude them from product findings. Provider/catalog/Quick Panel fixtures must not be mistaken for actual backend connectivity. At 2× display scale, regular 1100×780 content yields 2200×1624 image including native titlebar; compact 780×640 yields 1560×1344 including titlebar. No platform acceptance PASS is implied.

| Capture | Surface IDs | Theme / state |
| --- | --- | --- |
| [01](01-onboarding-welcome.jpg) | S33 | Light; welcome, safe initial focus on Next |
| [02](02-onboarding-processing.jpg) | S34 | Light; local processing preference |
| [03](03-onboarding-provider.jpg) | S35 | Light; provider controls, fixture readiness text |
| [04](04-onboarding-model.jpg) | S36 | Light; no model configured |
| [05](05-onboarding-voice.jpg) | S37 | Light; missing STT/TTS prerequisite, device choice |
| [06](06-onboarding-privacy.jpg) | S38 | Light; privacy/tool/workspace boundary copy |
| [07](07-onboarding-finish.jpg) | S39 | Light; finish summary, provider/model set up later |
| [08](08-settings-general-light.jpg) | S16 | Light; language/Desktop, raw shortcut selector |
| [09](09-settings-appearance-light.jpg) | S17 | Light; presets, category keyboard focus ring |
| [10](10-settings-providers-light.jpg) | S18 | Light; unconfigured model / disabled runtime |
| [11](11-settings-voice-light.jpg) | S19 | Light; microphone actions and audio devices |
| [12](12-settings-workspace-light.jpg) | S20 | Light; empty response instructions, memory status |
| [13](13-settings-security-light.jpg) | S21 | Light; retention and initial safety controls |
| [14](14-settings-notifications-light.jpg) | S22 | Light; history action, policy/channels |
| [15](15-settings-system-light.jpg) | S23 | Light; backup entry controls and healthy empty recovery |
| [16](16-chat-empty-light.jpg) | S02/S04 | Light; empty greeting, composer, collapsed history |
| [17](17-chat-history-light.jpg) | S03/S04 | Light; expanded disposable history panel |
| [18](18-chat-compact-light.jpg) | S02/S04 | Light; 780×640 compact composer / elided provider |
| [19](19-settings-system-compact.jpg) | S23 | Light; compact category selector and scrollable System |
| [20](20-settings-appearance-dark.jpg) | S17 | Obsidian; compact, theme selector focus, dark rail icons |
| [21](21-chat-empty-dark-focus.jpg) | S02/S04 | Obsidian; compact empty chat, Tab focus to composer |
| [22](22-chat-voice-error-dark.jpg) | S04 | Obsidian; missing STT error and voice button focus |

18 distinct surfaces captured: S02, S03, S04, S16–S23 and S33–S39. Other surfaces/uncaptured states and exact limitations are listed in the [inventory](../../development/UI_UX_SCREEN_INVENTORY.md). Source-only panels are not silently counted as reachable screens. No new images establish successful chat inference, genuine loading, active Agent, approval, populated Inspector, real download/restore, tray dismissal or installation.

The session's AX tree confirmed category navigation, named composer controls and a provider runtime-status accessible name. Compact category selection and theme popup selection worked with Home/Down/Return; Tab reached the composer. This is not a full keyboard or screen-reader certification.
