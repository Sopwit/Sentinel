#!/usr/bin/env sh
set -eu

root_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$root_dir"

echo "== git diff whitespace check =="
git diff --check

echo "== configure tests preset =="
cmake --preset tests

echo "== build tests preset =="
cmake --build --preset tests

echo "== run tests preset =="
ctest --preset tests --output-on-failure

echo "== build desktop target =="
cmake --build --preset tests --target sentinel-desktop

if command -v clang-format >/dev/null 2>&1; then
  echo "== clang-format check =="
  find apps core tests -type f \( -name '*.cpp' -o -name '*.h' \) -print0 \
    | xargs -0 clang-format --dry-run --Werror
else
  echo "== clang-format unavailable; skipping optional check =="
fi

if command -v qmllint >/dev/null 2>&1; then
  echo "== qmllint check =="
  qmllint ui/qml/Main.qml ui/qml/pages/*.qml ui/qml/components/*.qml ui/qml/theme/*.qml
else
  echo "== qmllint unavailable; skipping optional check =="
fi
