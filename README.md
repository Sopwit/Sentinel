# Sentinel

Sentinel is a modular AI Operating Layer for native desktop-first companion software. It is not a simple chatbot. The long-term direction includes Linux, macOS, Windows, Raspberry Pi, Jetson, and wearable devices, but this repository currently contains only the Desktop release-candidate foundation.

## Sentinel 1.0 Release Candidate Preparation

Phase 52 prepares Sentinel for a professional cross-platform 1.0 release candidate. Release
metadata, packaging manifests, QA plans, and project templates are present, while privacy and
authority boundaries remain unchanged:

- No telemetry.
- No hidden cloud calls.
- No silent update checks.
- No automatic downloads or installs.
- No autonomous tool or agent execution.

Release-candidate references:

- [Packaging and distribution](docs/PACKAGING.md)
- [Release checklist](docs/RELEASE_CHECKLIST.md)
- [Release QA plan](docs/QA_PLAN.md)
- [Release notes](RELEASE_NOTES.md)
- [Changelog](CHANGELOG.md)

## Desktop Foundation

This first version includes:

- CMake + Ninja project structure.
- Qt 6 + QML desktop shell.
- Modular C++ core library.
- `IChatProvider` with a deterministic `LocalEchoProvider`.
- Structured in-memory `ChatSession` with a QML-safe chat history model.
- `IMemoryStore` with `InMemoryStore` for tests and `SQLiteMemoryStore` for desktop persistence.
- `IChatHistoryStore` with `SQLiteChatHistoryStore` for desktop chat history persistence.
- Generic chat history status and clear-confirmation UX.
- Memory storage diagnostics and SQLite schema metadata preparation.
- `IPlugin` and `IContextEngine` interfaces.
- `ApplicationController` and `ModeManager`.
- `AppSettings` with `ISettingsStore`, in-memory test storage, and lightweight JSON desktop storage.
- `DesktopShellViewModel` as the QML boundary.
- HUD-style dashboard, sidebar navigation, mode switcher, chat panel, settings placeholder, and memory placeholder.
- Release metadata for version, build number, git commit, build type, platform, and architecture.
- Cross-platform icon and app metadata foundations for macOS, Windows, and Linux.

## Build

Requirements:

- CMake 3.24 or newer.
- Ninja.
- C++20 compiler.
- Qt 6.5 or newer with `Core`, `Gui`, `Quick`, `Qml`, `Sql`, and `Test`.

Configure and build:

```bash
cmake -S . -B build -G Ninja
cmake --build build
```

Preset-based debug build:

```bash
cmake --preset debug
cmake --build --preset debug
```

If Qt is not discoverable automatically, pass `CMAKE_PREFIX_PATH`:

```bash
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/gcc_64
cmake --build build
```

Common Qt prefix examples:

```bash
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=$HOME/Qt/6.7.0/gcc_64
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/qt
```

## Run

```bash
./build/apps/sentinel-desktop/sentinel-desktop
```

On macOS bundle builds, the binary may be inside:

```bash
./build/apps/sentinel-desktop/sentinel-desktop.app/Contents/MacOS/sentinel-desktop
```

## Tests

Run the isolated C++ core tests with CTest:

```bash
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

Current tests cover `ModeManager`, memory stores, chat history storage, chat/session behavior, settings, providers, `ApplicationController`, and desktop view-model behavior including chat history status. They do not launch the QML UI.

Preset-based test workflow:

```bash
cmake --preset tests
cmake --build --preset tests
ctest --preset tests
```

Development workflow details are in [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md). Test details are in [docs/TESTING.md](docs/TESTING.md).

## Release Builds

```bash
cmake --preset release
cmake --build --preset release --target sentinel-desktop
```

Package-ready local validation builds can set an explicit build number:

```bash
cmake --preset package-ready -DSENTINEL_BUILD_NUMBER=52
cmake --build --preset package-ready
```

Packaging tools, signing credentials, notarization credentials, package upload credentials, and
update feeds are intentionally not required by normal builds.

## Open With CLion

Open the repository root in CLion. CLion should detect the top-level `CMakeLists.txt`. Use a Ninja-based CMake profile and ensure Qt 6 is available through your environment or `CMAKE_PREFIX_PATH`.

## Open With VS Code

Open the repository root directly. Configure the `no-ccache` CMake preset once so clangd can use
the checked-in `.clangd` compilation database path. For Qt QML module resolution, set
`QML_IMPORT_PATH` and `QML2_IMPORT_PATH` to your Qt `qml` directory (examples are in
`docs/DEVELOPMENT.md`).

## Intentionally Not Included Yet

- Real AI API calls.
- Network code.
- Voice input or output.
- Automation agents.
- Cloud sync.
- Runtime plugin loading.
- Wearable support.
- Advanced semantic memory.
- Multi-conversation chat threads.
- Chat history encryption, export, or pruning.

See [docs/ROADMAP.md](docs/ROADMAP.md) for planned phases.
