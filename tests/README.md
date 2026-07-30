# Sentinel Automated Test Suite Architecture

Sentinel utilizes QtTest and QtQuickTest frameworks across a 5-tier modular testing hierarchy:

## Test Architecture Layout

```
tests/
├── unit/                 # C++ unit tests (ModeManager, Stores, RAG, Ollama, Runtime)
├── integration/          # End-to-end integration tests (RAG pipeline, ViewModel sync)
├── gui/                  # QtQuickTest QML component & UI navigation tests
├── performance/          # Performance & load verification (Startup time, Memory limits)
└── benchmarks/           # Micro-performance latency benchmarks (QBENCHMARK)
```

## Running Tests

### 1. Run Complete Test Suite
```bash
cmake --preset tests
cmake --build --preset tests
ctest --preset tests
```

### 2. Run Specific Test Subsuites
```bash
# Integration Tests
ctest --test-dir build -R test_rag_e2e_pipeline --output-on-failure

# Benchmarks (Semantic Search & Context Compression Latency)
ctest --test-dir build -R bench_ --output-on-failure

# Cold Startup Performance Verification
ctest --test-dir build -R test_startup_time --output-on-failure
```

