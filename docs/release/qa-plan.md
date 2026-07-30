# QA & Quality Assurance Plan

This document outlines Sentinel's quality assurance framework across automated tests, UI regression verification, and platform sanity checks.

---

## 1. Automated Testing Layers

- **Unit Tests**: Test core components, data parsing, memory stores, settings serialization, and view models.
- **Integration Tests**: Verify database migrations, SQLite transaction safety, and Ollama endpoint discovery.
- **Static Analysis**: Enforce `.clang-format` rules, `.qmllint.ini` syntax checks, and compiler warning hygiene (`-Wall -Wextra -Werror`).

---

## 2. Platform Regression Matrices

| Platform | Target OS Version | Core Focus Area |
| :--- | :--- | :--- |
| **Fedora KDE** | Fedora 38+ (KDE 5.27 / Plasma 6) | Glass visual styling, system tray, dark mode |
| **Ubuntu** | Ubuntu 22.04 LTS / 24.04 LTS | Desktop launcher integration, AppImage compatibility |
| **macOS** | macOS 13+ (Ventura / Sonoma / Sequoia) | Native menu bar, Apple Silicon ARM64 performance |
| **Windows** | Windows 10 / 11 64-bit | Path resolution, HiDPI scaling, MSVC binary stability |
