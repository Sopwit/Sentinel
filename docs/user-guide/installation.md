# Installation Guide

Sentinel is cross-platform personal AI desktop application, optimized first for **Fedora KDE Plasma**.

---

## 1. Fedora KDE Plasma (Primary Target)

### System Requirements
- Fedora Linux 38+ (KDE Plasma 5.27+ or Plasma 6)
- Qt 6.5 or higher (`qt6-qtdeclarative`, `qt6-qtbase`, `qt6-qtsql`)
- CMake 3.25+ and Ninja
- GCC 13+ or Clang 16+ supporting C++20

### Install Dependencies
```bash
sudo dnf install -y \
    cmake ninja-build gcc-c++ \
    qt6-qtbase-devel qt6-qtdeclarative-devel qt6-qtsql-devel \
    sqlite-devel
```

### Running Sentinel AppImage or Build Binary
```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/apps/sentinel-desktop/sentinel-desktop
```

---

## 2. Ubuntu / Debian

### Install Dependencies
```bash
sudo apt update && sudo apt install -y \
    build-essential cmake ninja-build \
    qt6-base-dev qt6-declarative-dev libqt6sql6-sqlite \
    libsqlite3-dev
```

### Build & Execute
```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/apps/sentinel-desktop/sentinel-desktop
```

---

## 3. macOS (Apple Silicon & Intel)

### System Requirements
- macOS 13 (Ventura) or newer
- Xcode Command Line Tools (`xcode-select --install`)
- Homebrew

### Install Dependencies
```bash
brew install cmake ninja qt@6 sqlite
```

### Build & Execute
```bash
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH="$(brew --prefix qt@6)"
cmake --build build
./build/apps/sentinel-desktop/sentinel-desktop.app/Contents/MacOS/sentinel-desktop
```

---

## 4. Windows 11 / 10

### System Requirements
- Windows 10/11 64-bit
- MSVC 2022 (Visual Studio 2022) with C++ Desktop workload
- Qt 6.5+ (installed via Qt Online Installer or vcpkg)

### Build via PowerShell
```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
.\build\apps\sentinel-desktop\Release\sentinel-desktop.exe
```
