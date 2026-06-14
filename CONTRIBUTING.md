# Contributing

Sentinel is a C++20, Qt 6, QML, CMake, and SQLite desktop application.

## Development Rules

- Preserve MVVM boundaries: QML presents state and calls view-model actions; C++ owns logic.
- Keep provider behavior behind `IChatProvider`.
- Keep memory, settings, and chat history persistence separate.
- Keep platform-specific behavior behind service interfaces.
- Do not add telemetry, hidden cloud calls, silent update checks, autonomous behavior, or new
  runtime authority without an explicit phase.

## Local Validation

```bash
cmake --preset tests
cmake --build --preset tests
ctest --preset tests --output-on-failure
```

Before release-oriented changes, also run:

```bash
git diff --check
cmake --preset release
cmake --build --preset release --target sentinel-desktop
```

Optional checks:

- `qmllint`
- `clang-format --dry-run --Werror`
- `clang-tidy` after a project profile exists

## Pull Requests

- Keep changes scoped.
- Update docs when architecture, privacy, security, packaging, or phase boundaries change.
- Do not include signing credentials, API keys, secrets, update feed credentials, or generated
  packages.
