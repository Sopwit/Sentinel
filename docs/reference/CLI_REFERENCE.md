# CLI reference

Syntax: `sentinel-cli <command> [args...]`. No command prints usage and exits `0`; an unknown command or missing command argument exits `1`.

| Command | Current behavior |
| --- | --- |
| `chat <prompt>` | Sends one joined prompt through `ApplicationController`; waits up to 30 seconds for an asynchronous response. Missing prompt exits `1`. |
| `model [list\|pull]` | Lists local Ollama models by default. `model pull <model_name>` requests a pull and prints progress. |
| `status` | Prints runtime/status information. |
| `config [get\|set]` | Reads or updates `ollamaEndpoint`, `routingMode`, or `appLanguage`. With no key, `get` prints all three. |

There are no documented global flags, environment-variable overrides, interactive mode, JSON output flag, workspace command, or RAG command in the current CLI dispatcher. Do not infer them from older documentation.
