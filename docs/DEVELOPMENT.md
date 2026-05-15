# Development

Sentinel uses CMake presets to keep local and CI builds consistent.

## Requirements

- CMake 3.24 or newer.
- Ninja.
- C++20 compiler.
- Qt 6.5 or newer with `Core`, `Gui`, `Quick`, `Qml`, `Sql`, and `Test`.

## Configure And Build

Debug build:

```bash
cmake --preset debug
cmake --build --preset debug
```

Release build:

```bash
cmake --preset release
cmake --build --preset release
```

Test-focused build:

```bash
cmake --preset tests
cmake --build --preset tests
ctest --preset tests
```

If your environment wraps the compiler with `ccache` and that causes local permission issues, use:

```bash
cmake --preset no-ccache
cmake --build --preset no-ccache
ctest --preset no-ccache
```

## Qt Path Issues

If CMake cannot find Qt, pass `CMAKE_PREFIX_PATH` when configuring:

```bash
cmake --preset debug -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/gcc_64
```

Common examples:

```bash
cmake --preset debug -DCMAKE_PREFIX_PATH=$HOME/Qt/6.7.0/gcc_64
cmake --preset debug -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/qt
```

## Formatting

Format C++ source files with `clang-format`:

```bash
find apps core tests -type f \( -name '*.cpp' -o -name '*.h' \) -print0 \
  | xargs -0 clang-format -i
```

Check formatting without writing changes:

```bash
find apps core tests -type f \( -name '*.cpp' -o -name '*.h' \) -print0 \
  | xargs -0 clang-format --dry-run --Werror
```

## Static Analysis

`.clang-tidy` is provided as an optional baseline. It intentionally avoids high-noise checks that do not fit Qt/QObject/QML-heavy code well, including broad identifier naming and magic-number checks. Qt APIs, QML dimensions, generated MOC code, and signal/slot naming patterns make those checks noisy before they are useful.

Run clang-tidy after generating `compile_commands.json`:

```bash
cmake --preset debug
clang-tidy core/src/*.cpp apps/sentinel-desktop/*.cpp tests/core/*.cpp -p build/debug
```

Treat clang-tidy output as advisory for now. CI currently enforces build, tests, and formatting only.

## IDE Problem Triage

If CLion, VS Code, clangd, or Qt Creator shows hundreds or thousands of problems, first check whether it is indexing generated files. The canonical source folders are:

- `apps/`
- `core/`
- `tests/`
- `ui/`
- `integrations/`
- `plugins/`
- `docs/`

Exclude these generated or local-only folders from IDE indexing and static analysis:

- `build/`
- `build-*/`
- `build-tests/`
- `build-noccache/`
- `cmake-build-*/`
- `.cache/`
- `.qt/`
- Qt autogen folders such as `*_autogen/`
- Qt qmlcache folders such as `.rcc/qmlcache/`

For clangd, this repository includes `.clangd` pointing at `build/no-ccache`. Reconfigure that preset before relying on IDE diagnostics:

```bash
cmake --preset no-ccache
```

QML linting should run against the built QML module, not arbitrary generated qmlcache C++ files:

```bash
cmake --build --preset no-ccache
qmllint -I build/no-ccache/apps/sentinel-desktop \
  build/no-ccache/apps/sentinel-desktop/Sentinel/Desktop/*.qml
```

The remaining known QML lint warning is the root `shellViewModel` context property injected from C++. It is harmless at runtime and intentionally left until a typed QML singleton is worth adding.

## Canonical Verification

Before reporting source problems, run:

```bash
cmake --preset no-ccache
cmake --build --preset no-ccache
ctest --preset no-ccache
find apps core tests -type f \( -name '*.cpp' -o -name '*.h' \) -print0 \
  | xargs -0 clang-format --dry-run --Werror
```

## CI Expectations

Linux CI installs Qt, configures with the `tests` preset, builds with Ninja, runs CTest, and checks formatting.

## Desktop Shell Boundary

QML should bind to `DesktopShellViewModel`, not directly to core services. Keep business rules in C++ and expose only small `Q_PROPERTY` and `Q_INVOKABLE` surfaces needed by the UI.

Settings placeholders should go through `AppSettings`. Memory storage should go through `IMemoryStore`. Do not add provider networking or platform automation in the desktop shell layer.

Chat UI should consume `ChatMessageListModel` roles instead of formatting message history in QML. Keep chat mutation in `ApplicationController` and `ChatSession`.

## Memory Storage

Sentinel currently has two memory backends:

- `InMemoryStore`: volatile backend used by unit tests and simple development fixtures.
- `SQLiteMemoryStore`: persistent desktop backend implementing the same `IMemoryStore` key-value contract.

The desktop app wires `SQLiteMemoryStore` to Qt's `AppDataLocation` as `memory.sqlite3`. Keep settings in `JsonSettingsStore`; do not mix settings and app memory in the same file or table.

SQLite stores only explicit key-value memory entries. Chat history remains in-memory until a separate persistence design is added.

`SQLiteMemoryStore` creates a schema metadata table and records schema version `1`. Future migrations should build on that metadata table, but no migration framework should be added until there is a real schema change.

If the SQLite database path is unavailable or unwritable, the store reports unavailable and safely behaves as an empty/no-op backend. This avoids noisy runtime logs and keeps the desktop shell usable.

## Linux Packaging Direction

Fedora/KDE packaging should stay conventional:

- Install the `sentinel-desktop` binary.
- Install `packaging/linux/dev.sentinel.Sentinel.desktop.in` as a desktop entry after replacing placeholders if needed.
- Install `resources/icons/dev.sentinel.Sentinel.svg` into the icon theme path.
- Keep settings under Qt's `AppConfigLocation`.

Do not add privileged services, autostart agents, or system automation until those phases are explicitly designed.
