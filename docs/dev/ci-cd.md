# CI/CD & Automation Pipelines

Sentinel uses GitHub Actions for continuous integration and automated build verification.

---

## 1. Automated Workflow Jobs

Every push and Pull Request triggers the CI pipeline (`.github/workflows/ci.yml`):

1. **Linux Build & Test (Fedora / Ubuntu)**:
   - Compiles C++ code with GCC 13 and Clang 16.
   - Runs full CTest suite (`ctest --preset tests`).
   - Verifies `.clang-format` and `.qmllint.ini` compliance.

2. **macOS Build & Test**:
   - Compiles ARM64 and x86_64 binaries with Apple Clang.
   - Executes CTest suite.

3. **Windows Build & Test**:
   - Compiles MSVC 2022 64-bit target.
   - Verifies test execution.

---

## 2. Release & Packaging Pipeline

- **AppImage Build**: Generates self-contained AppImage packages for Linux/Fedora.
- **macOS App Bundle**: Generates signed `.dmg` bundles.
- **Windows Packaging**: Generates standalone zip/installer distributions.
