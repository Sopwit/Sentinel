# Models

A model is selected through a provider; it is not itself a provider. `ModelService`, `ModelRegistry`, and `IModelRouter` manage provider/model selection and produce a `ModelBinding` for an active run.

Ollama discovery reports installed local models and health. ModelOperationService and ModelLibrary support acquisition/import/registration and storage metadata, including Hugging Face sources. ModelLifecycleService is a static metadata catalog, not a process load/unload manager. Downloaded/installed, selected, reachable, authorized and loaded are separate states. Selection is validated against the selected provider’s current capabilities.

The daemon LocalModelRuntime helper can start a selected GGUF using an installed llama-server on an allowed loopback HTTP endpoint when no listener responds. It tracks its own process through ProcessExecutor, avoids competing with an existing listener, and terminates its owned process on an idle model change. A successful health probe does not establish the loaded model identity. Uniform load/unload/retry and external resource ownership are incomplete; fixed context 8192 and parallelism 1 are not adaptive resource management.

Model binaries and external runtimes are not bundled by Sentinel. Local model use remains local to the configured runtime endpoint; cloud model use crosses the configured provider’s network boundary.
