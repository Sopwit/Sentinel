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

## Release-Candidate Roadmap

- Add release and package-ready configure/build jobs on Linux, Windows, and macOS.
- Add `qmllint` where Qt tooling is stable in CI.
- Add optional `clang-tidy` only after a checked-in profile and suppression policy exist.
- Publish unsigned local artifacts for review only.
- Add signed/notarized/repository packages only after secrets, credential ownership, and release
  approval policy are documented.

CI must not add telemetry, hidden update checks, cloud credentials, signing assumptions, or package
publishing side effects.
