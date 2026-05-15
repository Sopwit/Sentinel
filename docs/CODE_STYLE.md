# Sentinel Code Style

## C++

- Use C++20.
- Prefer clear ownership with values, references, and `std::unique_ptr`.
- Keep interfaces small and stable.
- Use `sentinel::core` for core library code.
- Keep business logic out of QML.
- Add comments only when they clarify non-obvious behavior.

## Qt

- Expose only necessary QObject APIs to QML.
- Prefer properties and invokable methods over direct UI-side state mutation.
- Keep QML focused on layout, binding, and presentation.

## CMake

- Keep targets explicit.
- Link only required Qt modules.
- Prefer small libraries with clear boundaries over global include paths and shared state.

## Project Direction

- Prefer clarity over clever abstractions.
- Do not add placeholder subsystems that imply working functionality.
- Expand interfaces only when a real implementation requires it.
