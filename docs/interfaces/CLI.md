# CLI

**Status: Partial.** The executable is useful for one-shot diagnostics, configuration, model operations, and prompt submission; it is not yet a full interactive or automation-oriented interface.

`sentinel-cli` is a command-oriented interface over `ApplicationController` and the shared settings path. It is not an interactive TUI or a complete scripting API.

Implemented commands are `chat`, `model`, `status`, and `config`; see the exact [CLI reference](../reference/CLI_REFERENCE.md). `chat` accepts one prompt, waits up to 30 seconds for an asynchronous response, and prints the result. It does not expose a persistent interactive session mode or command-line permission prompts.

For shared execution behavior, see [Agent Runtime](../concepts/AGENT_RUNTIME.md).
