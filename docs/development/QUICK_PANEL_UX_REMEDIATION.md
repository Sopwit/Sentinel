# Quick Panel focused remediation

2026-10-09. Existing native Qt/QML layout retained; no Desktop Phase 1 redesign.

- Add focusable connection-detail action plus visible connection text; transport status never asserts model readiness. Detail popup exposes runtime/model and original technical error.
- Explain request-timeout as absent daemon reply, provider-unavailable and unavailable-model separately. Unknown failures remain visible; no timeout values or retry semantics altered. No current live timeout reproduced during the read-only hello preflight.
- Grow prompt/preview within compact bounds. Enter sends only when ready/not busy/non-empty; Shift+Enter inserts a line. Rejected dispatch preserves text.
- Reuse themed SentinelButton for Allow Once/Deny. Preserve existing approval authority and busy selector locking.
- Escape closes details first and restores prompt focus; subsequent Escape closes the panel. NativeCompanionAdapter was updated because its application event filter otherwise swallowed Escape and hid the whole panel. Existing outside-click/deactivation/tray/global-shortcut behavior remains in that adapter.

UNIT/INTEGRATION: actual QML fixture tests cover disconnected disabled input, Chat/Agent submission, Shift+Enter, focus, detail-popup Escape, 420/320 layout, busy locking, friendly timeout and raw diagnostics. Existing Desktop IPC/native integration tests exercise transport/approval seams. NATIVE OBSERVATION: actual Qt window grabs of light/dark synthetic error, idle and approval. These are clearly test fixtures, not live Agent/model outcomes.

Not accepted: actual user tray invocation/global shortcut, OS appearance change, multi-monitor positioning, microphone/STT, notification delivery/click routing, screen reader or battery/startup latency. Existing storage and native permission boundaries were unchanged.
