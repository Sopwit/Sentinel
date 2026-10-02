# Testing

Tests are QtTest-based executables registered with CTest; most are under `tests/core`, with QML/UI coverage also configured by the test CMake tree. Benchmarks live under `tests/benchmarks` and are not a substitute for correctness tests.

```bash
cmake --preset tests
cmake --build --preset tests
ctest --preset tests --output-on-failure
```

For a focused test, use `ctest --test-dir build/tests -R <name> --output-on-failure`. GUI CI uses `QT_QPA_PLATFORM=offscreen`. Linux CI also checks formatting, QML lint, and desktop metadata; Linux ASan uses the `asan` preset. Verify a changed Desktop flow manually for loading, empty, failure, focus, and responsive behavior.
