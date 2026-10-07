#!/bin/sh
# Install the canonical thin terminal client without changing shell configuration.
set -eu
sentinel_repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
cargo install --path "$sentinel_repo_root/cli/crates/sentinel-cli" --locked "$@"
if command -v sentinel >/dev/null 2>&1; then
    command -v sentinel
    sentinel --version
else
    printf '%s\n' 'Installation completed. Add the Cargo install bin directory to PATH manually (normally $HOME/.cargo/bin). Run sentinel doctor after starting the daemon.' >&2
fi
