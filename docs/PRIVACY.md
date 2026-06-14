# Privacy

Sentinel is local-first and privacy-first.

## Phase 52 Release Guarantees

- Build metadata shown in About and Diagnostics is local static metadata.
- Diagnostics export remains user-initiated and local.
- Packaging metadata does not add telemetry, analytics upload, hidden cloud activation, hidden
  update checks, or package-manager calls.
- Future update channels must be explicit, signed, user-visible, and separately reviewed before
  network behavior is enabled.

## Phase 51 Guarantees

- Onboarding, theme, AI provider choice, accessibility, notification lifecycle, update workflow
  state, and recovery draft state are stored locally in settings.
- Manual update checks do not contact an update server in this build.
- Release notes, About, Diagnostics Center, Brain Insights, and export preview are local UI
  summaries.
- Diagnostics export is user-initiated and writes to the local app-controlled export directory.
- No telemetry, analytics upload, hidden cloud activation, hidden uploads, hidden indexing,
  automatic downloads, silent updates, or background polling are added.

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
- Diagnostics and data exports: `QStandardPaths::AppDataLocation + "/exports"`
- Controlled task history and workspace permissions: settings JSON

These stores remain separate. Local RAG document and retrieval metadata is isolated by workspace id.
Phase 51 notification, onboarding, accessibility, recovery, and update workflow state stays in
settings JSON and does not alter memory, chat history, or Local RAG storage.
