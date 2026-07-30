# Building Sentinel

Sentinel uses CMake presets (`CMakePresets.json`) to standardize builds across Linux, macOS, and Windows.

---

## 1. Quick Build via CMake Presets (Recommended)

### Build Application
```bash
# Configure default build preset
cmake --preset default

# Build all targets using Ninja
cmake --build --preset default
```

### Build and Run Tests Preset
```bash
# Configure test preset
cmake --preset tests

# Build test binaries
cmake --build --preset tests

# Run all unit tests via CTest
ctest --preset tests
```

---

## 2. Explicit Custom CMake Build Commands

If you prefer building without presets:

```bash
# Configure build directory
cmake -S . -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DSENTINEL_BUILD_TESTS=ON

# Compile using Ninja
cmake --build build --parallel

# Execute binaries
./build/apps/sentinel-desktop/sentinel-desktop
```

---

## 3. Build Targets Overview

- `sentinel-desktop`: Main QML/C++ executable target.
- `sentinel_core`: Core modular C++ library (services, stores, provider interfaces).
- `sentinel_ui`: QML view models and UI presentation backend.
- `sentinel_tests`: Complete unit test suite executable.
