# Secrets

Provider, MCP, and plugin credentials are stored through `CredentialStore`. Sentinel prefers macOS Keychain, Windows Credential Manager, and Linux Secret Service. When the appropriate backend is unavailable, the secure credential feature reports an unavailable/disabled fallback rather than claiming secure persistence.

Settings views expose credential presence, not raw values. The credential policy forbids plaintext persistence and logging of raw secrets. Existing legacy values may be migrated only after a secure write and readback succeed.

Remove or replace credentials through the owning settings or extension flow. Never put credentials in settings exports, bug reports, plugin manifests, source control, or release configuration.
