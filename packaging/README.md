# Packaging sources

`packaging/` contains distribution-specific assets and metadata for Sentinel
Desktop. CMake/CPack remains the canonical package configuration, and the
release workflow owns signing, notarization, artifact upload, and publishing.
No credentials, certificates, or release build output belong here.

```
packaging/
├── linux/       # desktop/AppStream, services, DEB/RPM inputs, Flatpak, Snap
├── macos/       # bundle metadata, entitlements, Cask and release helpers
└── windows/     # executable/installer metadata, CPack artwork, signing, Winget
```

Canonical cross-platform application assets, including the application icons,
remain in `resources/`. Platform bundle and installer inputs live with their
respective package definitions. Flatpak and Snap files are source inputs; they
are not published automatically by the GitHub release workflow.

See [Packaging](../docs/release/PACKAGING.md) and
[Release process](../docs/release/RELEASE_PROCESS.md).
