# Build & Development Guide

This document details toolchain prerequisites, build instructions, test execution, code style standards, and packaging workflows for developers and maintainers.

---

## 1. Toolchain Prerequisites

Sentinel requires a C++20 compliant compiler, CMake 3.24+, and Qt 6.5+ with the required Qt modules (`Core`, `Gui`, `Quick`, `Qml`, `Sql`, `Test`, `Widgets`).

### Fedora / RHEL (Recommended)
```bash
sudo dnf install -y \
  gcc-c++ cmake ninja-build ccache \
  qt6-qtbase-devel qt6-qtdeclarative-devel qt6-qttools-devel \
  qt6-qtsvg-devel sqlite-devel clang-tools-extra
```

### Ubuntu / Debian (24.04+)
```bash
sudo apt update && sudo apt install -y \
  build-essential g++ cmake ninja-build ccache \
  qt6-base-dev qt6-declarative-dev qt6-tools-dev \
  libqt6svg6-dev libsqlite3-dev clang-format clang-tidy
```

### Arch Linux
```bash
sudo pacman -S --needed \
  base-devel cmake ninja ccache \
  qt6-base qt6-declarative qt6-tools qt6-svg sqlite clang
```

### macOS (Apple Silicon & Intel)
1. Install **Xcode Command Line Tools**: `xcode-select --install`
2. Install dependencies via **Homebrew**:
   ```bash
   brew install cmake ninja ccache qt@6 sqlite clang-format
   ```
3. Ensure Qt 6 is in your environment or set `CMAKE_PREFIX_PATH`:
   ```bash
   export CMAKE_PREFIX_PATH="/opt/homebrew/opt/qt"
   ```

### Windows (MinGW / MSVC)
1. Install **Qt 6.7+** via the official Qt Online Installer (select MinGW 64-bit or MSVC 2022 64-bit component).
2. Install **CMake** and **Ninja** via `winget install Kitware.CMake Ninja-build.Ninja`.

---

## 2. Compiling from Source

### Command Line (CMake & Ninja)

```bash
# 1. Clone the repository
git clone https://github.com/Sopwit/Sentinel.git
cd Sentinel

# 2. Configure the build directory
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DSENTINEL_BUILD_TESTS=ON

# 3. Compile all targets
cmake --build build --parallel $(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
```

The resulting binaries will be placed in `build/apps/`:
- **Desktop Companion Shell:** `build/apps/sentinel-desktop/sentinel-desktop` (or `.app` on macOS / `.exe` on Windows)
- **Headless CLI:** `build/apps/sentinel-cli/sentinel-cli`
- **Background Daemon:** `build/apps/sentinel-daemon/sentinel-daemon`

---

## 3. IDE Workflows

### Qt Creator (Easiest)
1. Open **Qt Creator**.
2. Select **File > Open File or Project...** and select the top-level `CMakeLists.txt`.
3. Choose your desktop Qt 6 kit and click **Configure Project**.
4. Press `Ctrl+R` (or `Cmd+R` on macOS) to build and run.

### CLion
1. Open the repository root directory in CLion.
2. Under **Settings > Build, Execution, Deployment > CMake**, select `Ninja` as the generator and ensure your Qt 6 `CMAKE_PREFIX_PATH` is configured.
3. Select the `sentinel-desktop` target and run.

---

## 4. CMake Presets

Sentinel provides standard presets defined in `CMakePresets.json`:

```bash
# Debug build
cmake --preset debug
cmake --build --preset debug

# Release build
cmake --preset release
cmake --build --preset release

# Test suite build
cmake --preset tests
cmake --build --preset tests
ctest --preset tests
```

---

## 5. Running the Test Suite

The test suite uses **QtTest** and **CTest** to validate core modules, view-models, storage integrity, memory indexing, and tool executors:

```bash
# Run all unit tests with failure output
ctest --test-dir build --output-on-failure

# Run a specific test suite directly
./build/tests/test_fuzzy_editor
./build/tests/test_real_tool_executor_tools
./build/tests/test_local_rag_store
```

---

## 6. Code Quality, Linting & Formatting

Sentinel enforces consistent formatting standards across C++ and QML:

```bash
# Format C++ source and headers
clang-format -i core/**/*.{h,cpp} apps/**/*.{h,cpp} tests/**/*.{h,cpp}

# Validate QML components
qmllint apps/sentinel-desktop/qml/**/*.qml

# Static analysis with Clang-Tidy
clang-tidy -p build core/src/editor/FuzzyEditor.cpp
```

---

## 7. Packaging Builds

Platform packaging scripts are located in the `packaging/` directory:

```bash
# Linux RPM package (Fedora)
packaging/rpm/build_rpm.sh

# Linux AppImage
packaging/appimage/build_appimage.sh

# macOS DMG & notarization
packaging/macos/build_dmg.sh

# Windows Installer (Inno Setup / WiX)
packaging/windows/build_installer.bat
```
