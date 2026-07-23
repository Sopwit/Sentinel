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
- **Theme & Accessibility**: Curated styles (Liquid Glass Light - default, Liquid Glass Dark, Sentinel Classic, Midnight Blue, Aurora Teal, Graphite Grey, System Sync) with transparency, reduced motion, high contrast, and UI density controls.
- **Security & Privacy Boundaries**: No telemetry, no silent updates, no automatic downloads, and no external cloud calls.
- **Localization (i18n)**: Translation frameworks and catalog preparation for English and Turkish locales.
- **Persistence separation**: Safe local SQLite databases for Chat History, Brain Memories, and Local RAG metadata, separated from settings JSON.

## Build & Run

Sentinel is a cross-platform Qt/C++ application. You can build and run it either using **Qt Creator** (easiest for all platforms, especially Windows/macOS) or via the **Command Line**.

### Prerequisites

- **CMake** 3.24 or newer.
- **C++20 Compiler** (GCC 13+, Clang 15+, or MSVC 2022+).
- **Qt 6.5 or newer** with `Core`, `Gui`, `Quick`, `Qml`, `Sql`, and `Test`.
- **Ninja** (optional, recommended for command-line preset builds).
- **Git** (to manage versioning).

---

### Option A: Qt Creator (Easiest / Recommended)

Qt Creator handles toolchains, compiler paths, and build directories automatically, making it the most straightforward option.

1. Open **Qt Creator**.
2. Select **File > Open File or Project...** and open the top-level [CMakeLists.txt](file:///D:/GitHub/Sentinel/CMakeLists.txt) file.
3. Select your Qt 6.x desktop kit (e.g., `Desktop Qt 6.11.1 MinGW 64-bit` on Windows, or `Desktop Qt 6.x.x Clang` on macOS).
4. Click **Configure Project**.
5. Click the green **Run (Play)** button in the bottom-left corner to build and launch the application.

---

### Option B: Command Line (CLI)

#### 1. Linux & macOS

If Ninja is installed and Qt is discoverable in your system path:
```bash
cmake -S . -B build -G Ninja
cmake --build build
```

If Qt is not automatically found, pass your `CMAKE_PREFIX_PATH`:
```bash
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/gcc_64
cmake --build build
```
*Common path examples:*
- macOS (Homebrew): `-DCMAKE_PREFIX_PATH=/opt/homebrew/opt/qt`
- Linux (Qt Installer default): `-DCMAKE_PREFIX_PATH=$HOME/Qt/6.7.0/gcc_64`

To run:
- **Linux:** `./build/apps/sentinel-desktop/sentinel-desktop`
- **macOS:** `./build/apps/sentinel-desktop/sentinel-desktop.app/Contents/MacOS/sentinel-desktop`

#### 2. Windows

On Windows, you can compile using **MinGW** (typically bundled with the Qt Installer) or **MSVC**.

**Building with MinGW (Without Ninja):**
If you do not have Ninja installed in your path, use the MinGW generator. Open PowerShell and run:
```powershell
# 1. Add MinGW bin folder to PATH for this session (adjust to match your compiler version/location)
$env:PATH += ";C:\Qt\Tools\mingw1310_64\bin"

# 2. Configure using MinGW generator and pointing to your Qt library
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="C:\Qt\6.11.1\mingw_64"

# 3. Build project
cmake --build build
```

**Building with Ninja / Preset-based builds:**
If you have Ninja installed, configure and compile using standard CMake presets:
```cmd
cmake --preset debug
cmake --build --preset debug
```

To run:
```cmd
# Standard build directory:
build\apps\sentinel-desktop\sentinel-desktop.exe

# Preset-based build directory (e.g. debug):
build\debug\apps\sentinel-desktop\sentinel-desktop.exe
```

---

## Local AI Setup (Ollama / LM Studio)

Sentinel executes AI models locally via loopback connections to protect privacy and support offline usage.

### 1. Ollama (Default Provider)
1. Download and install [Ollama](https://ollama.com).
2. Start the Ollama application.
3. Download a local LLM from your command prompt/terminal (e.g., `llama3.2` or `qwen2.5`):
   ```bash
   ollama pull llama3.2
   ```
4. Launch Sentinel. It will automatically detect Ollama and your downloaded model.

### 2. LM Studio / llama.cpp
Alternatively, you can configure Sentinel to route requests to LM Studio or custom llama.cpp servers in the Workspace & Model Settings within the application.


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

For local manual release builds:

```bash
cmake --preset release
cmake --build --preset release --target sentinel-desktop
```

Package-ready local validation builds can set an explicit build number:

```bash
cmake --preset package-ready -DSENTINEL_BUILD_NUMBER=52
cmake --build --preset package-ready
```

### CI/CD Automated Releases
 
GitHub Actions automatically builds and packages native binaries for all three major platforms on tag pushes (e.g., `v*`) or manual triggers via the **Release Build** workflow (`.github/workflows/release.yml`):
- **Windows (x64):** Packages a portable `.zip` file, and native `.exe` and `.msi` installers.
- **macOS (Apple Silicon ARM64):** Packages a native `.dmg` Disk Image containing a self-contained `.app` bundle.
- **Linux (x64):** Packages a portable `.tar.gz` archive, native `.deb` and `.rpm` (Fedora) packages, and a self-contained `.AppImage`.
 
These packages are automatically uploaded and published to the GitHub Releases page upon successful completion of the workflow.
 
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

## License

Sentinel is open-source software licensed under the [GNU General Public License v3.0 (GPLv3)](LICENSE).
