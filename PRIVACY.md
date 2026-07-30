# Privacy Policy

Sentinel is a local-first, privacy-first AI companion desktop application built with Qt 6 and C++20.

## Core Privacy Principles

1. **Zero Telemetry**: Sentinel collects no analytics, usage statistics, tracking metrics, or crash reports.
2. **Local Storage Only**: All conversation transcripts, memory stores, application settings, and Local RAG metadata reside exclusively on your local machine.
3. **No Hidden Cloud Calls**: Sentinel makes no unauthorized network calls, silent background updates, or automatic telemetry reporting.
4. **Explicit User Authority**: Any external model connection (e.g. local Ollama runtime) or update check requires explicit user initiation and configuration.

## Data Storage Architecture

Sentinel segregates local application state into distinct storage boundaries:

- **Settings**: `QStandardPaths::AppConfigLocation + "/settings.json"`  
  Stores local UI settings, selected theme, active model selection, onboarding state, and accessibility options.
- **Key-Value Memory**: `QStandardPaths::AppDataLocation + "/memory.sqlite3"`  
  Stores user-approved memory keys and agent context.
- **Chat Transcripts**: `QStandardPaths::AppDataLocation + "/chat_history.sqlite3"`  
  Stores local conversation transcripts.
- **Local RAG Index**: `QStandardPaths::AppDataLocation + "/local_rag.sqlite3"`  
  Stores user-selected workspace document metadata and vector/keyword index tables.
- **Diagnostics & Exports**: `QStandardPaths::AppDataLocation + "/exports"`  
  Stores user-initiated export archives.

## Workspace & Document Privacy

- **Knowledge Base Scope**: Document indexing and retrieval are strictly scoped to the active workspace.
- **Explicit Ingestion Only**: Sentinel does not automatically scan folders, watch directories in the background, or import files without explicit user action.
- **Local Provider Isolation**: Retrieval and document processing remain local. No document content is sent to third-party endpoints unless explicitly configured by the user.

## Updates & Network Isolation

- Sentinel release packages do not include automatic background updaters.
- Manual update checks only query static version metadata when initiated by the user.
