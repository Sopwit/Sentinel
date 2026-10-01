# Adding a provider

Provider behavior belongs behind `IChatProvider`; model selection is routed with `IModelRouter` and a `ModelBinding`. Do not couple a provider directly to QML or bypass network and credential policy.

Add the provider’s configuration and capability representation to the existing provider/model services, preserve cancellation and streaming behavior, and use `CredentialStore` for secrets. Add tests for unavailable credentials, unhealthy transport, cancellation, and selected-model validation. Update [Providers](../capabilities/PROVIDERS.md) only after the implementation is wired into the application.
