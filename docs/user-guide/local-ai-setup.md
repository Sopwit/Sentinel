# Local AI Setup Guide

Sentinel is built to work seamlessly with local AI backends without requiring cloud API subscriptions.

---

## 1. Ollama (Recommended Local Runtime)

Ollama is the preferred local AI provider engine for Sentinel.

### Installing Ollama

#### Linux / Fedora / Ubuntu
```bash
curl -fsSL https://ollama.com/install.sh | sh
```

#### macOS
Download from [ollama.com/download/mac](https://ollama.com/download) or install via Homebrew:
```bash
brew install ollama
```

#### Windows
Download the Windows installer from [ollama.com/download/windows](https://ollama.com/download/windows).

---

## 2. Pulling Recommended Models

Sentinel works best with high-performance open-weights models:

- **General Assistant & Coding**:
  ```bash
  ollama pull llama3.2
  ollama pull qwen2.5-coder
  ```
- **Lightweight / Fast Inference**:
  ```bash
  ollama pull phi3:mini
  ```

---

## 3. Verifying Health & Discovery in Sentinel

1. Launch Ollama in the background (or run `ollama serve`).
2. Open Sentinel Desktop.
3. Sentinel automatically probes `http://127.0.0.1:11434/api/tags` to check health and discover locally pulled models.
4. Open **Settings -> AI Provider** to inspect the detected runtime version and active model catalog.

> [!IMPORTANT]
> Real prompt execution remains disabled until explicitly enabled in Phase 10 execution boundary phases. Current release handles endpoint health checking, discovery, catalog listing, and connection state reporting.
