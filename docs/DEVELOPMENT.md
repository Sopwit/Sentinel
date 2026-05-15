# Development

Sentinel uses CMake presets to keep local and CI builds consistent.

## Requirements

- CMake 3.24 or newer.
- Ninja.
- C++20 compiler.
- Qt 6.5 or newer with `Core`, `Gui`, `Quick`, `Qml`, and `Test`.

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

`.clang-tidy` is provided as an optional baseline. Run it after generating `compile_commands.json`:

```bash
cmake --preset debug
clang-tidy core/src/*.cpp apps/sentinel-desktop/*.cpp tests/core/*.cpp -p build/debug
```

Treat clang-tidy output as advisory for now. CI currently enforces build, tests, and formatting only.

## CI Expectations

Linux CI installs Qt, configures with the `tests` preset, builds with Ninja, runs CTest, and checks formatting.

## Desktop Shell Boundary

QML should bind to `DesktopShellViewModel`, not directly to core services. Keep business rules in C++ and expose only small `Q_PROPERTY` and `Q_INVOKABLE` surfaces needed by the UI.

Settings placeholders should go through `AppSettings`. Storage placeholders should go through `IMemoryStore`. Do not add SQLite, provider networking, or platform automation in the desktop shell layer.
