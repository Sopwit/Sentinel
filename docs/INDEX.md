# Sentinel Documentation Index & Sitemap

Welcome to the central documentation index for **Sentinel** (Qt 6 / C++20 Desktop AI Companion).

---

## 🗺️ Documentation Directory Map

```
docs/
├── INDEX.md                     # Master navigation index and sitemap (this file)
│
├── user-guide/                  # Son Kullanıcı Dokümantasyonu (User Guide)
│   ├── README.md                # User Guide Overview
│   ├── installation.md          # Platform installation (Fedora, Ubuntu, macOS, Windows)
│   ├── local-ai-setup.md        # Ollama, LM Studio & Local Model Configuration
│   ├── features.md              # Workspaces, Local RAG, Command Palette & Themes
│   ├── shortcuts.md             # Complete Keyboard Shortcut Reference
│   └── troubleshooting.md      # FAQ & Connection / Display Troubleshooting
│
├── dev/                         # Geliştirici Dokümantasyonu (Developer Portal)
│   ├── README.md                # Developer Portal Overview
│   ├── setup.md                 # IDE & Toolchain Environment Setup (CLion, VS Code, Qt Creator)
│   ├── building.md              # CMake Presets & Build Commands
│   ├── testing.md               # CTest, Unit Testing & QA Workflows
│   ├── code-style.md            # C++20 and QML Code Standards (.clang-format)
│   └── ci-cd.md                 # GitHub Actions Pipelines & Automation
│
├── architecture/                # Mimari ve Tasarım Dokümantasyonu (Architecture)
│   ├── overview.md              # High-Level Architecture & Monolithic Monolith Overview
│   ├── core-components.md       # Provider, Store, Memory, Task Planner Subsystems
│   ├── data-persistence.md      # SQLite vs JSON Storage Architecture
│   ├── security-boundaries.md   # Explicit Authority, Tool Boundaries & Sandbox Security
│   ├── ui-ux-design.md          # QML Shell, Liquid Glass Theme & UI System
│   ├── opencode-feature-integration.md # OpenCode Analizi & Ajan/Bellek Entegrasyon Planı
│   └── decisions/               # Architecture Decision Records (ADR)
│       └── index.md             # ADR Summary & Decision Log Index
│
├── api/                         # API ve Arayüz Referansları (API References)
│   ├── cpp-interfaces.md        # C++ Core Interfaces (IChatProvider, IMemoryStore, ISettingsStore)
│   ├── view-models.md           # QML View-Models & Q_PROPERTY Presentation API
│   └── platform-services.md     # Platform Services (IPlatformService, IPathProvider, INotificationService)
│
├── release/                     # Release ve Paketleme Dokümantasyonu (Release & QA)
│   ├── release-checklist.md     # Pre-Distribution Release Checklist
│   ├── packaging.md             # AppImage, RPM, DEB, DMG & Windows MSI Packaging Guide
│   ├── qa-plan.md               # Quality Assurance Framework & Testing Matrix
│   ├── ui-qa-checklist.md       # Visual Styling & Interaction Verification Checklist
│   └── release-notes/           # Version Release Notes
│       ├── v1.0.0-rc1.md        # Sentinel 1.0.0-rc.1 Release Notes
│       └── v1.0.0-rc7.md        # Sentinel 1.0.0-rc.7 Release Notes
│
└── archive/                     # Arşiv ve Faz Geçmişi (Archive & Phase History)
    ├── ROADMAP.md               # Complete Project Roadmap
    ├── PHASE_STATUS.md          # Completed Phase Log & Status History
    ├── phases/                  # Phase Checkpoint Reports (phase-04.md through phase-18.md)
    └── legacy/                  # Legacy Monolithic Design Specs
```

---

## 🚀 Quick Links

- **New Users**: Start with the **[Installation Guide](file:///Volumes/NeuralSilicon/DEV/Personal/Sentinel/docs/user-guide/installation.md)** and **[Local AI Setup](file:///Volumes/NeuralSilicon/DEV/Personal/Sentinel/docs/user-guide/local-ai-setup.md)**.
- **Developers**: Read **[Developer Setup](file:///Volumes/NeuralSilicon/DEV/Personal/Sentinel/docs/dev/setup.md)** and **[Building Sentinel](file:///Volumes/NeuralSilicon/DEV/Personal/Sentinel/docs/dev/building.md)**.
- **Architects & Contributors**: Check **[Architecture Overview](file:///Volumes/NeuralSilicon/DEV/Personal/Sentinel/docs/architecture/overview.md)** and **[C++ Core Interfaces](file:///Volumes/NeuralSilicon/DEV/Personal/Sentinel/docs/api/cpp-interfaces.md)**.
- **Release Engineers**: Consult the **[Release Checklist](file:///Volumes/NeuralSilicon/DEV/Personal/Sentinel/docs/release/release-checklist.md)** and **[Packaging Guide](file:///Volumes/NeuralSilicon/DEV/Personal/Sentinel/docs/release/packaging.md)**.
