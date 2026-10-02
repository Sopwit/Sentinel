# Quickstart

1. Start a local Ollama service and install a model, or configure a cloud provider credential in Desktop settings.
2. Launch `sentinel-desktop` and select the provider and model.
3. Use chat for a provider conversation. Use Agent Mode only when you intend Sentinel to propose or perform tool-backed work; approvals remain part of the execution path.

The current CLI is deliberately small:

```bash
sentinel-cli status
sentinel-cli model list
sentinel-cli chat "Summarize this idea"
```

See [Desktop](../interfaces/DESKTOP.md), [CLI](../interfaces/CLI.md), and [Agent Runtime](../concepts/AGENT_RUNTIME.md).
