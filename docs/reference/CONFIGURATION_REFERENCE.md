# Configuration reference

Settings are versioned application state rather than a public text configuration format. `AppSettings` owns persisted user preferences; `SettingsService` validates supported writes and composes settings snapshots. The CLI exposes only the three keys below; the Desktop exposes additional subsystem-owned settings.

| Setting | Type | Default / scope | Description |
| --- | --- | --- | --- |
| `ollamaEndpoint` | URL string | application | Endpoint used for Ollama health, discovery, and local inference. |
| `routingMode` | named mode | application | Selected routing mode reported by `AppSettings`. |
| `appLanguage` | locale/name string | application | Selected application language. |

| Area | Owner | Notes |
| --- | --- | --- |
| Provider and model selection | `ModelService` / `AppSettings` | Ollama endpoint and selected model are user-facing. |
| Provider credentials | `CredentialStore` | UI exposes presence only; raw values are not settings output. |
| Network and privacy | `NetworkPolicyService` / privacy services | Effective policy may combine workspace and requested state. |
| Permissions | permission services and grant store | Persistent grants are separate from general settings. |
| Workspaces | `WorkspaceService` | Global, preset, workspace, and session preferences resolve by scope. |
| Speech and extensions | owning services | Settings snapshots reflect subsystem availability. |

The on-disk settings path is an implementation detail listed in [Data locations](DATA_LOCATIONS.md). Unsupported keys should not be written by hand.
