# Security

Sentinel defaults to explicit user authority.

## Phase 50B Boundary

- No telemetry.
- No hidden cloud calls.
- No autonomous agents.
- No folder import or recursive filesystem scanning.
- No background document processing.
- No automatic embedding generation.
- No automatic knowledge-base activation.

File chat and Local RAG accept only explicit foreground user actions. Local RAG metadata is
workspace-scoped and stored in SQLite through Qt SQL.

## Execution Separation

Workspace, attachment, and retrieval metadata do not enable tool execution, subprocess execution,
filesystem mutation, cloud retrieval, or agent autonomy. Local Ollama foreground chat remains the
only real provider execution path from Phase 50A.
