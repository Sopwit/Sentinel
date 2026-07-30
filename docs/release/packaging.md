# Packaging & Distribution Guide

Sentinel supports cross-platform packaging for Linux (AppImage, RPM, DEB), macOS (DMG bundle), and Windows (MSI / standalone Zip).

---

## 1. Linux Packaging

### AppImage Build
```bash
cmake -S . -B build-appimage -G Ninja -DCMAKE_BUILD_TYPE=Release -DSENTINEL_PACKAGE_FORMAT=AppImage
cmake --build build-appimage --target package
```

### Fedora RPM & Debian DEB
```bash
# Generate RPM package via CPack
cpack -G RPM --config build-appimage/CPackConfig.cmake

# Generate DEB package via CPack
cpack -G DEB --config build-appimage/CPackConfig.cmake
```

---

## 2. macOS Bundling & DMG Creation

```bash
cmake -S . -B build-mac -G Ninja -DCMAKE_PREFIX_PATH="$(brew --prefix qt@6)" -DCMAKE_BUILD_TYPE=Release
cmake --build build-mac --target sentinel-desktop
cpack -G DragNDrop --config build-mac/CPackConfig.cmake
```

---

## 3. Windows Installer Packaging

```powershell
cmake -S . -B build-win -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build build-win --config Release --target sentinel-desktop
cpack -G WIX --config build-win/CPackConfig.cmake
```
