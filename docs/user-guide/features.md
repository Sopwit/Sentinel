# Sentinel Features & Workspaces

Sentinel provides a modular suite of companion tools designed to streamline developer workflows and daily desktop usage.

---

## 1. Workspaces & Isolated Contexts

Sentinel organizes project contexts into **Workspaces**. Each workspace isolates:
- **Local RAG Documents**: Files, code repositories, or reference notes attached to the specific project.
- **Memory Store Keys**: Project-specific parameters, workspace configurations, and key-value memories.
- **Controlled Task History**: History of multi-step task plans and explicit operator consents.

---

## 2. Command Palette (`Ctrl+K` / `Cmd+K`)

Press `Ctrl+K` (or `Cmd+K` on macOS) anywhere within Sentinel to bring up the unified **Command Palette**:
- Instantly switch active Workspaces.
- Jump to Settings, Memory Manager, or Provider Health.
- Trigger diagnostics exports or theme toggle.
- Execute quick navigational actions without leaving the keyboard.

---

## 3. Local RAG (Retrieval-Augmented Generation)

Sentinel allows attaching local folders and documents for contextual retrieval:
- **Local SQLite Indexing**: Metadata and vector indexes are stored locally in `local_rag.sqlite3`.
- **Explicit Ingestion**: Sentinel only indexes files you explicitly add to a workspace.
- **Zero Cloud Upload**: Document text remains strictly local on your filesystem.

---

## 4. Theme & UI Styling (Liquid Glass)

Sentinel features a custom **Transparent / Matte Liquid Glass** theme:
- Clean modern typography using system fonts and smooth rendering.
- Tailored color palette tuned for Fedora KDE Plasma dark and light themes.
- Micro-animations for high-responsiveness and visual feedback.
