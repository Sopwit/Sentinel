# Model library, local runtime and Inspector validation

Validated on macOS on 2026-10-08. Linux and Windows remain build/platform targets, but these native flows were not exercised on those hosts.

## Delivered behavior

- Selecting llama.cpp probes its configured loopback HTTP endpoint. If no listener exists, the daemon starts an installed `llama-server` using a registered GGUF model and fixed arguments through `ProcessExecutor`. Existing servers are reused. Only daemon-owned processes are stopped on model changes or shutdown. Missing binary/model conditions are reported by the model helper.
- The provider-independent catalogue contains 52 curated entries, including 11 distinct video models. Live Ollama, LM Studio and Hugging Face GGUF results supplement it. Media pipelines are identified explicitly; catalogue inclusion does not imply support for video generation through a chat runtime.
- Ollama downloads preserve the selected full variant tag. GGUF download/register/import/select operations use `ModelOperationService`; actionable download flags come from the library contract. LM Studio opens `lmstudio://open_from_hf?model=...` for the relevant repository.
- Model details show source, format, variant, size, context, capabilities and available repository metadata. Unknown fields remain marked as not reported.
- Inspector reads the authoritative daemon run store through generated `agent.history` IPC, with typed decoding, pagination and bounded detail payloads. The desktop does not open the daemon database.

## Evidence

- Tests preset configured and built; all 114 CTest tests passed (77.98 seconds). New integration coverage exercises durable Inspector history/details/pagination and provider-independent catalogue/category contracts.
- Rust workspace tests passed. Both generated contract checks and `git diff --check` passed.
- Changed QML passes qmllint with context-property access warnings; no lint errors.
- Release desktop and daemon built successfully. Compilation used `CCACHE_DISABLE=1` because the configured external cache directory is outside the filesystem sandbox.
- Isolated daemon profile automatically launched llama.cpp on port 18080, completed a real chat response, then shut down cleanly with exit code 0.
- The user's desktop profile imported its existing Nemotron GGUF. The daemon automatically launched the server on port 8080. Native UI Send returned `Çalışıyor` and restored the idle composer.
- Native Models navigation, video category, model search, popup, Escape close and keyboard navigation were exercised. Inspector displayed persisted runs without the unavailable-history error.
- The LM Studio button opened the target repository's Download Options screen in the installed application. No large download was started as part of validation.

## Limits

Full multi-gigabyte downloads were not repeated for every model or provider. Remote availability, gating and resource requirements remain source/runtime dependent. Responsive popup sizing is implemented, but exhaustive native window-size and cross-platform visual verification remains outstanding.
