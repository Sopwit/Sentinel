#!/bin/sh
# Run from the repository root after building the daemon and release Rust TUI.
# Requires a running local LM Studio server and both models already installed.
# No model download, permission bypass, or private workspace is involved.
set -eu
: "${V323_APPROVE_QWEN:?Set V323_APPROVE_QWEN=1 only after explicitly approving the installed Qwen comparison}"
test "$V323_APPROVE_QWEN" = 1
repo=$(pwd)
scripts="$repo/docs/reviews/agent-v323-2026-10-10/reproduce"
work=$(mktemp -d "${TMPDIR:-/tmp}/sentinel-v323-reproduce.XXXXXX")
chmod 700 "$work"
mkdir "$work/evidence"
curl --fail --silent --show-error http://127.0.0.1:1234/api/v1/models > "$work/evidence/models-before.json"
python3 - "$work/evidence/models-before.json" <<'PY'
import json,sys
models=json.load(open(sys.argv[1]))['models']
assert not any(m.get('loaded_instances') for m in models), 'Refusing to alter an already-loaded user model'
assert {'nvidia/nemotron-3-nano-4b','qwen/qwen3.5-9b'} <= {m['key'] for m in models}, 'Install models separately; this script does not download them'
PY
cp build/release/apps/sentinel-daemon/sentinel-daemon "$work/sentinel-daemon"
export SENTINEL_V32_DAEMON="$work/sentinel-daemon"
export QT_LOGGING_RULES='sentinel.provider.diagnostics.info=true;sentinel.agent.diagnostics.info=true'
relay_pid=
owned_model=
cleanup() {
 if test -n "$owned_model"; then lms unload "$owned_model" >> "$work/evidence/unload.log" 2>&1 || true; fi
 if test -n "$relay_pid"; then kill "$relay_pid" 2>/dev/null || true; fi
 echo "Evidence: $work/evidence"
}
trap cleanup EXIT
trap 'exit 130' INT TERM
harness=live_direct.py
if test "${V323_DIRECT_FLOW:-0}" != 1; then
 harness=live.py
 python3 "$scripts/relay.py" "$work/evidence/relay" > "$work/evidence/relay.log" 2>&1 &
 relay_pid=$!
 export V323_PROXY_PORT="$work/evidence/relay/proxy-port.txt"
 for tick in 1 2 3 4 5; do test -f "$V323_PROXY_PORT" && break; sleep 1; done
 test -f "$V323_PROXY_PORT"
fi
for model in nvidia/nemotron-3-nano-4b qwen/qwen3.5-9b; do
 owned_model="$model"
 lms load "$model" --context-length 8192 --parallel 4 -y >> "$work/evidence/load.log" 2>&1
 name=nemotron
 test "$model" != qwen/qwen3.5-9b || name=qwen
 curl --fail --silent --show-error http://127.0.0.1:1234/api/v1/models > "$work/evidence/$name-loaded.json"
 V323_OUT="$work/evidence/$name" V323_MODEL="$model" python3 "$scripts/$harness"
 lms unload "$model" >> "$work/evidence/unload.log" 2>&1
 owned_model=
done
