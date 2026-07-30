# Platform Services API Reference

Platform-specific logic is abstracted behind C++ service interfaces to preserve portability across Linux, macOS, and Windows.

---

## 1. `IPlatformService`

Main interface for OS platform detection, desktop environment integration, and system metadata.

```cpp
namespace sentinel::platform {

class IPlatformService {
public:
    virtual ~IPlatformService() = default;

    virtual QString platformName() const = 0; // "fedora_kde", "linux", "macos", "windows"
    virtual QString desktopEnvironment() const = 0; // "KDE Plasma", "Aqua", "Explorer"
    virtual bool supportsNativeBlur() const = 0;
};

} // namespace sentinel::platform
```

---

## 2. `IPathProvider`

Resolves standard application paths for settings, databases, and temporary scratch files across platforms.

```cpp
namespace sentinel::platform {

class IPathProvider {
public:
    virtual ~IPathProvider() = default;

    virtual QString appDataLocation() const = 0;
    virtual QString appConfigLocation() const = 0;
    virtual QString exportsLocation() const = 0;
};

} // namespace sentinel::platform
```
