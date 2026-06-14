# Local RAG

Phase 50B adds an optional workspace-scoped Local Knowledge Base.

## Defaults

- Knowledge Base: disabled
- Indexing: manual only
- Document scope: workspace only
- Cloud retrieval: disabled
- Embedding generation: not automatic

## Storage

Local RAG metadata is stored in SQLite through Qt SQL at:

`QStandardPaths::AppDataLocation + "/local_rag.sqlite3"`

The store records document metadata, manual index status, and recent retrieval explainability
metadata. It does not store cloud provider state and does not call embedding services.

## Document Lifecycle

Users can explicitly:

- add a supported document
- remove a document
- re-index manually
- clear the workspace knowledge base
- view document status

Folder import, recursive scanning, file watching, background processing, automatic embeddings, and
automatic knowledge-base activation are disabled.

## Retrieval Explainability

When visible, retrieval summaries include:

- source document
- section/chunk reference
- relevance metadata

Explainability can be disabled from workspace settings. Retrieval metadata is advisory and does not
grant automatic prompt authority.
