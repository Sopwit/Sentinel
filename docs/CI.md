# CI Roadmap

The current CI baseline is `.github/workflows/linux-ci.yml`.

## Current Linux CI

- Checkout.
- Install CMake, Ninja, C++ compiler, Qt build dependencies, and clang-format.
- Install Qt 6.
- Configure with `cmake --preset tests`.
- Build with `cmake --build --preset tests`.
- Test with `ctest --preset tests`.
- Run clang-format in dry-run mode.

## Release Builds (CI/CD)

The project includes an automated multi-platform release pipeline (`.github/workflows/release.yml`) that triggers on tag pushes matching `v*` (or via manual run):
- **Linux (GCC / Ubuntu 24.04):** Produces a portable `.tar.gz` package.
- **macOS (Clang / macOS 14 Apple Silicon):** Produces a native `.dmg` Disk Image.
- **Windows (MSVC / Windows 2022):** Produces a portable `.zip` package.

Upon successful completion, compiled artifacts are published directly to a GitHub Release.

## Future CI/CD Enhancements

- Add `qmllint` where Qt tooling is stable in CI.
- Add optional `clang-tidy` only after a checked-in profile and suppression policy exist.
- Add signed/notarized/repository packages only after secrets, credential ownership, and release approval policy are documented.

CI must not add telemetry, hidden update checks, cloud credentials, signing assumptions, or package publishing side effects.
