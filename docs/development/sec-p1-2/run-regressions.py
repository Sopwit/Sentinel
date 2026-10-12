#!/usr/bin/env python3
"""Run existing terminal fixtures with a selected binary and new private output paths.

Does not edit archived harnesses or overwrite their historical results.
"""
import argparse
import hashlib
import json
from pathlib import Path

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--repo', type=Path, required=True)
p.add_argument('--binary', type=Path, required=True)
p.add_argument('--output', type=Path, required=True)
p.add_argument('--suite', choices=['compatibility', 'signals', 'warden', 'comparison'], required=True)
p.add_argument('--profile', help='Compatibility profile subset for diagnosis; omit for the full matrix')
a = p.parse_args()
root, binary, out = a.repo.resolve(), a.binary.resolve(), a.output.resolve()
out.mkdir(parents=True, exist_ok=False)
if a.suite in ('warden', 'comparison'):
    path = root / 'docs/reviews/core-warden-cw045/production_pty.py'
    source = path.read_text()
    old = "ROOT=Path(__file__).resolve().parents[3]\nOUT=Path(__file__).resolve().parent\nBINARY=ROOT/'cli/target/release/sentinel'"
    new = f'ROOT=Path({str(root)!r})\nOUT=Path({str(out)!r})\nBINARY=Path({str(binary)!r})'
else:
    path = root / f'docs/reviews/tui-v4b/{a.suite}.py'
    source = path.read_text()
    old = "HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];EXE=HERE/'artifacts/sentinel-cli-1.0.0-rc.8-macos-arm64-evaluation/bin/sentinel'"
    new = f'HERE=Path({str(out)!r});ROOT=Path({str(root)!r});EXE=Path({str(binary)!r})'
    if a.suite == 'compatibility':
        source = source.replace("HERE.parent/'tui-v33b/pty_journeys.py'", "ROOT/'docs/reviews/tui-v33b/pty_journeys.py'")
        marker = "spec.loader.exec_module(vt)"
        source = source.replace(marker, marker + "\nimport re\n_original_screen = vt.screen\ndef _screen_without_dcs(data, w, h):\n    data = re.sub(rb'\\x1bP[\\s\\S]*?(?:\\x1b\\\\|$)', b'', data)\n    return _original_screen(data, w, h)\nvt.screen = _screen_without_dcs")
        source = source.replace("   assert b'\\x1b[5n' not in data", "   assert b'\\x1b[5n' not in data\n   assert b'\\x1b[6n' not in data, 'Unexpected blocking cursor-position query'")
        # Wait for the asserted text state while draining graphics; retain a finite deadline.
        source = source.replace("  def expect(text):\n   assert text in view()", "  def expect(text):\n   deadline = time.monotonic() + 1.0\n   while text not in view() and time.monotonic() < deadline: drain(.02)\n   assert text in view()")
        if a.profile:
            source = source.replace(' rows=[]\n', f' profiles=[row for row in profiles if row[0] == {a.profile!r}]\n rows=[]\n')
assert source.count(old) == 1, 'Archived harness structure changed; review adapter first'
source = source.replace(old, new)
# Archived assertions remain intact; compatibility also skips DCS graphics during text
# replay and rejects unexpected cursor-position requests.
(out / 'adapter-receipt.json').write_text(json.dumps({
    'suite': a.suite,
    'harness': str(path.relative_to(root)),
    'original_harness_sha256': hashlib.sha256(path.read_bytes()).hexdigest(),
    'binary_sha256': hashlib.sha256(binary.read_bytes()).hexdigest(),
    'scope': 'Synthetic IPC/PTY fixtures; not physical terminal acceptance',
    'adapter_changes': ['binary/root/output selection'] + (['Skip complete/partial Sixel DCS in text VT replay', 'Reject cursor-position requests', 'Bounded one-second text-state waits while draining graphics'] if a.suite == 'compatibility' else []),
}, indent=2) + '\n')
import sys
sys.dont_write_bytecode = True
sys.argv = [str(path)] + (['--compare'] if a.suite == 'comparison' else [])
exec(compile(source, str(path), 'exec'), {'__name__': '__main__', '__file__': str(path)})
