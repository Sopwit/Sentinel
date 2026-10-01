# Configuration

Desktop settings own provider selection, model selection, workspace preferences, permissions, speech, privacy, and system settings. `SettingsService` composes settings snapshots; subsystem services retain ownership of their own state.

Configure an Ollama endpoint before selecting a local model. Cloud providers require an explicit credential and are subject to the effective network policy. A healthy local loopback runtime remains available in Local Only and Offline modes where the relevant policy permits it.

Provider credentials are represented as presence metadata at the UI boundary; see [Secrets](../security/SECRETS.md). Exact stable settings are listed in the [configuration reference](../reference/CONFIGURATION_REFERENCE.md).
