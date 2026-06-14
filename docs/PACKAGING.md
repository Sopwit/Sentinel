# Packaging And Distribution

Phase 52 prepares Sentinel for a professional 1.0 release-candidate packaging pass without adding
telemetry, silent update checks, hidden cloud calls, autonomous behavior, or new runtime authority.

## Version Metadata

Release builds expose these values through CMake-generated metadata:

- app id: `dev.sentinel.Sentinel`
- display name: `Sentinel Desktop`
- app version: `1.0.0-rc.1`
- numeric project version: `1.0.0`
- build number: `SENTINEL_BUILD_NUMBER`
- git commit: short commit hash when available, otherwise `unknown`
- build type, platform, architecture, and Qt version

Set a release build number explicitly for artifacts:

```bash
cmake --preset package-ready -DSENTINEL_BUILD_NUMBER=52
cmake --build --preset package-ready
```

## macOS

Prepared metadata:

- bundle identifier: `dev.sentinel.Sentinel`
- app icon: `resources/icons/dev.sentinel.Sentinel.icns`
- Info.plist template: `resources/macos/Info.plist.in`
- display name: `Sentinel Desktop`
- category: `public.app-category.productivity`
- minimum target: macOS 12.0 by default

Packaging plan:

- Build a Release or package-ready `.app` bundle with CMake.
- Use `macdeployqt` only in an explicit packaging job.
- Create a DMG with local packaging tooling after manual validation.
- Signing, notarization, and Sparkle-style updates require separate credentials and policy work.

## Windows

Prepared metadata:

- app icon: `resources/icons/dev.sentinel.Sentinel.ico`
- version resource: `resources/windows/sentinel-desktop.rc.in`
- product name, company name, product version, file version, and copyright metadata

Packaging plan:

- Build with the package-ready preset on a Windows runner or workstation.
- Use `windeployqt` only in an explicit packaging job.
- Prepare an installer with WiX, NSIS, MSIX, or another reviewed Windows packaging tool.
- Code signing and WinSparkle/MSIX-style updates require separate credentials and policy work.

## Linux

Prepared metadata:

- desktop file: `packaging/linux/dev.sentinel.Sentinel.desktop.in`
- AppStream metadata: `packaging/linux/dev.sentinel.Sentinel.metainfo.xml`
- scalable icon install from `resources/icons/dev.sentinel.Sentinel.svg`
- 1024px PNG icon install from `resources/icons/dev.sentinel.Sentinel.png`

Packaging plan:

- AppImage: use an explicit packaging job after `linuxdeploy` and Qt plugin validation.
- Flatpak: create a reviewed manifest with no network permissions beyond explicitly scoped future
  provider use.
- RPM/DEB: package installed CMake outputs and Qt runtime dependencies according to distro policy.

## Update Readiness

The current update UX remains manual-only:

- no background polling
- no automatic download
- no automatic install
- no telemetry
- no hidden cloud calls

Future update mechanisms must define trusted endpoints, package signatures, rollback behavior, and
platform-specific UX before networking is enabled.
