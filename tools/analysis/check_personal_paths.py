#!/usr/bin/env python3
"""Reject concrete home-directory paths in public Git source/evidence files."""
import argparse
from pathlib import Path
import re
import subprocess

PATTERN = re.compile(r'(?:/Users/|/home/|[A-Za-z]:[\\/]Users[\\/])([A-Za-z0-9_.-]+)[\\/]')

def check(root):
    names = subprocess.check_output(['git', 'ls-files', '-z', '--cached', '--others', '--exclude-standard'], cwd=root).decode().split('\0')
    failures = []
    for name in sorted(set(names)):
        path = root / name
        if not name or not path.is_file():
            continue
        data = path.read_bytes()
        # Binaries can contain debug paths too; report them without exposing content.
        value = data.decode('utf-8', errors='replace')
        # Linuxbrew's documented shared installation prefix is not a user path.
        value = value.replace('/home/linuxbrew/.linuxbrew/', '<system-homebrew>/')
        if PATTERN.search(value):
            failures.append(name)
    return failures

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, default=Path(__file__).resolve().parents[2])
    args = parser.parse_args()
    failures = check(args.repo)
    for name in failures:
        print(f'Personal directory path found: {name}')
    print(f'Personal path scan: {len(failures)} file(s) flagged.')
    raise SystemExit(bool(failures))
