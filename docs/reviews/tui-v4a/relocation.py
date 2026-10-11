import importlib.util,shutil,tempfile,json,subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
s=importlib.util.spec_from_file_location('compat',HERE/'compatibility.py');c=importlib.util.module_from_spec(s);s.loader.exec_module(c);original=c.subprocess.Popen
rows=[]
with tempfile.TemporaryDirectory(prefix='v4a-relocation-',dir='/private/tmp') as td:
 td=Path(td);stage=HERE/'rc-stage';moved=td/'moved';shutil.copytree(stage,moved);alone=td/'alone';alone.mkdir();shutil.copy2(stage/'bin/sentinel',alone/'sentinel')
 for name,exe,extra,valid in [('installed',stage/'bin/sentinel',{},True),('relocated',moved/'bin/sentinel',{},True),('binary-only',alone/'sentinel',{},False),('override-missing',moved/'bin/sentinel',{'SENTINEL_CORE_WARDEN_ASSETS':str(td/'missing')},False),('checkout-outside-cwd',ROOT/'cli/target/release/sentinel',{},True)]:
  c.EXE=exe;c.subprocess.Popen=lambda *a,**kw:original(*a,cwd=str(td),**kw)
  row=c.run(name,[(.01,b'\x1b_Gi=31;OK\x1b\\'),(.02,b'\x1b[6;16;8t')],'kitty',extra,120,40)
  records=[json.loads(x) for x in (HERE/f'{name}-diagnostics.jsonl').read_text().splitlines()];startup=records[0];row['asset_root']=startup['asset_root'];row['assets']=startup['assets'];assert ('Ok' in startup['assets'])==valid,row
  if valid:assert max(x.get('frames_loaded',0) for x in records)>0
  else:assert all(x.get('frames_loaded',0)==0 for x in records)
  rows.append(row)
(HERE/'relocation-results.json').write_text(json.dumps(rows,indent=2)+'\n');print(len(rows),'relocation cases PASS')
