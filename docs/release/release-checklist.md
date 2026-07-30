# Sentinel Release Checklist

This release checklist details pre-distribution validation rules for Sentinel release candidates.

---

## 1. Build Verification

- [ ] Configure test preset: `cmake --preset tests`
- [ ] Build test targets: `cmake --build --preset tests`
- [ ] Execute full test suite: `ctest --preset tests --output-on-failure`
- [ ] Build desktop executable: `cmake --build --preset tests --target sentinel-desktop`
- [ ] Configure release preset: `cmake --preset release`
- [ ] Build release target: `cmake --build --preset release --target sentinel-desktop`

---

## 2. Quality Assurance & Formatting

- [ ] Run `git diff --check` for whitespace anomalies.
- [ ] Verify formatting with `clang-format --dry-run --Werror`.
- [ ] Run QML syntax check: `qmllint`.
- [ ] Perform smoke launch on clean target OS installation.

---

## 3. Packaging & Metadata Verification

- [ ] Confirm application version, build number, git commit hash in About / Diagnostics.
- [ ] Linux: Verify `.desktop` file, AppStream XML metadata, SVG/PNG icons.
- [ ] macOS: Verify bundle ID, Info.plist metadata, icon set, minimum macOS version.
- [ ] Windows: Verify executable metadata, application icon resource, manifest.

---

## 4. Privacy & Security Guarantees

- [ ] Confirm zero telemetry or background analytics collection.
- [ ] Verify update check remains manual-only with zero automatic background polling.
- [ ] Confirm diagnostics export writes only to user-selected export path.
- [ ] Verify zero credentials or private build tokens in binary artifacts.

---

## 5. Platform Validation Targets

- [ ] **Fedora KDE Plasma**: Primary manual validation target.
- [ ] **Ubuntu / Debian**: Linux portability check.
- [ ] **macOS (ARM64 / x86_64)**: Bundle verification and startup check.
- [ ] **Windows 11**: Executable startup and settings path isolation.
