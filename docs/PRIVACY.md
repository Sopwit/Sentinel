# Privacy

Sentinel is local-first and privacy-first.

## Phase 50C Guarantees

- Controlled task history is stored locally.
- Workspace task histories, approvals, and permission choices are isolated by workspace id.
- Task planning does not call cloud services.
- Step execution is visible metadata progression only.
- No telemetry, hidden provider calls, hidden tool use, clipboard access, terminal execution,
  browser automation, calendar/email access, downloads, or filesystem scanning is added.

## Phase 50B Guarantees

- Knowledge Base is disabled by default.
- Indexing is manual only.
- Document scope is workspace only.
- Cloud retrieval is disabled.
- Telemetry is disabled.
- Hidden filesystem scanning is disabled.

Attachments are user-selected files or pasted text only. Sentinel does not import folders, watch
directories, process documents in the background, generate embeddings automatically, or activate a
knowledge base without user opt-in.

## Storage Separation

- Settings: `QStandardPaths::AppConfigLocation + "/settings.json"`
- Memory: `QStandardPaths::AppDataLocation + "/memory.sqlite3"`
- Chat history: `QStandardPaths::AppDataLocation + "/chat_history.sqlite3"`
- Local RAG: `QStandardPaths::AppDataLocation + "/local_rag.sqlite3"`
- Controlled task history and workspace permissions: settings JSON

These stores remain separate. Local RAG document and retrieval metadata is isolated by workspace id.
