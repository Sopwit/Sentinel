"""Extract actual archive; hash, mode, path safety and production fallback tests."""
import importlib.util,shutil,tempfile,json,subprocess,tarfile,hashlib,os
from pathlib import Path
HERE=Path(__file__).resolve().parent;ART=HERE/'artifacts';NAME='sentinel-cli-1.0.0-rc.8-macos-arm64-evaluation'
s=importlib.util.spec_from_file_location('compat',HERE/'compatibility.py');c=importlib.util.module_from_spec(s);s.loader.exec_module(c);original=c.subprocess.Popen
rows=[]
def hashes(d):return {str(f.relative_to(d)):hashlib.sha256(f.read_bytes()).hexdigest() for f in d.rglob('*') if f.is_file()}
with tempfile.TemporaryDirectory(prefix='Sentinel package with spaces ',dir='/private/tmp') as td:
 td=Path(td)
 with tarfile.open(ART/(NAME+'.tar.gz')) as tar:
  names=tar.getnames();assert len(names)==len(set(names))
  for m in tar.getmembers():assert not Path(m.name).is_absolute() and '..' not in Path(m.name).parts and (m.isfile() or m.isdir()),m.name
  tar.extractall(td,filter='data')
 moved=td/NAME;before=hashes(moved)
 for line in (moved/'SHA256SUMS').read_text().splitlines():
  sha,rel=line.split('  ',1);assert before[rel]==sha
 inventory=json.loads((moved/'INVENTORY.json').read_text())
 for f in inventory['files']:assert before[f['path']]==f['sha256'] and (moved/f['path']).stat().st_size==f['bytes']
 assert subprocess.check_output([str(moved/'bin/sentinel'),'--version'],cwd='/private/tmp',text=True).strip()=='sentinel 1.0.0-rc.8'
 cases=[('spaces-relocated',moved,True,True)]
 for name in ['missing-assets','corrupt-manifest','corrupt-png','traversal','symlink-escape']:
  d=td/name;shutil.copytree(moved,d);root=d/'share/sentinel/core-warden'
  if name=='missing-assets':shutil.rmtree(root)
  if name=='corrupt-manifest':(root/'animations/manifest.json').write_text('invalid json')
  if name=='corrupt-png':
   for png in root.rglob('*.png'):png.write_bytes(b'invalid PNG payload')
  if name=='traversal':
   f=root/'state-contract.json';v=json.loads(f.read_text());v['animation_manifest']='../../outside.json';f.write_text(json.dumps(v))
  if name=='symlink-escape':
   f=root/'animations/idle/96/frame_000.png';f.unlink();external=td/'outside.png';external.write_bytes(b'external');f.symlink_to(external)
  cases.append((name,d,False,name=='corrupt-png'))
 # Read-only is a separate fresh extracted copy, not an environment override.
 readonly=td/'Read Only Installation';shutil.copytree(moved,readonly)
 for f in readonly.rglob('*'):f.chmod(0o555 if f.is_dir() or f.name=='sentinel' else 0o444)
 readonly.chmod(0o555);assert not os.access(readonly,os.W_OK)
 cases.insert(1,('read-only',readonly,True,True))
 try:
  for name,d,frames,assets_valid in cases:
   c.EXE=d/'bin/sentinel';c.subprocess.Popen=lambda *a,**kw:original(*a,cwd=str(td),**kw)
   row=c.run(name,[(.01,b'\x1b_Gi=31;OK\x1b\\'),(.02,b'\x1b[6;16;8t')],'kitty',w=120,h=40)
   records=[json.loads(x) for x in (HERE/f'{name}-diagnostics.jsonl').read_text().splitlines()];startup=records[0]
   assert ('Ok' in startup['assets'])==assets_valid,(name,startup)
   loaded=max(r.get('frames_loaded',0) for r in records);assert (loaded>0)==frames,(name,loaded)
   row.update(asset_root=startup['asset_root'],assets=startup['assets'],frames_loaded=loaded,cwd_outside_repository=True,path_contains_spaces=True,read_only=name=='read-only');rows.append(row)
   if name=='read-only':assert hashes(readonly)==before
  assert hashes(moved)==before
 finally:
  for f in readonly.rglob('*'):f.chmod(0o755 if f.is_dir() or f.name=='sentinel' else 0o644)
  readonly.chmod(0o755)
(HERE/'relocation-results.json').write_text(json.dumps({'archive_verified':True,'inventory_verified':True,'no_archive_symlinks_or_traversal':True,'read_only_package_unchanged':True,'cases':rows},indent=2)+'\n');print('7 relocated package cases PASS')
