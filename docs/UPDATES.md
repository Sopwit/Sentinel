# Updates

Phase 51 provides a manual update experience only.

Phase 52 keeps this behavior unchanged for release-candidate packaging.

## Behavior

- Check for Updates is user-initiated.
- Release notes are shown locally.
- Current version is compared against local product metadata.
- Download confirmation is explicit and visible.

## Not Implemented

- No background polling.
- No silent updates.
- No automatic downloads.
- No automatic installation.
- No telemetry.
- No hidden cloud calls.

The current implementation records local workflow state and shows product copy. A later update
phase must define trusted update endpoints, signature verification, platform packaging behavior,
rollback, and tests before real update networking is allowed.

## Future Platform Strategies

- macOS: Sparkle-style update flow only after signing, notarization, feed signing, rollback, and
  explicit user-consent UX are designed.
- Windows: WinSparkle/MSIX-style flow only after code signing, package identity, rollback, and
  explicit user-consent UX are designed.
- Linux: prefer distro/package-manager, Flatpak, AppImage, RPM, or DEB update strategies that are
  visible to the user and respect platform policy.

No future strategy may introduce background polling, auto-download, auto-install, telemetry, or
hidden cloud calls by default.
