#!/bin/sh
# Install the canonical thin terminal client without changing shell configuration.
set -eu
sentinel_repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
cargo install --path "$sentinel_repo_root/cli/crates/sentinel-cli" --locked "$@"
# The runtime mascot asset root resolves to ../share/sentinel/core-warden next
# to the installed binary. cargo install only ships the binary, so link the
# checkout assets where the TUI looks for them (SENTINEL_CORE_WARDEN_ASSETS
# stays authoritative when set).
cargo_home=${CARGO_HOME:-$HOME/.cargo}
asset_link=$cargo_home/share/sentinel/core-warden
mkdir -p "$(dirname "$asset_link")"
if [ ! -e "$asset_link" ] && [ ! -L "$asset_link" ]; then
    ln -s "$sentinel_repo_root/assets/core-warden" "$asset_link" || true
fi
if command -v sentinel >/dev/null 2>&1; then
    command -v sentinel
    sentinel --version
else
    printf '%s\n' 'Installation completed. Add the Cargo install bin directory to PATH manually (normally $HOME/.cargo/bin). Run sentinel doctor after starting the daemon.' >&2
fi
