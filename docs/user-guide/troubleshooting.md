# Troubleshooting & FAQ

Frequently asked questions and solutions for common issues when running Sentinel Desktop.

---

## 1. Ollama Connection Issues

### Problem: "Ollama status shows Disconnected" or "No models detected"

**Possible Causes & Solutions:**
- **Ollama Service is Not Running**: Ensure Ollama is running locally. Run `ollama serve` in a terminal or verify the system service status (`systemctl status ollama`).
- **Custom Port or IP**: Sentinel probes `http://127.0.0.1:11434`. If Ollama is running on a custom port or remote server, configure the host URL in **Settings -> Provider Settings**.
- **No Models Pulled**: Sentinel requires at least one model in Ollama's catalog. Run `ollama pull llama3.2` or `ollama pull qwen2.5-coder` to pull models.

---

## 2. Display or Rendering Issues on Linux (KDE Plasma)

### Problem: Window background transparency or blur effect is disabled

**Solutions:**
- **KWin Compositor**: Ensure KDE Plasma compositor is enabled (`System Settings -> Display & Monitor -> Compositor`).
- **Software Rendering**: If running in a virtual machine without GPU acceleration, force software rendering by launching with `QT_QUICK_BACKEND=software ./sentinel-desktop`.

---

## 3. SQLite Database Locked or Permission Errors

### Problem: Application fails to open memory or chat history database

**Solutions:**
- Verify write permissions for `QStandardPaths::AppDataLocation`:
  - **Linux**: `~/.local/share/Sopwit/Sentinel/`
  - **macOS**: `~/Library/Application Support/Sopwit/Sentinel/`
  - **Windows**: `%LOCALAPPDATA%\Sopwit\Sentinel\`
- Ensure another instance of Sentinel is not running in the background (`killall sentinel-desktop`).

---

## 4. Frequently Asked Questions (FAQ)

### Q: Does Sentinel send my conversation data to cloud servers?
**A:** No. Sentinel is 100% local-first and zero-telemetry. All data is saved in local SQLite and JSON stores on your hard drive.

### Q: Can I run Sentinel on Apple Silicon Macs?
**A:** Yes. Sentinel compiles natively for ARM64 (`darwin-arm64`) on Apple Silicon as well as x86_64 macOS.
