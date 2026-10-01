# Data locations

`StandardPathProvider` is authoritative. In normal mode, settings are at Qt `AppConfigLocation`; memory, chat history, conversations, local RAG data, and exports are under `QStandardPaths::DocumentsLocation/Sentinel`; logs and crash dumps are under `AppLocalDataLocation`.

Qt resolves these roots per operating system and application identity, so hard-coded home-directory paths are intentionally not promised here. On typical systems this maps to platform configuration, Documents, and local application-data locations. Use the Desktop’s diagnostics or Qt platform tools to inspect the resolved path.

| Data | Filename or directory |
| --- | --- |
| Settings | `settings.json` |
| Memory | `memory.sqlite3` |
| Chat history | `chat_history.sqlite3` |
| Conversations | `conversations.sqlite3` |
| Local retrieval | `local_rag.sqlite3` |
| Exports | `exports/` |
| Logs / crash dumps | `Logs/` / `Crashes/` |

Portable mode is enabled with `--portable` or an adjacent `portable.txt`; all listed files then live beside the executable. This can be convenient but makes the application directory sensitive data.
