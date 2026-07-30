# Code Style Guidelines

Sentinel strictly enforces modern C++20 standards and clean QML separation.

---

## 1. C++ Coding Standards

- **Standard**: Modern C++20 (`std::optional`, `std::variant`, `std::string_view`, concepts).
- **Formatting**: ClangFormat standard configuration (see `.clang-format` in repository root).
- **Naming Conventions**:
  - Classes & Interfaces: `PascalCase` (`IChatProvider`, `MemoryStore`, `MainViewModel`).
  - Interfaces: Prefix with `I` (`ISettingsStore`, `IPlatformService`).
  - Methods & Functions: `camelCase` (`getSetting()`, `loadTranscripts()`).
  - Private Member Variables: Prefix with `m_` (`m_settingsStore`, `m_provider`).
  - Constants: `ALL_CAPS` or `kCamelCase`.

---

## 2. QML Coding & MVVM Boundaries

- **No Business Logic in QML**: QML files handle layout, styling, animations, and user interaction signals only.
- **View Models**: Expose C++ view models to QML via `Q_PROPERTY` with `READ`, `WRITE`, and `NOTIFY` signals.
- **QML Formatting**: Follow `.qmllint.ini` guidelines. Group properties, signals, function handlers, and child components logically.
