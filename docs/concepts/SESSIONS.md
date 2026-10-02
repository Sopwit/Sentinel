# Sessions

Conversation state is owned by `ConversationSession`, `ChatSession`, and their stores. Chat history, conversation records, agent-run records, and key-value memory are separate persistence domains.

The application can reload persisted conversations through `SQLiteConversationStore` and history through `SQLiteChatHistoryStore`. Agent execution has its own `SQLiteAgentRunStore`. Cancelling an active agent run stops that run; it does not imply deletion of the conversation or stored history.

The current CLI command is one-shot. Desktop is the primary surface for resumed conversation and session state.
