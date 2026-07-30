# Development Environment Setup

This guide details how to set up your local development environment for building and contributing to Sentinel.

---

## Toolchain Requirements

- **C++ Compiler**: GCC 13+, Clang 16+, or MSVC 2022 (supporting C++20 standard feature set).
- **Build System**: CMake 3.25+ and Ninja 1.10+.
- **GUI Toolkit**: Qt 6.5 or higher (`Core`, `Gui`, `Quick`, `Qml`, `Sql`).
- **Database**: SQLite 3 (`Qt6::Sql` with `QSQLITE` driver).

---

## Recommended IDE Configurations

### 1. JetBrains CLion
1. Open the repository root directory in CLion.
2. CLion will automatically detect `CMakePresets.json`.
3. Select the `default` or `tests` preset in **Settings -> Build, Execution, Deployment -> CMake**.
4. Set Generator to `Ninja`.

### 2. VS Code
1. Install extensions: **C/C++**, **CMake Tools**, and **Qt Tools**.
2. Run command: `CMake: Select Configure Preset` -> select `default` or `tests`.
3. Build target: `sentinel-desktop` or `all`.

### 3. Qt Creator
1. Open `CMakeLists.txt` in Qt Creator.
2. Select Qt 6.5+ Kit.
3. Configure build directory as `build/`.
