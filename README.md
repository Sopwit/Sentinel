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

Sentinel 1.0-RC1 includes a comprehensive desktop companion core and native UI shell:

- **CMake & Ninja Build**: Standard cross-platform desktop build framework.
- **Qt 6 & QML Desktop Shell**: Optimised for Fedora KDE Plasma, with native window controls, layout adaptations, and custom glass styling.
- **Local AI Execution**: Real local streaming chat integration with Ollama (via loopback HTTP) and support for LM Studio, llama.cpp, and OpenAI-compatible local provider configurations.
- **Workspace Management**: Standard built-in (Personal, Coding, Student, etc.) and user-created custom workspaces with isolated preferences and data scopes.
- **Collapsible Sidebar**: Full multi-conversation thread browser with pinning, renaming, filtering, archiving, and chat message count metadata.
- **Local RAG & File Chat**: Local SQLite-backed Knowledge Base with drag-and-drop document attachments (PDF, TXT, Markdown, CSV, JSON, source code) and retrieval explainability context.
- **Controlled Agent Tasks**: User-approved foreground agent workflows with plan step editing, skip/retry actions, and granular workspace-scoped tool permissions.
- **Context Observability**: Detailed explainability panels showing prompt context budget allocations, compression, contribution weights, and retrieval decisions.
- **Notification Center**: Multi-category center covering Tasks, Models, Updates, Brain, Workspace, and Security alerts.
- **Command Palette**: Universal keyboard shortcut (`Ctrl/Cmd+K`) for quick navigation, mode toggles, and chat history export actions.
- **Native Companion Integration**: Native system tray and menu bar adapter backed by `QSystemTrayIcon` for quick access.
- **Theme & Accessibility**: Curated styles (Sentinel Dark, Midnight, Aurora, Graphite, System Adaptive) with reduced motion, high contrast, and UI density controls.
- **Security & Privacy Boundaries**: No telemetry, no silent updates, no automatic downloads, and no external cloud calls.
- **Localization (i18n)**: Translation frameworks and catalog preparation for English and Turkish locales.
- **Persistence separation**: Safe local SQLite databases for Chat History, Brain Memories, and Local RAG metadata, separated from settings JSON.

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

### Linux / macOS

If built manually using the default directory (`cmake -S . -B build`):

```bash
./build/apps/sentinel-desktop/sentinel-desktop
```

If built using a CMake preset (e.g., `debug`, `release`, or `package-ready`), run the binary from the corresponding directory under `build/`:

```bash
./build/<preset-name>/apps/sentinel-desktop/sentinel-desktop
```

For example, for the `debug` preset:

```bash
./build/debug/apps/sentinel-desktop/sentinel-desktop
```

On macOS bundle builds, the binary is inside the application bundle:

```bash
# Default build directory:
./build/apps/sentinel-desktop/sentinel-desktop.app/Contents/MacOS/sentinel-desktop

# Preset-based build directory (e.g., debug):
./build/debug/apps/sentinel-desktop/sentinel-desktop.app/Contents/MacOS/sentinel-desktop
```

### Windows

On Windows, executables have the `.exe` extension and use backslashes (`\`) for file paths.

If built manually using the default directory:

```cmd
build\apps\sentinel-desktop\sentinel-desktop.exe
```

If built using a CMake preset (e.g., `debug`, `release`, or `package-ready`), run the binary from the corresponding directory under `build/`:

```cmd
build\<preset-name>\apps\sentinel-desktop\sentinel-desktop.exe
```

For example, for the `debug` preset:

```cmd
build\debug\apps\sentinel-desktop\sentinel-desktop.exe
```

## Tests

Run the isolated C++ core tests with CTest:

```bash
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

Current tests cover `ModeManager`, memory/chat/conversation/RAG stores, local AI inference/streaming clients, controlled task planner, secure credentials, path provider, view-model boundaries, and desktop view-model behavior. They do not launch the QML UI.

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

- Cloud-based AI API calls (external network connections).
- External network requests (only local loopback connections are used).
- Voice processing execution (Whisper STT and Piper TTS remain metadata-only/readiness boundaries).
- Autonomous or unsupervised agent execution (tasks require step-by-step foreground approval).
- Cloud sync or remote backup.
- Dynamic runtime plugin loading.
- Wearable or IoT hardware support.
- Fully automated semantic indexing (Local RAG is manual-only, semantic prompt authority is disabled by default).
- Chat history encryption or automated pruning (local export is supported).

See [docs/ROADMAP.md](docs/ROADMAP.md) for planned phases.
