# Testing

Sentinel uses Qt Test for isolated C++ unit tests.

## Run Tests

Preferred preset workflow:

```bash
cmake --preset tests
cmake --build --preset tests
ctest --preset tests
```

Equivalent explicit CMake workflow:

```bash
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

## Current Coverage

- `ModeManager`: default mode, available modes, repeated changes, duplicate mode selection, and unknown mode names.
- `InMemoryStore`: missing keys, empty keys, put/get, repeated overwrites, multiple independent keys, and deterministic entry ordering.
- `ApplicationController`: initial state, local provider responses, unavailable-provider handling, blank chat rejection, memory key validation, runtime memory writes, and overwrite behavior through `IMemoryStore`.
- `LocalEchoProvider`: provider name/status, stable status labels, and deterministic local response.
- `AppSettings`: defaults, input normalization, duplicate updates, blank input handling, and configuration profile changes.
- `JsonSettingsStore`: missing files, directory creation, and persistence across instances.
- `DesktopShellViewModel`: QML-facing state exposure and forwarding for chat, mode, memory, and settings actions.

## Test Boundaries

Tests link against `sentinel_core` and do not launch QML. This keeps the test suite deterministic and suitable for CI.
