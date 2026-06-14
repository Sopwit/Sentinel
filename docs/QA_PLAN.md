# Release QA Plan

Use this plan for Sentinel 1.0 release-candidate validation.

## Automated Checks

```bash
git diff --check
cmake --preset tests
cmake --build --preset tests
ctest --preset tests --output-on-failure
cmake --build --preset tests --target sentinel-desktop
cmake --preset release
cmake --build --preset release --target sentinel-desktop
```

Optional checks when tools are available:

```bash
find apps core tests -type f \( -name '*.cpp' -o -name '*.h' \) -print0 | xargs -0 clang-format --dry-run --Werror
qmllint ui/qml/Main.qml ui/qml/pages/*.qml ui/qml/components/*.qml ui/qml/theme/*.qml
clang-tidy
```

`clang-tidy` is optional until a checked-in project profile exists.

## Smoke Launch

- Launch the built desktop executable from the tests or release build.
- Verify the main window opens.
- Verify About and Diagnostics show version, build number, commit, build type, platform, and
  architecture.
- Verify manual Check for Updates does not perform background polling, download, or installation.
- Verify diagnostics export is explicit and writes to the app-controlled export directory.

In the current Linux offscreen/sandbox environment, exit 139 can occur during smoke launch after
successful build and tests. Treat it as an environment limitation only when the build and CTest
suite pass.

## Manual UI Checklist

- Home can start and stop a foreground local chat request where the configured local provider is
  available.
- Settings opens as a modal surface.
- Notifications can be filtered, pinned, archived, and cleared.
- Command Palette opens with `Ctrl/Cmd+K` and executes navigation/update/settings commands.
- Export preview and diagnostics export require explicit user action.
- No screen presents telemetry opt-out language because telemetry is absent.

## Accessibility Checklist

- Keyboard navigation reaches primary shell controls.
- Focus indicators are visible.
- Reduced motion, high contrast, and UI density settings persist locally.
- Dialogs and primary buttons expose accessible names.
- Text remains readable at compact, comfortable, and large density settings.

## Packaging Checklist

- macOS `.app` contains the expected bundle id, icon, Info.plist values, display name, copyright,
  category, and minimum target.
- Windows executable contains icon and version resource metadata.
- Linux install tree contains desktop file, AppStream metadata, SVG icon, and PNG icon.
- Packaging tools are invoked only by explicit operator action.
- Missing packaging tools do not fail normal configure, build, or test workflows.
