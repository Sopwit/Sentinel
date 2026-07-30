# Packaging Specifications & Distribution Metadata

This directory contains cross-platform distribution metadata, specs, and manifests for **Sentinel Desktop** (`dev.sentinel.Sentinel`).

## Packaging Layout

```
packaging/
├── flatpak/
│   ├── org.sentinel.Sentinel.yml   # Primary Flatpak manifest (YAML)
│   ├── dev.sentinel.Sentinel.yaml  # Flathub-ready Flatpak manifest (YAML)
│   ├── dev.sentinel.Sentinel.json  # Flathub-ready Flatpak manifest (JSON)
│   └── build_flatpak.sh          # Local Flatpak builder script
├── linux/
│   ├── fedora-kde/                 # Fedora KDE Plasma specific packaging
│   │   ├── sentinel-desktop.spec   # Enterprise Fedora RPM specification
│   │   ├── build_copr.sh           # Fedora COPR automation script
│   │   └── dev.sentinel.Sentinel.metainfo.xml # AppStream metadata
│   ├── snap/                       # Ubuntu Snapcraft packaging
│   │   └── snapcraft.yaml
│   ├── dev.sentinel.Sentinel.desktop.in # freedesktop Desktop Entry
│   └── dev.sentinel.Sentinel.metainfo.xml # AppStream 0.16 metadata
├── windows/
│   └── winget/                     # Windows Package Manager (Winget) manifest
│       └── Sopwit.Sentinel.yaml
├── rpm/
│   └── sentinel-desktop.spec       # Enterprise Fedora RPM specification
├── dbus/
│   └── dev.sentinel.Sentinel.service # D-Bus session activation service
├── systemd/
│   └── sentinel-desktop.service    # systemd user session service
├── etc/
│   └── sentinel/
│       └── config.json.template    # Enterprise /etc default system configuration
└── macos/                           # macOS packaging assets


```

## Linux & Fedora KDE Plasma Build Instructions

### 1. Native Fedora RPM Package (`sentinel-desktop.spec`)
To build the RPM package natively using `rpmbuild` or `mock`:

```bash
# Prepare source tarball
git archive --prefix=sentinel-desktop-1.0.0/ -o sentinel-desktop-1.0.0.tar.gz HEAD

# Build RPM package
rpmbuild -ta sentinel-desktop-1.0.0.tar.gz
```

### 2. Flatpak Build & Sandbox Test
To build and test the Flatpak package locally using `build_flatpak.sh`:

```bash
# Build Flatpak package locally
./packaging/flatpak/build_flatpak.sh

# Build and run inside Flatpak sandbox
./packaging/flatpak/build_flatpak.sh --run
```

### 3. Snapcraft (Ubuntu Snap) Build
To build the Snap package using `snapcraft`:

```bash
cd packaging/linux/snap && snapcraft
```

### 4. Windows Package Manager (Winget) Manifest
To validate and submit the Winget package manifest:

```bash
winget validate packaging/windows/winget/Sopwit.Sentinel.yaml
```

### 5. CPack Build (RPM, DEB, Tarball, NSIS, WiX)
```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
cd build && cpack
```

## Portable Data Mode

Sentinel supports fully isolated portable operation. When running in Portable Data Mode, all configuration files (`settings.json`), SQLite databases (`memory.sqlite3`, `chat_history.sqlite3`, `conversations.sqlite3`, `local_rag.sqlite3`), logs, and crash dumps are saved inside the application's executable directory instead of system `%APPDATA%` or `~/.local/share` directories.

To enable Portable Data Mode:
1. Pass the `--portable` CLI argument when launching Sentinel:
   ```bash
   sentinel-desktop --portable
   ```
2. Or create an empty marker file named `portable.txt` in the same directory as the `sentinel-desktop` executable.

## Cross-Platform Compatibility

- **Linux / Fedora KDE Plasma**: Full native FHS 3.0 layout, systemd user service, D-Bus session activation, AppStream metadata, desktop entry, RPM spec, Flatpak manifest, and Fedora COPR build automation (`packaging/linux/fedora-kde/build_copr.sh`).
- **macOS**: App bundle structure (`Sentinel Desktop.app`), Info.plist, code signing entitlements, DMG packaging via CPack / DragNDrop.
- **Windows**: Executable manifest, MSVC / MinGW DLL bundling, NSIS installer (`.exe`), WiX installer (`.msi`), and signtool code signing integration.

