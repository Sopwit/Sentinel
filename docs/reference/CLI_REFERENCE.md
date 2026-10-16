# Legacy C++ CLI reference

This reference covers the in-process compatibility executable, not the daemon-backed Rust `sentinel` CLI/TUI. See the [Rust CLI guide](../user/CLI.md).

Syntax: `sentinel-cli <command> [args...]`. No command prints usage and exits `0`; an unknown command or missing command argument exits `1`.

| Command | Current behavior |
| --- | --- |
| `chat <prompt>` | Sends one joined prompt through `ApplicationController`; waits up to 30 seconds for an asynchronous response. Missing prompt exits `1`. |
| `model [list\|pull]` | Lists local Ollama models by default. `model pull <model_name>` requests a pull and prints progress. |
| `status` | Prints runtime/status information. |
| `config [get\|set]` | Reads or updates `ollamaEndpoint`, `routingMode`, or `appLanguage`. With no key, `get` prints all three. |

There are no documented global flags, environment-variable overrides, interactive mode, JSON output flag, workspace command, or RAG command in the current CLI dispatcher. Do not infer them from older documentation.

Automation limitation: `chat` can print a 30-second timeout and still return `0`; `model pull` can print failure and still return `0`. Model-list discovery errors do return `1`. These exit semantics are not equivalent to the Rust CLI failure/cancellation contract. No Agent/TUI command or interactive approval workflow exists in this dispatcher.
