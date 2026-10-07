#!/usr/bin/env python3
"""Check production brand provenance without image libraries or build outputs."""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
BRAND = ROOT / 'resources/branding'
FA3 = 'M16 16H44V32L68 20L84 40L68 52L80 76L56 88L40 64L16 72V52L28 44L16 36Z'
BANNED = re.compile(r'resources/app-icons/|(?:nsis_header|nsis_welcome|wix_dialog|wix_banner)\.bmp|sentinel-(?:logo|icon)\.(?:svg|png|ico|icns)|(?:Narrow|Inset|Integrated) ?(?:Plane|Field)|Minimal ?Edge|#303438', re.I)
VISUAL = {'.svg', '.png', '.ico', '.icns', '.bmp', '.jpg', '.jpeg', '.webp'}


def source_files():
    names = subprocess.check_output(['git', 'ls-files', '-z', '--cached', '--others', '--exclude-standard'], cwd=ROOT).decode().split('\0')
    return sorted({ROOT / name for name in names if name and (ROOT / name).is_file()})


def check():
    errors = []
    files = source_files()
    for file in files:
        relative = file.relative_to(ROOT).as_posix()
        if relative.startswith(('docs/', 'tests/', 'tools/branding/', 'resources/branding/')):
            continue  # History and the frozen provenance package are not production selectors.
        if file.suffix.lower() in VISUAL:
            if relative.startswith('resources/icons/') and file.suffix == '.svg' and FA3 in file.read_text():
                errors.append(f'{relative}: brand master disguised as a functional icon')
            if not relative.startswith('resources/icons/'):
                errors.append(f'{relative}: independent visual asset outside authoritative brand root')
            continue
        if file.suffix.lower() in {'.ttf', '.otf', '.woff', '.woff2'}:
            continue
        try:
            content = file.read_text()
        except (UnicodeError, OSError):
            continue
        for number, line in enumerate(content.splitlines(), 1):
            if BANNED.search(line):
                errors.append(f'{relative}:{number}: legacy production reference')
    manifest = json.loads((BRAND / 'manifest.json').read_text())
    if manifest['geometry'] != FA3 or manifest['authoritativePath'] != FA3:
        errors.append('manifest: changed FA-3 geometry')
    expected = {item['path'] for item in manifest['files']} | {'manifest.json', 'validation-report.json'}
    actual = {file.relative_to(BRAND).as_posix() for file in BRAND.rglob('*') if file.is_file()}
    if actual != expected:
        errors.append(f'branding inventory mismatch: {sorted(actual ^ expected)}')
    for item in manifest['files']:
        file = BRAND / item['path']
        if not file.is_file() or hashlib.sha256(file.read_bytes()).hexdigest() != item['sha256']:
            errors.append(f"manifest hash mismatch: {item['path']}")
    for file in BRAND.rglob('*.svg'):
        tree = ET.parse(file)
        nodes = list(tree.iter())
        paths = [node for node in nodes if node.tag.endswith('}path')]
        if not any(node.get('d') == FA3 for node in paths) or ('/docs/' not in file.as_posix() and any(node.get('d') != FA3 for node in paths)):
            errors.append(f'{file.relative_to(ROOT)}: changed FA-3 path')
        if any(node.tag.split('}')[-1] in {'filter', 'linearGradient', 'radialGradient', 'image'} for node in nodes):
            errors.append(f'{file.relative_to(ROOT)}: prohibited decoration')
        if file.name == 'master.svg' or 'app-icon/linux/scalable' in file.as_posix():
            shapes = [node for node in nodes if node.tag.split('}')[-1] in {'rect', 'path'}]
            if len(shapes) != 2 or {node.get('fill') for node in shapes} != {'#151719', '#ECEFEE'}:
                errors.append(f'{file.relative_to(ROOT)}: retired app-icon composition')
        if '/tray/' in file.as_posix():
            if any(node.get('fill') not in {None, '#000000', '#FFFFFF', 'currentColor', 'none'} for node in nodes):
                errors.append(f'{file.relative_to(ROOT)}: non-monochrome tray')
    if errors:
        raise SystemExit('\n'.join(errors))
    print(f'PASS: {len(files)} source files; {len(actual)} branding files; hashes, FA-3, two-color app icon, monochrome tray and legacy selectors')


if __name__ == '__main__':
    check()
