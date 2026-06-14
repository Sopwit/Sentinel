# Updates

Phase 51 provides a manual update experience only.

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
