# Building

Sentinel requires CMake 3.27+ (presets), Ninja, a C++20 compiler, and Qt 6.5+ with Core, Gui, Quick, Qml, Sql, Test, Network, Widgets, LinguistTools, Multimedia, and Concurrent. CI currently installs Qt 6.11 on Linux, macOS, and Windows.

```bash
cmake --preset debug
cmake --build --preset debug
./build/debug/apps/sentinel-desktop/sentinel-desktop
```

Use `cmake --preset tests`, `release`, `relwithdebinfo`, `package-ready`, `asan`, or `coverage` as needed. The `windows-arm64` preset is experimental. Fedora KDE Plasma is the primary desktop target; do not put Linux-only assumptions in core code.

The CMake tree builds `sentinel_core`, plugin SDK/samples, desktop, CLI, daemon, plugin host, and tests when `SENTINEL_BUILD_TESTS` is enabled. Packaging configuration is in `cmake/SentinelCPack.cmake`.

## macOS scoped clang-tidy without production PCH

Apple compiler PCH cannot be consumed by a differently versioned Homebrew clang-tidy. Keep the production compiler/PCH unchanged and generate an analysis-only database:

```bash
python3 tools/analysis/compile_database.py build/tests/compile_commands.json /private/tmp/sentinel-cleanup-analysis
/opt/homebrew/opt/llvm/bin/clang-tidy -p /private/tmp/sentinel-cleanup-analysis apps/sentinel-desktop/src/DesktopRuntimeClient.cpp
```

The helper strips only CMake PCH flags, preserves other compile arguments and resolves macOS SDK/libc++ paths through xcrun. See the deferred cleanup report for the actually completed five-source scope and narrow QObject ownership analyzer exception. This supersedes the previous PCH blocker for that scope, not whole-repository analysis certification.
