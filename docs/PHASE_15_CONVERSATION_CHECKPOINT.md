# Phase 15 Conversation Checkpoint

Phase 15.33-15.35 completes the conversation browser runtime QA checkpoint for the current
Desktop Alpha conversation architecture.

## Completed QA Scope

- Reviewed the active multi-conversation storage/browser/session switching path.
- Verified that `IConversationStore` remains separate from `IChatHistoryStore` and `IMemoryStore`.
- Verified that legacy single-transcript chat history remains a compatibility startup source when
  the conversation store is empty.
- Verified that SQLite conversation deletion is soft metadata only and keeps stored message rows.
- Verified that archived conversations remain visible/loadable and reject new sends clearly.
- Verified that conversation switching cancels active local inference metadata and stale async
  completions are ignored by request-id guards.
- Verified that permanent delete readiness remains disabled and request handling refuses without
  mutating storage.

## Architecture Findings

- `ApplicationController` owns one active conversation id and never exposes raw store objects to
  QML.
- `SQLiteConversationStore` owns `conversations.sqlite3`; legacy `chat_history.sqlite3` remains a
  separate compatibility store.
- Browser actions are limited to create, switch, rename, archive, and unarchive.
- Permanent delete is represented by value-only policy/readiness/result metadata. The controller
  refusal path does not call `IConversationStore::deleteConversation()`.
- Archived active conversations are read-only for sending through both controller checks and store
  append refusal.
- Async local inference callbacks are request-id guarded, so a completion after switch/cancel
  cannot append to the newly active transcript.

## Confirmed Non-Goals

- No semantic memory, embeddings, vector database, SQLite FTS, cloud sync, import/export workflow
  expansion, permanent delete execution, broad UI redesign, model/voice/tool/plugin change, or
  runtime-policy expansion was added.

## Remaining Limitations

- Permanent delete is still non-operational.
- Conversation export remains scoped to the currently visible transcript.
- The browser remains compact and local-only.
- Legacy single-transcript history is a compatibility source, not a multi-conversation migration
  framework.

## Phase 16 Readiness Verdict

Phase 16 is ready to begin for explicitly scoped future work. It is not ready for destructive
permanent delete, cloud sync, semantic/vector memory, import, multi-conversation export, tool/plugin
execution, or runtime authority expansion without a new phase gate and matching tests.
