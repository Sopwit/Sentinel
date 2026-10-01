# Packaging

Packaging inputs live in `packaging/` and CPack configuration lives in `cmake/SentinelCPack.cmake`. Fedora RPM specs, Flatpak manifests, Snap metadata, desktop/AppStream metadata, macOS assets, and Windows signing/Winget metadata are maintained there.

The tagged release workflow builds the `package-ready` preset and currently publishes:

| Platform | Artifacts |
| --- | --- |
| Linux | DEB, RPM, AppImage, and tarball |
| macOS | DMG |
| Windows | EXE and MSI |

CPack is also configured for platform-native package generation. Flatpak and Snap files are packaging sources, not automatically published by the GitHub release workflow. See [Release process](RELEASE_PROCESS.md).
