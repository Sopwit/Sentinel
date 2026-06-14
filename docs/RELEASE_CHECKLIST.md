# Sentinel 1.0 RC Release Checklist

Phase 52 prepares release-candidate packaging and validation. It does not authorize telemetry,
silent updates, hidden cloud calls, autonomous tools, or new runtime authority.

## Build

- Configure `cmake --preset tests`.
- Build `cmake --build --preset tests`.
- Run `ctest --preset tests --output-on-failure`.
- Build the desktop target: `cmake --build --preset tests --target sentinel-desktop`.
- Configure `cmake --preset release`.
- Build `cmake --build --preset release --target sentinel-desktop`.
- For release artifacts, configure `cmake --preset package-ready -DSENTINEL_BUILD_NUMBER=<number>`.

## Tests And QA

- Run `git diff --check`.
- Run the full CTest suite.
- Run `qmllint` where Qt tooling is available.
- Run `clang-format --dry-run --Werror` on `apps`, `core`, and `tests`.
- Treat `clang-tidy` as an optional release gate until the project has a stable checked-in profile.
- Smoke launch on each target platform from a clean build.
- On the current Linux sandbox/offscreen environment, exit 139 is a known smoke-launch limitation
  when it appears after successful configure, build, and tests.

## Packaging

- Verify app version, build number, git commit, build type, platform, and architecture in About
  and Diagnostics.
- Verify macOS bundle id, display name, icon, Info.plist, copyright, category, and minimum macOS
  target.
- Verify Windows icon and version resource metadata.
- Verify Linux desktop file, AppStream metadata, SVG icon install, and PNG icon install.
- Build platform packages only with explicit operator action. Missing packaging tools must not
  fail normal builds.
- Do not require signing credentials for local validation builds.

## Privacy

- Telemetry remains absent.
- Cloud providers remain disabled unless explicitly configured by a user-facing future phase.
- No hidden uploads, hidden indexing, hidden filesystem scans, or silent update checks are present.
- Diagnostics export is user-initiated and written only to the app-controlled export directory.
- Logs are not automatically collected or uploaded.

## Security

- Filesystem access is permission-gated.
- Tools are permission-gated.
- Agents are approval-gated and metadata-only unless a later explicit execution phase changes that.
- Voice capture/playback is disabled unless explicitly enabled in a later phase.
- Credentials are never stored in plaintext app settings.
- Update downloads and installs are not automatic.

## Accessibility

- Keyboard navigation works for shell, Settings, dialogs, and Command Palette.
- Focus indicators are visible.
- Reduced motion, high contrast, and UI density preferences persist locally.
- Primary controls retain descriptive labels.

## Update Behavior

- Check for Updates is manual only.
- No background polling.
- No auto-download.
- No auto-install.
- Future update mechanisms must include signature verification and platform rollback planning.

## Known Limitations

- Packaging scripts are planning helpers only and do not sign, notarize, upload, or publish.
- Real updater networking is not implemented.
- Some future providers, tools, agents, voice, and packaging formats remain documented plans.
- Linux offscreen smoke launch may exit 139 in the current sandbox.

## Platform Validation

- Fedora KDE Plasma: primary manual validation target.
- Ubuntu or Debian derivative: Linux portability validation.
- Windows 11: executable metadata, icon, startup, and settings paths.
- macOS 12 or newer: bundle metadata, icon, startup, and settings paths.
