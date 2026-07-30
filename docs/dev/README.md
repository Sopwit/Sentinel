# Sentinel Developer Portal

Welcome to the Sentinel Developer Portal. This documentation section covers environment setup, compilation workflows, testing guidelines, code formatting standards, and CI/CD pipelines.

## Navigation

- **[Environment Setup](file:///Volumes/NeuralSilicon/DEV/Personal/Sentinel/docs/dev/setup.md)**: Setting up your IDE (CLion, VS Code, Qt Creator) and build environment.
- **[Building Sentinel](file:///Volumes/NeuralSilicon/DEV/Personal/Sentinel/docs/dev/building.md)**: CMake presets, Ninja build commands, and target options.
- **[Testing & QA Workflow](file:///Volumes/NeuralSilicon/DEV/Personal/Sentinel/docs/dev/testing.md)**: CTest execution, unit test writing, and test suite layout.
- **[Code Style Guidelines](file:///Volumes/NeuralSilicon/DEV/Personal/Sentinel/docs/dev/code-style.md)**: C++20 formatting, QML structure rules, and MVVM presentation boundaries.
- **[CI/CD & Automation](file:///Volumes/NeuralSilicon/DEV/Personal/Sentinel/docs/dev/ci-cd.md)**: GitHub Actions integration, automated testing presets, and release builds.

---

## Architectural Principles for Developers

1. **Modular Monolith**: Core logic, stores, providers, and presentation are separated into clean, modular C++ targets.
2. **C++ Core / QML Presentation Boundary**: Keep business logic out of QML. Expose QML-safe view models inheriting from `QObject`.
3. **Interface Abstractions**:
   - `IChatProvider`: Model provider abstraction (e.g. Ollama health/discovery).
   - `IMemoryStore`: Key-value memory persistence abstraction.
   - `ISettingsStore`: App configuration store abstraction.
   - `IPlatformService`: Operating system integration abstractions.
4. **Strict Separation of Storage**: Never overload memory or settings stores with chat transcripts. Keep SQLite database paths distinct.
