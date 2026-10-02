# Models

A model is selected through a provider; it is not itself a provider. `ModelService`, `ModelRegistry`, and `IModelRouter` manage provider/model selection and produce a `ModelBinding` for an active run.

Ollama discovery reports installed local models and health. Model-management services also maintain lifecycle and storage metadata, including Hugging Face source support. Selection is validated against the selected provider’s current capabilities.

Model binaries and external runtimes are not bundled by Sentinel. Local model use remains local to the configured runtime endpoint; cloud model use crosses the configured provider’s network boundary.
