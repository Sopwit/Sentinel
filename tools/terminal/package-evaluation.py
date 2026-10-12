#!/usr/bin/env python3
"""Stage-only macOS arm64 evaluation archive; no build/install/sign/publish."""
import argparse,gzip,hashlib,io,json,os,re,shutil,subprocess,tarfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('output',type=Path);args=ap.parse_args();out=args.output.absolute()
 if out.exists():ap.error('Output already exists; use a new directory')
 binary=ROOT/'cli/target/release/sentinel'
 identity=subprocess.check_output(['file','-b',str(binary)],text=True).strip()
 if 'Mach-O 64-bit executable arm64' not in identity:ap.error('Expected macOS arm64 release binary')
 # The protected V4-B executable accepts only the original three-state catalog.
 # Its version string is unchanged by later source/asset work, so check bytes.
 if digest(binary)=='45087e81132d23b934c88b152195f5f59f5dd0276c7639358ef8e370a690491c':
  manifest=json.loads((ROOT/'assets/core-warden/animations/manifest.json').read_text())
  if len(manifest['states'])!=3:ap.error('Existing V4-B executable cannot read current CW03B assets; rebuild a compatible release or use the protected V4-B archive. No output created.')
 version=subprocess.check_output([str(binary),'--version'],text=True).strip().removeprefix('sentinel ')
 if not re.fullmatch(r'[A-Za-z0-9.+-]+',version):ap.error('Unsafe version string')
 def cargo(*a):return subprocess.check_output(['cargo',*a,'--locked','--offline'],cwd=ROOT/'cli',text=True)
 metadata=json.loads(cargo('metadata','--format-version','1','--filter-platform','aarch64-apple-darwin'))
 tree=cargo('tree','--target','aarch64-apple-darwin','-e','normal,build','--prefix','none','--format','{p}')
 keys=set(re.findall(r'^([\w-]+) v([^\s]+)',tree,re.M))
 packages=sorted([p for p in metadata['packages'] if (p['name'],p['version']) in keys],key=lambda p:(p['name'],p['version']))
 out.mkdir(parents=True);name=f'sentinel-cli-{version}-macos-arm64-evaluation';stage=out/name
 subprocess.run([str(ROOT/'tools/terminal/stage-release.sh'),str(stage)],check=True,stdout=subprocess.DEVNULL)
 docs=stage/'share/doc/sentinel';docs.mkdir(parents=True)
 shutil.copy2(ROOT/'docs/interfaces/CLI.md',docs/'CLI.md')
 shutil.copy2(ROOT/'cli/crates/sentinel-mascot/README.md',docs/'CORE_WARDEN.md')
 notices=['# Third-party notices — local evaluation only','', 'Locked macOS arm64 normal + build graph; conservative superset, not a binary-symbol attribution proof. Original metadata expressions are preserved. No legal clearance is implied.','']
 audit=[];missing=[]
 provenance_root=ROOT/'third_party/licenses'
 provenance=json.loads((provenance_root/'provenance.json').read_text())['packages']
 for p in packages:
  if p['source'] is None:continue
  d=Path(p['manifest_path']).parent;files=[]
  for f in sorted(d.rglob('*')):
   if f.is_file() and re.match(r'(?i)^(licen[sc]e|copying|notice|copyright|unlicense)(?:[._-]|$)',f.name):files.append(f)
  if p['license_file']:
   f=d/p['license_file']
   if f.is_file() and f not in files:files.append(f)
  original_text_present=bool(files)
  target=stage/'share/licenses/third-party'/f"{p['name']}-{p['version']}";entries=[]
  for f in sorted(files):
   if f.is_symlink() or not f.resolve().is_relative_to(d.resolve()):raise ValueError('Unsafe license path')
   rel=f.relative_to(d);dest=target/rel;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(f,dest)
   entries.append({'path':str(dest.relative_to(stage)),'sha256':digest(dest)})
  if not files:
   for supplied in provenance:
    if (supplied['name'],supplied['version'])!=(p['name'],p['version']):continue
    vcs=json.loads((d/'.cargo_vcs_info.json').read_text())
    if vcs['git']['sha1']!=supplied['upstream_revision']:raise ValueError('Upstream license revision mismatch')
    source=provenance_root/supplied['path']
    if source.is_symlink() or not source.resolve().is_relative_to(provenance_root.resolve()):raise ValueError('Unsafe supplied license path')
    if digest(source)!=supplied['sha256']:raise ValueError('Supplied license checksum mismatch')
    dest=target/source.name;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(source,dest)
    entries.append({'path':str(dest.relative_to(stage)),'sha256':digest(dest),'source_url':supplied['source_url'],'complete_license_text':supplied['complete_license_text']})
    original_text_present=supplied['complete_license_text']
  row={k:p[k] for k in ['name','version','license','license_file','repository','source']};row['included_texts']=entries;audit.append(row)
  notices.extend([f"## {p['name']} {p['version']}",f"Declared license: {p['license'] or 'not declared; see original license_file'}",f"Upstream: {p['repository'] or 'not declared'}"])
  notices.extend([f"- [{x['path']}]({x['path']})" for x in entries] or ['**Original license text absent from local crate: unresolved.**'])
  notices.append('')
  if not original_text_present:
   notices.append('**Unresolved: upstream policy text alone is not a complete copyright/permission notice.**')
   missing.append(f"{p['name']} {p['version']}")
 obligations='''# Open distribution obligations

This is an unsigned local evaluation package, not a publication-cleared release.

- Project is GPL-3.0-or-later. Complete Corresponding Source and build/install scripts, dependency source obligations and an appropriate distribution mechanism must be resolved before conveying publicly. This binary archive is not a complete source distribution or source offer.
- Third-party metadata and original available license/notice files are included, without rewriting their declarations or choosing an unverified license branch. Preserve copyright/permission texts, applicable notices and required attribution. Apache, Unicode, BSD and other alternatives require review of the actual chosen distribution terms; this inventory is not a legal opinion or compatibility certification.
- Any crates listed below still lack complete verified original copyright/permission texts. Upstream policy documents may be included as evidence without closing this gate. Resolve these before public distribution.
- Core Warden is supplied project artwork. No third-party resource is intentionally added, but ownership/reference provenance and permission to redistribute artwork require maintainer confirmation. Blender/reference boards are excluded.
- Rust standard library/toolchain and system-library obligations are not exhaustively audited by Cargo metadata. macOS system libraries are linked, not copied. Review toolchain notices and redistribution obligations before publication.
- No Developer ID signing, hardened-runtime evaluation, notarization or Gatekeeper acceptance is claimed.
'''
 (stage/'DISTRIBUTION_GATES.md').write_text(obligations+'\nMissing texts:\n'+''.join('- '+n+'\n' for n in missing))
 (stage/'THIRD_PARTY_NOTICES.md').write_text('\n'.join(notices)+'\n')
 (stage/'dependency-licenses.json').write_text(json.dumps({'target':'aarch64-apple-darwin','scope':'normal + build; excludes dev-only and other-platform-only packages','packages':audit,'missing_original_texts':missing},indent=2)+'\n')
 (stage/'README.txt').write_text(f'''Sentinel CLI/TUI {version} — macOS arm64 local evaluation

Run ./bin/sentinel --version then ./bin/sentinel tui from any directory.
No daemon is bundled or launched. Start your existing compatible daemon separately
or use --socket PATH. Disconnected presentation is intentional and static.
Assets live at share/sentinel/core-warden relative to bin/sentinel.
SENTINEL_CORE_WARDEN_ASSETS overrides discovery, including invalid paths.
Modes: SENTINEL_TUI_MASCOT=off|static|animated|auto.
Text fallback is expected without confirmed Kitty/Sixel support.
Ctrl+P commands; F1 help; Alt+Enter / Ctrl+O multiline; idle empty Ctrl+D exits.
SIGTERM/SIGHUP gracefully restore an available terminal and exit 1.
No native Windows IPC support. Linux was not built/tested for this artifact.
Ad-hoc linker signature only; not Developer ID signed/notarized.
Read DISTRIBUTION_GATES.md and THIRD_PARTY_NOTICES.md before redistribution.
No global install, shell changes or network access is required by this package.
This archive is not a standalone AI runtime or a public release.
''')
 inventory=[]
 for f in sorted(stage.rglob('*')):
  if f.is_symlink():raise ValueError('Symlinks are not allowed')
  if f.is_file():inventory.append({'path':str(f.relative_to(stage)),'bytes':f.stat().st_size,'sha256':digest(f),'mode':'0755' if f.relative_to(stage)==Path('bin/sentinel') else '0644'})
 (stage/'INVENTORY.json').write_text(json.dumps({'version':version,'architecture':identity,'executable_sha256':digest(binary),'files':inventory,'excludes_self':True},indent=2)+'\n')
 (stage/'SHA256SUMS').write_text(''.join(f"{digest(f)}  {f.relative_to(stage)}\n" for f in sorted(stage.rglob('*')) if f.is_file() and f.name!='SHA256SUMS'))
 archive=out/(name+'.tar.gz')
 with archive.open('wb') as raw,gzip.GzipFile(filename='',fileobj=raw,mode='wb',mtime=0) as gz,tarfile.open(fileobj=gz,mode='w',format=tarfile.USTAR_FORMAT) as tar:
  for f in [stage,*sorted(stage.rglob('*'))]:
   info=tar.gettarinfo(str(f),arcname=str(Path(name)/f.relative_to(stage)));info.uid=info.gid=0;info.uname=info.gname='';info.mtime=0;info.mode=0o755 if f.is_dir() or f==stage/'bin/sentinel' else 0o644
   if f.is_file():
    with f.open('rb') as src:tar.addfile(info,src)
   else:tar.addfile(info)
 (out/'SHA256SUMS').write_text(f'{digest(archive)}  {archive.name}\n')
 summary={'archive':archive.name,'archive_sha256':digest(archive),'archive_bytes':archive.stat().st_size,'unpacked_bytes':sum(f.stat().st_size for f in stage.rglob('*') if f.is_file()),'files':sum(f.is_file() for f in stage.rglob('*')),'executable_sha256':digest(binary),'version':version,'third_party_packages':len(audit),'missing_license_texts':missing,'reproducibility':'sorted normalized archive metadata; independent bit-for-bit build reproducibility not verified'}
 (out/'package-summary.json').write_text(json.dumps(summary,indent=2)+'\n');print(json.dumps(summary,indent=2))
if __name__=='__main__':main()
