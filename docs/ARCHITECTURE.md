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
- **Agent Runtime Boundary:** `IAgentRuntime` exposes in-process sessions, submission,
  approval/resume, cancellation, state, and runtime errors. `AgentRuntime` owns session
  lifecycle, its foreground worker, tool snapshots, bounded subagent runner, and controlled
  task execution. It delegates multi-step work to the existing `AgentLoop`. Desktop chat
  presentation remains in `ApplicationController`; the runtime can be constructed without QML.
  Runtime consumers subscribe to typed `AgentEvent` values. Each accepted run has one turn ID;
  step and tool call IDs remain stable across approval pauses. A session's events are queued in
  order, with tool completion before step completion and exactly one terminal agent event.
  Event history is in memory and capped at 256 events per session. Ollama planner requests use
  the existing local stream client and emit real model deltas; providers with only a final-reply
  API emit request start and completion without synthetic deltas.
  Desktop presentation uses the terminal and approval event payloads directly; it does not poll
  the runtime to discover live progress.
  The foreground worker has a Qt event loop. `AgentLoop` suspends a process-backed step after
  the gateway approves it, and resumes from the tool completion callback. `RealToolExecutor`
  starts run-command, Docker, and other process-backed tools through `ProcessExecutor`; stdout
  and stderr chunks become correlated `ToolOutput` events while bounded output is retained for
  the final observation. Cancellation terminates the active process before the turn closes.
- **Native Tool Registry:** `AgentRuntime` owns the registry used by planner discovery and
  `ToolExecutionGateway`. `BuiltInToolProvider` registers each native descriptor with an
  executable handler at runtime creation. Immediate handlers call focused native operations;
  process-backed handlers retain the asynchronous `ProcessExecutor` path. An unregistered or
  disabled tool cannot fall through to the synchronous compatibility executor.
  Risk shown in gateway summaries now follows the planner's `ToolRiskLevel`; the former
  gateway-only `Critical` labels for high-risk native tools are no longer authoritative.
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
# MCP tool registration

`McpToolCatalog` maps each discovered definition to a descriptor. `McpToolProvider`
adds a handler and atomically replaces registrations owned by `mcp:<server-name>`
in the agent runtime's production registry. The planner and gateway use that same
registry. MCP tool IDs use `mcp.<server>.<tool>`; each UTF-8 byte outside lowercase
ASCII letters and digits is escaped as `_hh_`, including underscores. This keeps
IDs stable and distinct while the original server and tool names remain in the
handler. Unknown or duplicate names reject a refresh and leave prior entries in
place. Disconnect removes only that server's entries.

Dynamic MCP tools default to medium risk. Their input schemas are retained without
validation. MCP agent calls are asynchronous; cancellation suppresses late results
locally and aborts an HTTP reply or drops a pending stdio response when possible.
The MCP protocol cancellation notification is not implemented. The built-in
`mcp-list` and `mcp-call` tools remain for compatibility.

The gateway checks the MCP `tool-execution` permission domain before invoking
its handler. Disabled blocks execution; Ask Every Time requires explicit approval;
Trusted and Enabled allow it after the ordinary approval and sandbox gates. Tool
hooks run before the handler and after its result (or receive its error). The
stdio integration test uses a local MCP helper to cover the handshake, discovery,
AgentLoop observation, approval, permission and sandbox denial, hooks, runtime
events, reconnect, process failure, and cancellation.

# Tool argument contracts

`ToolDescriptor.inputSchema` is the model-visible and runtime-enforced argument contract.
`BuiltInToolProvider` generates strict schemas from built-in parameter descriptors;
MCP and plugin registrations retain their provider schemas. The registry rejects
malformed structural contracts, and executable plugins require a schema. A missing
MCP schema permits only an empty argument object. Unsupported structural composition
keywords are rejected; unknown descriptive keywords are ignored.

`AgentLoop` prevalidates planned calls before approval. `ToolExecutionGateway` validates
again against the resolved registration snapshot before hooks or handlers, applies
schema defaults, and returns `InvalidArguments` as a recoverable observation. Typed
JSON argument values travel alongside the legacy text form so native handlers,
MCP calls, and plugins receive the same normalized invocation. Path authorization,
shell safety, permissions, and sandbox checks remain separate from schema validation.
