# Data Persistence Architecture

Sentinel maintains strict separation of storage layers to prevent data corruption, preserve privacy, and enable fast SQLite queries.

---

## Storage Locations

All application data is stored locally under system standard paths (`QStandardPaths`):

| Data Store | Storage Engine | Path Location | Description |
| :--- | :--- | :--- | :--- |
| **Settings** | JSON (`QJsonDocument`) | `AppConfigLocation + "/settings.json"` | UI preferences, themes, active provider config |
| **Memory Store** | SQLite (`Qt6::Sql`) | `AppDataLocation + "/memory.sqlite3"` | Key-value memory taxonomy & persistent context |
| **Chat History** | SQLite (`Qt6::Sql`) | `AppDataLocation + "/chat_history.sqlite3"` | Local conversation transcripts & session records |
| **Local RAG** | SQLite (`Qt6::Sql`) | `AppDataLocation + "/local_rag.sqlite3"` | Document metadata & vector index tables |
| **Exports** | Filesystem Archive | `AppDataLocation + "/exports/"` | User-initiated diagnostic and data backups |

---

## Schema Migration & Integrity

- Database initialization uses explicit SQL DDL migrations with `user_version` PRAGMA checks.
- Transactions (`QSqlDatabase::transaction()`) wrap batch memory writes and transcript logging.
- Database access is thread-confined to core worker threads or database service wrappers.
