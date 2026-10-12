#!/bin/sh
# Stage the existing Rust release and canonical runtime assets; never install or publish.
set -eu
sentinel_repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
if [ "$#" -ne 1 ]; then
    printf '%s\n' 'Usage: stage-release.sh NEW_OUTPUT_DIRECTORY (build cargo --release --locked first)' >&2
    exit 2
fi
sentinel_stage=$1
if [ -e "$sentinel_stage" ]; then
    printf '%s\n' 'Output already exists; use a new directory.' >&2
    exit 2
fi
sentinel_binary="$sentinel_repo_root/cli/target/release/sentinel"
if [ ! -f "$sentinel_binary" ]; then
    printf '%s\n' 'Missing cli/target/release/sentinel; build the Rust workspace first.' >&2
    exit 1
fi
mkdir -p "$sentinel_stage/bin" "$sentinel_stage/share/sentinel/core-warden" "$sentinel_stage/share/licenses/sentinel"
cp "$sentinel_binary" "$sentinel_stage/bin/sentinel"
cp "$sentinel_repo_root/assets/core-warden/state-contract.json" "$sentinel_stage/share/sentinel/core-warden/"
cp -R "$sentinel_repo_root/assets/core-warden/animations" "$sentinel_stage/share/sentinel/core-warden/"
cp "$sentinel_repo_root/LICENSE" "$sentinel_stage/share/licenses/sentinel/LICENSE"
cp "$sentinel_repo_root/THIRD_PARTY_NOTICES.md" "$sentinel_stage/share/licenses/sentinel/THIRD_PARTY_NOTICES.md"
printf '%s\n' 'Local evaluation stage only. Not signed/notarized. Daemon is separate; no runtime is bundled.' > "$sentinel_stage/README.txt"
printf '%s\n' "$sentinel_stage"
