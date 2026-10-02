# Installation

Sentinel release artifacts are built for Linux, macOS, and Windows. Fedora KDE Plasma is the primary target; Windows and macOS builds are maintained by CI.

Download a release from the project’s GitHub Releases page. The release workflow produces Linux `.deb`, `.rpm`, AppImage, and tarball artifacts; a macOS DMG; and Windows `.exe` and `.msi` installers. Verify the accompanying `SHA256SUMS.txt` before installing.

For a source build, follow [Building](../development/BUILDING.md). Local inference requires a reachable local runtime such as Ollama and an installed model. Sentinel discovers Ollama through its configured endpoint; it does not bundle Ollama or model weights.

See [Configuration](CONFIGURATION.md) for provider setup and [Data locations](../reference/DATA_LOCATIONS.md) for removal of local data.
