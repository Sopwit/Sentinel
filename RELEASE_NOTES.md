# Sentinel 1.0.0-rc.1 Release Notes

Sentinel 1.0.0-rc.1 is a release-candidate preparation milestone for packaging, distribution, and
professional validation.

## Highlights

- About and Diagnostics expose version, build number, git commit, build type, platform, and
  architecture when available.
- Release build presets are available for `debug`, `tests`, `release`, `relwithdebinfo`, and
  `package-ready`.
- macOS, Windows, and Linux application metadata is prepared, and the application is licensed under GPLv3.
- Linux AppStream and desktop metadata are included.
- Packaging, QA, CI, and Sentinel 1.0 RC checklists are documented.

## Privacy And Security

- No telemetry.
- No hidden cloud calls.
- No silent update checks.
- No automatic downloads.
- No automatic installs.
- Diagnostics export remains user-initiated and local.

## Known Limitations

- Packaging scripts do not sign, notarize, or publish packages automatically to production stores, but compile and publish release artifacts to GitHub Releases.
- Future update systems are documented only and do not perform network calls.
- Some provider/tool/agent/voice capabilities remain disabled or metadata-only unless explicitly
  activated by later phases.
- The current Linux offscreen/sandbox smoke launch can exit 139 after successful build and tests.
