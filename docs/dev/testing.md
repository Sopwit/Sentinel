# Testing Guidelines & QA Workflow

Sentinel relies on QtTest and CTest to ensure high reliability across core interfaces, stores, and view models.

---

## 1. Running Tests

### Preferred Preset Workflow
```bash
cmake --preset tests
cmake --build --preset tests
ctest --preset tests
```

### Direct Test Execution with Verbose Output
```bash
ctest --test-dir build --output-on-failure
```

---

## 2. Test Suite Organization

Tests are organized under the `tests/` directory:

- `tests/unit/core/`: Unit tests for C++ core services, memory store, settings store, provider interface, and task planner.
- `tests/unit/ui/`: Unit tests for QML view models, property bindings, and state transitions.
- `tests/integration/`: Integration tests for SQLite schema migration, settings serialization, and Ollama endpoint discovery.

---

## 3. Writing Unit Tests

1. Every core service or view model feature must be accompanied by unit tests.
2. Use QtTest framework macros (`QVERIFY`, `QCOMPARE`, `QSIGNALSPY`).
3. Isolated test databases should use in-memory SQLite (`:memory:`) or temporary scratch paths to prevent polluting user application state.
