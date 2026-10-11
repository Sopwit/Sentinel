"""Existing Core Warden matrix, redirected into this phase's evidence directory."""
import importlib.util,json
from pathlib import Path
HERE=Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location('cw',HERE.parent/'core-warden-cw045/production_pty.py')
cw=importlib.util.module_from_spec(spec);spec.loader.exec_module(cw)
cw.OUT=HERE/'mascot';cw.OUT.mkdir(exist_ok=True)
rows=[cw.run('offline-cold',connected=False),cw.run('offline-warm',connected=False),cw.run('offline-static','static',connected=False),cw.run('offline-auto','auto',connected=False),cw.run('animated'),cw.run('static','static'),cw.run('off','off'),cw.run('auto','auto'),cw.run('populated',populated=True),cw.run('timeout',respond=False),cw.run('missing',extra={'SENTINEL_CORE_WARDEN_ASSETS':'/private/tmp/v33c-missing-assets'}),cw.run('no-color',extra={'NO_COLOR':'1'}),cw.run('ssh',extra={'SSH_TTY':'fixture'}),cw.run('tmux',extra={'TMUX':'fixture'}),cw.run('scenario',scenario=True)]
(HERE/'mascot-results.json').write_text(json.dumps(rows,indent=2)+'\n')
