# Security Policy

Sentinel defaults to explicit user authority.

## Supported Security Posture

- No telemetry.
- No silent update checks.
- No hidden cloud calls.
- No automatic downloads or installs.
- No autonomous tools or agents.
- Diagnostics export is user-initiated and local.

## Reporting

Do not file public issues containing secrets, credentials, private transcripts, or sensitive local
paths. Use the repository owner's private security reporting channel when one is available.

## Release Requirements

Before Sentinel 1.0 RC distribution:

- Run the release checklist in `docs/RELEASE_CHECKLIST.md`.
- Confirm update behavior remains manual-only.
- Confirm package signing/notarization credentials are not checked in.
- Confirm diagnostics and logs are not uploaded automatically.
