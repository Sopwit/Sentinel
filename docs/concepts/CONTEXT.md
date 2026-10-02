# Context

Context is the bounded, per-request information assembled for a provider or agent run. It can include the current user input, conversation state, instructions, the selected model binding, tool contracts, and observations from prior tool calls. `ContextAssembly`, `ContextEngine`, and session services own this work.

Persistent memory is not automatically equivalent to context. `MemoryRecall` and semantic retrieval can supply relevant records only when their owning policy and runtime are enabled. Tool output is evidence for the current run and may be truncated to protect the context budget.

Workspace content and tool results are untrusted inputs. Their presence in context does not grant tool authority or change permission policy.
