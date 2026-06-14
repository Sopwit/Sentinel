# Workspaces

Phase 50B makes Sentinel workspaces real local product state.

## Built-In Templates

- Personal
- Coding
- Research
- Writing
- Student

## Lifecycle

Workspaces can be created, renamed, archived, deleted, and duplicated through the
`DesktopShellViewModel` boundary. Built-in templates remain stable defaults; user-created workspace
records are persisted in settings as a compact catalog.

## Isolation

The active workspace scopes:

- selected models and model-role preferences
- routing-role preferences
- context settings
- notification settings
- Local RAG settings
- export preferences
- attached document metadata
- Brain workspace summaries

Chat history, key-value memory, settings, and Local RAG storage remain separate persistence areas.

## Filesystem Boundary

Workspaces do not grant folder access. Sentinel does not scan workspace folders, import directories,
watch files, or process documents in the background. Documents enter a workspace only when the user
explicitly attaches or adds one supported file.
