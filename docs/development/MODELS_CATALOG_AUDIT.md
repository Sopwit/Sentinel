# Models catalogue audit — 2026-10-08

## Findings and fixes

- Corrected DeepSeek R1 8B sources from the nonexistent Qwen variant to the official Llama variant, including its MLX conversion. Removed the unverified `mlx-community/flux1-schnell-mlx` source.
- Removed duplicate FLUX, SDXL, Kokoro and Whisper entries. The bundled catalogue now contains 47 entries: 18 text, 5 reasoning, 2 vision, 4 image, 11 video, 2 speech recognition, 2 text-to-speech, 1 embedding and 2 runtime entries.
- Shared task classification gives explicit media, embedding and vision metadata precedence over family-name guesses. Plain Phi-4 is a text model; it is no longer labelled as an explicit thinking model solely from its name. Mixed vision/reasoning models also appear in the reasoning filter. Local model inference remains a best-effort name fallback when runtime metadata is missing.
- Manual Refresh bypasses the 15-minute Hugging Face query cache. The page displays bundled catalogue validation date, remote status, last fetch time and the 100-artifact display limit. Refresh preserves the current search. Popular/Newest buttons explicitly describe Ollama ordering.
- Page activation and daemon reconnection retry catalogue loading, preventing the initial fetch from being lost before IPC connects. Catalogue loading includes all three sources; source errors are visible. HTML parse failures preserve prior results and report an error. Network catalogue requests have a 12-second timeout and ignore superseded replies.
- LM Studio cloud-only cards cannot offer local downloads. If no Hugging Face repository is known, the LM Studio action opens the actual model catalogue page rather than pretending to open a specific app download. Popularity counts are separated from file sizes.
- LM Studio SVG markup is removed from tags; common HTML character entities are decoded in descriptions and names. Model rows update when metadata changes, including categories and download availability.

## Verification

- All 114 CTest tests passed (149.38 seconds); focused Ollama/runtime and Desktop IPC tests passed again after final text/tag fixes (16.39 seconds).
- Added regression coverage for task precedence, mixed capabilities, cloud-only cards, malformed HTML retention, plain tags and explicit cache bypass.
- Release Desktop/daemon and tests builds succeeded. Changed QML passed lint without errors; context-property access warnings remain. Both generated contract checks and `git diff --check` passed.
- HTTP GET verified all 67 distinct bundled model/source/variant addresses and 245 live Ollama model pages with status 200. Ollama rejects HEAD for these pages, so HEAD responses were not treated as broken links.
- A generic Python HTTP client received 403 from the LM Studio catalogue. The actual Qt application successfully retrieved 104 LM Studio cards; this distinction is a transport/access limitation, not evidence of an invalid catalogue URL. Individual live LM Studio card URLs were not exhaustively fetched.
- Native Refresh increased the merged list to 441 records and advanced the Hugging Face timestamp. The embedding filter contained 10 models. The native cloud-only popup showed the disabled local action and explicit source availability.

## Coverage limits

The screen is a provider-independent discovery catalogue, not an exhaustive synchronized mirror of every upstream model. Bundled media entries update with Sentinel releases, whereas Ollama, LM Studio and Hugging Face metadata refresh on page entry/search/manual refresh. Hugging Face results are bounded to 40 repositories and the UI shows at most 100 GGUF artifacts. Some metadata remains unavailable or inferred. Media catalogue entries require compatible external generation/speech pipelines; chat runtimes do not execute all listed categories. No multi-gigabyte download, exhaustive responsive-window matrix or Linux/Windows native UI certification was performed in this audit.

Primary reference: [Hugging Face Hub API](https://huggingface.co/docs/hub/api). Corrected model sources: [official DeepSeek Llama 8B](https://huggingface.co/deepseek-ai/DeepSeek-R1-Distill-Llama-8B) and [MLX conversion](https://huggingface.co/mlx-community/DeepSeek-R1-Distill-Llama-8B-4bit).
