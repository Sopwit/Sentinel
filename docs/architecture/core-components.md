# Core Subsystems & Components

Sentinel's core functionality is organized into specialized C++ components behind clean interface contracts.

---

## 1. Provider Subsystem (`IChatProvider`)

- Handles connection state, local AI endpoint health checking, and model discovery.
- **Ollama Provider Boundary**: Probes `http://127.0.0.1:11434` for service status and model catalog tags.
- Keeps model discovery and catalog indexing distinct from prompt execution.

---

## 2. Memory & Settings Subsystems

- `IMemoryStore`: Manages key-value memory records, agent memories, and persistent context.
- `ISettingsStore`: Manages user preferences, active theme, window states, and provider configuration in JSON format.

---

## 3. Task Planning & Agent Registry

- Manages multi-step task planning snapshots, execution metadata, and agent capabilities.
- Enforces controlled execution boundaries: task plans are represented as metadata structures requiring explicit operator step-by-step approvals.

---

## 4. Platform Services (`IPlatformService`)

- Provides OS-level integration abstractions (`IPathProvider`, `INotificationService`, `ISystemIntegrationService`).
- Keeps platform-specific Linux/KDE Plasma, macOS, or Windows calls isolated from core business logic.
