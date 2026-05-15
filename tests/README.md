# Tests

Sentinel uses Qt Test for isolated C++ core tests.

Current test targets:

- `test_mode_manager`
- `test_in_memory_store`
- `test_application_controller`

Run from the repository root:

```bash
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

The tests link against `sentinel_core` and do not launch the QML desktop UI.

See `docs/TESTING.md` for the full testing workflow.
