"""Production candidate sample; compare with recorded V3.3-C measurements, not a fresh paired benchmark."""
import importlib.util,json
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
s=importlib.util.spec_from_file_location('perf',HERE.parent/'tui-v33c/performance.py');p=importlib.util.module_from_spec(s);s.loader.exec_module(p);p.HERE=HERE
rows=[]
for label,exe in [('v4b',HERE/'artifacts/sentinel-cli-1.0.0-rc.8-macos-arm64-evaluation/bin/sentinel')]:
 for mode,long in [('off',False),('static',False),('animated',False),('animated',True)]:
  r=p.run(exe,mode,long,cycles=long);r['label']=label;rows.append(r);print(json.dumps(r),flush=True);(HERE/'performance-results.json').write_text(json.dumps(rows,indent=2)+'\n')
