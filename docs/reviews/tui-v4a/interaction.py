"""Reuse V3.3-B production journeys without overwriting their artifacts."""
import importlib.util,json,hashlib
from pathlib import Path
HERE=Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location('journeys',HERE.parent/'tui-v33b/pty_journeys.py')
j=importlib.util.module_from_spec(spec);spec.loader.exec_module(j)
j.OUT=HERE/'journeys';j.OUT.mkdir(exist_ok=True)
rows=[]
for fallback in [False,True]:
    for w,h in [(60,20),(80,24),(100,30),(120,40),(160,48),(180,48)]:
        rows.append(j.run(w,h,fallback))
        (HERE/'interaction-results.json').write_text(json.dumps({'executable':str(j.EXE),'sha256':hashlib.sha256(j.EXE.read_bytes()).hexdigest(),'scope':'production release + disposable IPC/terminal fixtures; not physical acceptance','cases':rows},indent=2)+'\n')
for w,h in [(60,20),(180,48)]:
    rows.append(j.run(w,h,True,True))
(HERE/'interaction-results.json').write_text(json.dumps({'executable':str(j.EXE),'sha256':hashlib.sha256(j.EXE.read_bytes()).hexdigest(),'scope':'production release + disposable IPC/terminal fixtures; not physical acceptance','cases':rows},indent=2)+'\n')
