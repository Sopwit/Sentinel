# Diagnostics

Phase 51 adds a local Diagnostics Center.

## Displayed Data

- Sentinel version.
- Build number.
- Git commit hash when available.
- Build type.
- Platform.
- Architecture.
- Qt version.
- Active provider.
- Active model.
- Workspace count.
- Brain entry count.
- Controlled task statistics.
- Notification statistics.

## Export

Diagnostics can be exported as TXT or JSON to Sentinel's app-controlled export directory:

`QStandardPaths::AppDataLocation + "/exports"`

## Boundary

Diagnostics are local summaries. They do not upload analytics, expose raw provider payloads,
include secrets, start provider calls, scan files, or grant execution authority.

Logs are not automatically collected or uploaded. Any future log export must be user-initiated and
reviewed through the same diagnostics boundary.
