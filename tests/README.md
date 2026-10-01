# Tests

Sentinel uses QtTest executables registered with CTest. Core tests are under `tests/core`; GUI/QML tests and benchmarks are also configured from this tree. The source and target list in `tests/CMakeLists.txt` is authoritative.

Run the suite with:

```bash
cmake --preset tests
cmake --build --preset tests
ctest --preset tests --output-on-failure
```

For a focused target, use `ctest --test-dir build/tests -R <name> --output-on-failure`. See the canonical [testing guide](../docs/development/TESTING.md).
