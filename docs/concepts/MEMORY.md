# Memory

Sentinel keeps settings, memory, chat history, conversations, and semantic retrieval data in separate stores. `IMemoryStore` is for key-value memory; it is not a chat-message store. SQLite-backed stores use Qt SQL, while settings use the configured settings-store boundary.

`SQLiteMemoryStore` owns persistent memory. `SQLiteChatHistoryStore` and `SQLiteConversationStore` own their respective transcript/conversation domains. `LocalRagStore` owns local retrieval data. Backup/export is user initiated and excludes credentials and model binaries.

These records can contain sensitive local information. See [Data locations](../reference/DATA_LOCATIONS.md), [Secrets](../security/SECRETS.md), and the Desktop’s privacy and retention controls.
