#!/bin/sh
set -eu
repo=$(pwd)
work=$(mktemp -d /tmp/sentinel-v324-live.XXXXXX)
chmod 700 "$work"
curl --fail --silent --show-error --max-time 5 http://127.0.0.1:1234/api/v1/models > "$work/models-before.json"
python3 - "$work/models-before.json" <<'PY'
import json,sys
models=json.load(open(sys.argv[1]))['models']
assert not any(m.get('loaded_instances') for m in models), 'Refusing to alter user-loaded instances'
assert {'qwen/qwen3.5-9b','nvidia/nemotron-3-nano-4b'} <= {m['key'] for m in models}, 'Already-installed models required'
PY
cp build/release/apps/sentinel-daemon/sentinel-daemon "$work/sentinel-daemon"
export SENTINEL_V32_DAEMON="$work/sentinel-daemon"
export QT_LOGGING_RULES='sentinel.provider.diagnostics.info=true;sentinel.agent.diagnostics.info=true;sentinel.classification.diagnostics.info=true'
owned_model=
cleanup() {
 if test -n "$owned_model"; then lms unload "$owned_model" >> "$work/unload.log" 2>&1 || true; fi
 echo "Evidence: $work"
}
trap cleanup EXIT
trap 'exit 130' INT TERM
for model in qwen/qwen3.5-9b nvidia/nemotron-3-nano-4b; do
 owned_model="$model"
 lms load "$model" --context-length 8192 --parallel 4 -y >> "$work/load.log" 2>&1
 name=qwen
 test "$model" != nvidia/nemotron-3-nano-4b || name=nemotron
 curl --fail --silent --show-error --max-time 5 http://127.0.0.1:1234/api/v1/models > "$work/$name-loaded.json"
 V323_OUT="$work/$name" V323_MODEL="$model" python3 "$repo/docs/reviews/agent-v324-2026-10-10/reproduce/live_readonly.py"
 lms unload "$model" >> "$work/unload.log" 2>&1
 owned_model=
done
curl --fail --silent --show-error --max-time 5 http://127.0.0.1:1234/api/v1/models > "$work/models-after.json"
