# Installation & Deployment Guide

This document details installation procedures, system requirements, pre-built packages, and local AI runtime setup for **Sentinel** across Linux, macOS, and Windows.

---

## 1. System Requirements

| Component | Minimum | Recommended |
| :--- | :--- | :--- |
| **Operating System** | Linux (Fedora 40+, Ubuntu 22.04+, Arch), macOS 14+ (Sonoma/Sequoia), Windows 10/11 (x64/ARM64) | Fedora 41+ KDE Plasma 6 (Wayland) / macOS 15 Apple Silicon / Windows 11 |
| **Processor** | Dual-core 64-bit x86_64 or Apple Silicon ARM64 | Modern 8-core CPU or Apple Silicon (M1/M2/M3/M4) |
| **RAM** | 8 GB RAM | 16 GB - 32 GB RAM (for local 7B-14B LLM inference) |
| **Storage** | 500 MB free space (Application binaries) | 20 GB+ SSD space (for Ollama model storage) |
| **GPU (Optional)** | Integrated graphics | Dedicated NVIDIA GPU (CUDA), Apple Silicon Unified Memory, or AMD ROCm |

---

## 2. Installing Pre-Built Packages

Sentinel provides native installers and portable packages for all major operating systems. Download release assets from the [GitHub Releases](https://github.com/Sopwit/Sentinel/releases) page.

### Linux

#### Fedora & RHEL-based Systems (RPM)
```bash
# Install via DNF (resolves Qt 6 and desktop dependencies automatically)
sudo dnf install -y ./sentinel-desktop-1.0.0-1.x86_64.rpm
```
Or open the `.rpm` package using **KDE Discover** or GNOME Software.

#### Debian & Ubuntu-based Systems (DEB)
```bash
# Install via APT
sudo apt update
sudo apt install -y ./sentinel-desktop_1.0.0_amd64.deb
```

#### Generic Linux (AppImage & Tarball)
```bash
# AppImage (self-contained, no installation required)
chmod +x Sentinel-1.0.0-x86_64.AppImage
./Sentinel-1.0.0-x86_64.AppImage

# Portable tarball
tar -xzf sentinel-desktop-1.0.0-linux-x64.tar.gz
cd sentinel-desktop-1.0.0-linux-x64
./sentinel-desktop
```

---

### macOS

#### Disk Image (DMG)
1. Download `Sentinel-1.0.0-macOS-arm64.dmg` (Apple Silicon) or `x86_64.dmg` (Intel).
2. Open the DMG image and drag **Sentinel.app** into your `/Applications` folder.
3. Launch Sentinel from Launchpad or Spotlight.

#### Homebrew Cask
```bash
brew install --cask Sopwit/tap/sentinel
```

---

### Windows

#### Windows Installer (MSI / EXE)
1. Download `Sentinel-1.0.0-Setup.exe` or `Sentinel-1.0.0-x64.msi`.
2. Run the installer wizard and follow the prompts.
3. Sentinel installs to `C:\Program Files\Sentinel` and registers desktop shortcuts and start menu entries.

#### Portable ZIP Archive
1. Download `sentinel-1.0.0-win64.zip`.
2. Extract to a directory of your choice (e.g., `C:\Tools\Sentinel`).
3. Run `sentinel-desktop.exe`.

---

## 3. Local AI Runtime Setup

Sentinel prioritizes 100% offline, privacy-first local execution via loopback HTTP connections.

### Option A: Ollama (Recommended)

1. **Install Ollama:**
   - **Linux:** `curl -fsSL https://ollama.com/install.sh | sh`
   - **macOS / Windows:** Download installer from [ollama.com](https://ollama.com).

2. **Pull your preferred models:**
   ```bash
   # General conversational & reasoning
   ollama pull llama3.2:latest
   
   # Coding & engineering companion
   ollama pull qwen2.5-coder:7b
   
   # Fast small footprint model
   ollama pull deepseek-r1:7b
   ```

3. **Verify Ollama Loopback:**
   ```bash
   curl http://127.0.0.1:11434/api/tags
   ```

4. **Launch Sentinel:**
   Sentinel automatically connects to `http://127.0.0.1:11434`, discovers installed models, and sets the active model.

### Option B: LM Studio / llama.cpp / Custom Endpoints

1. Launch **LM Studio** or `llama-server`.
2. Start the local inference server (default port `1234` or `8080`).
3. In Sentinel, open **Settings (`Ctrl+,`) > Model & Provider Configuration**.
4. Select **OpenAI-Compatible Local Endpoint** and set URL to `http://127.0.0.1:1234/v1`.

---

## 4. System Files & Storage Locations

All user configurations and persistent data adhere strictly to platform standard paths (`QStandardPaths`):

| Platform | Configuration (`settings.json`) | Databases (`*.sqlite3`) |
| :--- | :--- | :--- |
| **Linux** | `~/.config/sentinel/settings.json` | `~/.local/share/sentinel/` |
| **macOS** | `~/Library/Preferences/sentinel/settings.json` | `~/Library/Application Support/sentinel/` |
| **Windows**| `%APPDATA%\sentinel\settings.json` | `%LOCALAPPDATA%\sentinel\` |

Persistent databases include:
- `chat_history.sqlite3`: Thread histories, message sessions, and timestamps.
- `memory.sqlite3`: Long-term episodic memory, user preferences, and workspace context.
- `local_rag.sqlite3`: Document chunks, embeddings, and full-text search indexes.

---

## 5. Keyboard Shortcuts

| Shortcut (Linux / Win) | Shortcut (macOS) | Action |
| :--- | :--- | :--- |
| `Ctrl+K` | `Cmd+K` | Open universal Command Palette |
| `Ctrl+N` | `Cmd+N` | Create new conversation thread |
| `Ctrl+W` | `Cmd+W` | Close active conversation |
| `Ctrl+,` | `Cmd+,` | Open Preferences & Settings |
| `Ctrl+P` | `Cmd+P` | Toggle conversation pin status |
| `Ctrl+B` | `Cmd+B` | Toggle sidebar visibility |
| `Ctrl+L` | `Cmd+L` | Clear chat view |
| `Ctrl+Shift+T` | `Cmd+Shift+T` | Toggle Dark / Light Liquid Glass theme |
| `Escape` | `Escape` | Close active dialog, palette, or modal |

---

## 6. Uninstallation

### Fedora / RHEL
```bash
sudo dnf remove -y sentinel-desktop
```

### Ubuntu / Debian
```bash
sudo apt remove -y sentinel-desktop
```

### macOS
Delete `/Applications/Sentinel.app` and remove user data:
```bash
rm -rf ~/Library/Application\ Support/sentinel ~/Library/Preferences/sentinel
```

### Windows
Uninstall via **Windows Settings > Installed apps** or run `C:\Program Files\Sentinel\uninstall.exe`.
