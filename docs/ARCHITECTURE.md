# System Architecture & Technical Specification

**Sentinel** is architected as a native, modular C++20 / Qt 6 desktop companion and AI operating layer, prioritizing privacy, low resource footprint, explainability, and explicit user authority.

---

## 1. Subsystem Architecture

```
┌────────────────────────────────────────────────────────────────────────┐
│                        User Presentation Layer                         │
├────────────────────────────────┬───────────────────────────────────────┤
│    Liquid Glass Desktop Shell  │             Headless CLI              │
│       (Qt6 / QML / QtQuick)    │        (sentinel-cli dispatch)        │
└───────────────┬────────────────┴───────────────────┬───────────────────┘
                │                                    │
                ▼                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│                Presentation & View-Model Coordination                  │
├────────────────────────────────────────────────────────────────────────┤
│ • DesktopShellViewModel        • ApplicationController                 │
│ • ChatSessionViewModel         • WorkspaceViewModel                    │
│ • TaskPlannerViewModel         • SettingsViewModel                     │
└───────────────┬────────────────────────────────────┬───────────────────┘
                │                                    │
                ▼                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│                         C++20 Core Subsystems                          │
├────────────────────────────────────────────────────────────────────────┤
│ • AgentLoop & Runtime          • TaskPlanner & ApprovalPolicy          │
│ • RealToolExecutor             • LocalRagStore & Embeddings            │
│ • ModelRouter & ProviderCatalog• MemoryManager & ContextAssembly       │
│ • FuzzyEditor & DiffEngine     • PlatformServices (Audio/Tray/Sys)     │
└───────────────┬────────────────────────────────────┬───────────────────┘
                │                                    │
                ▼                                    ▼
┌─────────────────────────────────┐ ┌────────────────────────────────────┐
│      Persistence Layer          │ │         AI Execution Engine        │
├─────────────────────────────────┤ ├────────────────────────────────────┤
│ • memory.sqlite3 (Taxonomy)     │ │ • Ollama (127.0.0.1:11434)         │
│ • chat_history.sqlite3 (Threads)│ │ • LM Studio / llama.cpp loopback   │
│ • local_rag.sqlite3 (Vectors)   │ │ • Explicit Cloud Provider APIs     │
│ • settings.json (Preferences)   │ │ • Local Whisper Audio Inference    │
└─────────────────────────────────┘ └────────────────────────────────────┘
```

---

## 2. Core Subsystems

### A. AI Execution & Model Router
- **Abstract Provider Interface (`IChatProvider`):** Standardizes streaming text tokens, model discovery, and health inspection.
- **Ollama Loopback Provider:** Communicates over local HTTP loopback (`127.0.0.1:11434`) for high-throughput zero-telemetry streaming.
- **Model Router:** Dynamically routes queries between fast reasoners, coding LLMs, and compact models based on workspace requirements and task tags.

### B. Workspace Isolation & Storage Separation
- **Strict Scope Isolation:** Memory scopes, conversational history, and document embeddings are strictly separated by `workspace_id` (`Personal`, `Engineering`, `Student`, `Custom`).
- **Dedicated SQLite Databases:**
  - `memory.sqlite3`: Semantic key-value memory records, user preferences, and project goals.
  - `chat_history.sqlite3`: Conversation threads, message roles, tokens, and timestamps.
  - `local_rag.sqlite3`: Chunked document vectors, full-text search indexes (FTS5), and document metadata.
  - `settings.json`: Lightweight UI configurations and active workspace state.

### C. Explainable Local RAG Engine
- **Supported Formats:** Offline ingestion of PDF, Markdown, TXT, CSV, JSON, and source code files.
- **Context Budget Allocator:** Dynamically scores and compresses retrieved knowledge to fit within the model's active context window.
- **Retrieval Explainability:** Exposes exact source references, similarity scores, and document chunks in the UI inspector panel.

### D. Controlled Foreground Agent Workflows
- **Autonomous Agent Loop (`AgentLoop` + `LlmAgentRuntime`):** Multi-step task reasoning engine.
- **Explicit Human Approval Gate:** Every destructive or privileged tool execution (file modification, shell command, workspace deletion) halts for explicit user approval unless explicitly overridden.
- **Tool Sandbox & Isolation:** Built-in workspace boundaries prevent tool execution outside the authorized project root directory.

### E. Native Liquid Glass UI System
- **Qt 6 / QML Architecture:** Hardware-accelerated rendering utilizing Qt Quick Scene Graph.
- **Liquid Glass Aesthetic:** Matte translucent glass panels, subtle backdrop blur, and high-contrast typography tailored for KDE Plasma, macOS, and Windows.
- **Themes:** Supports Liquid Glass Light (default), Liquid Glass Dark, Sentinel Classic, Midnight Blue, and Aurora Teal.

---

## 3. Security, Privacy & Consent Model

Sentinel strictly implements zero-trust privacy boundaries:

1. **No Telemetry:** No analytics, tracking pixels, or diagnostic phone-homes.
2. **No Silent Background Actions:** No automatic background indexing or unexpected network requests.
3. **Explicit API Keys:** Cloud models require explicit user configuration; keys are stored in secure local credential stores (Apple Keychain / Windows Credential Manager / Secret Service).
4. **Tool Isolation:** Workspace boundary enforcement prevents tools from accessing parent directories or unauthorized filesystem paths.

---

## 4. Multi-Client & IPC Architecture

- **`sentinel-desktop`:** Primary native graphical shell with tray integration (`QSystemTrayIcon`), global hotkey handling (`Ctrl/Cmd+K`), and live visual monitors.
- **`sentinel-cli`:** Headless command dispatcher for CLI automation, scripted queries, and pipeline integration.
- **`sentinel-daemon`:** Background coordination service managing background jobs, notification dispatch, and inter-process session state.
