#!/bin/sh
# Transport-only controls, separately from Agent acceptance; no diagnostic relay.
set -eu
: "${V323_APPROVE_QWEN:?Explicit Qwen approval required}"
test "$V323_APPROVE_QWEN" = 1
scripts="$(pwd)/docs/reviews/agent-v323-2026-10-10/reproduce"
work=$(mktemp -d "${TMPDIR:-/tmp}/sentinel-v323-direct.XXXXXX")
chmod 700 "$work"
curl --fail --silent --show-error --max-time 10 http://127.0.0.1:1234/api/v1/models > "$work/models-before.json"
python3 - "$work/models-before.json" <<'PY'
import json,sys
models=json.load(open(sys.argv[1]))['models']
assert not any(m.get('loaded_instances') for m in models), 'Refusing to alter an already-loaded user model'
assert {'nvidia/nemotron-3-nano-4b','qwen/qwen3.5-9b'} <= {m['key'] for m in models}, 'Models must already be installed'
PY
owned_model=
cleanup() {
 if test -n "$owned_model"; then lms unload "$owned_model" >> "$work/unload.log" 2>&1 || true; fi
 echo "Transport evidence: $work"
}
trap cleanup EXIT
trap 'exit 130' INT TERM
for model in nvidia/nemotron-3-nano-4b qwen/qwen3.5-9b; do
 owned_model="$model"
 lms load "$model" --context-length 8192 --parallel 4 -y >> "$work/load.log" 2>&1
 name=nemotron
 test "$model" != qwen/qwen3.5-9b || name=qwen
 curl --fail --silent --show-error --max-time 10 http://127.0.0.1:1234/api/v1/models > "$work/$name-loaded.json"
 python3 "$scripts/direct_probe.py" "$model" "$work/$name-probe.json"
 lms unload "$model" >> "$work/unload.log" 2>&1
 owned_model=
done
curl --fail --silent --show-error --max-time 10 http://127.0.0.1:1234/api/v1/models > "$work/models-after.json"
