# CLI reference

Syntax: `sentinel-cli <command> [args...]`. No command prints usage and exits `0`; an unknown command or missing command argument exits `1`.

| Command | Current behavior |
| --- | --- |
| `chat <prompt>` | Sends one joined prompt through `ApplicationController`; waits up to 30 seconds (response deadline, see environment overrides) for an asynchronous response. Missing or blank prompt exits `1`. |
| `model [list\|pull]` | Lists local Ollama models by default under a 5-second discovery deadline. `model pull <model_name>` requests a pull, prints progress, and is bounded by an inactivity deadline so a stalled endpoint cannot hang the command. Blank model name exits `1`. |
| `status` | Prints runtime/status information. |
| `config [get\|set]` | Reads or updates `ollamaEndpoint`, `routingMode`, or `appLanguage`. With no key, `get` prints all three. |

## Exit codes

This is the exit-code contract of the retained legacy in-process `sentinel-cli`:

| Code | Meaning |
| --- | --- |
| `0` | Success, or usage printed because no command was supplied. |
| `1` | Usage/argument error: unknown command or subcommand, unknown configuration key, or a missing/blank required argument. |
| `2` | Operation failure: provider failure, refused request, model discovery failure, model pull failure. |
| `3` | Deadline timeout: chat response deadline, model discovery deadline, or model pull inactivity deadline expired. |

Signals are not caught by the CLI: an interrupted run dies by signal and the shell reports the platform convention (for example `130` for `SIGINT` on Unix). A failed operation must never exit `0`, and an asynchronous callback that arrives after a deadline decision cannot convert a failed run into a successful exit; the outcome is decided once, from the state that settled the wait.

This table applies only to `sentinel-cli`. It is separate from the canonical daemon-backed `sentinel` executable, whose exit table is documented in [Sentinel CLI](../user/CLI.md); the only shared rule is that success is `0`. The legacy CLI's disposition (retain or retire) remains open under P0-03/EP-P5 and is not changed by the exit-contract work.

## Environment overrides

Deadline overrides exist so deterministic tests can bound the command waits without changing command-line arguments. Production behavior is unchanged when they are unset.

| Variable | Default | Meaning |
| --- | --- | --- |
| `SENTINEL_CLI_CHAT_TIMEOUT_MS` | `30000` | Chat response deadline in milliseconds (minimum `100`). |
| `SENTINEL_CLI_PULL_IDLE_TIMEOUT_MS` | `60000` | Model pull inactivity deadline in milliseconds (minimum `100`). |

There are no documented global flags, interactive mode, JSON output flag, workspace command, or RAG command in the current CLI dispatcher. Do not infer them from older documentation. The two deadline variables above are the only environment-variable inputs of this CLI.
