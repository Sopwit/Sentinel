import sys,os,json,time,select,hashlib,difflib,pty,fcntl,termios,struct,subprocess
sys.path.insert(0,str(__import__('pathlib').Path(__file__).resolve().parent));import sentinel_v322_accept as a
from pathlib import Path
out=Path('docs/reviews/agent-v322-2026-10-10');cases=[];ui=None;master=None;raw=b''
fixtures={'calc.h':'#pragma once\nint add(int a, int b);\n','calc.cpp':'#include "calc.h"\nint add(int a, int b) { return a - b; }\n','test.cpp':'#include "calc.h"\n#include <iostream>\nint main() { if (add(2,3) != 4) { std::cerr << "expected 4 got " << add(2,3) << "\\n"; return 1; } return 0; }\n','unrelated.txt':'USER_CHANGE_PRESERVE_V322\n','README.md':'Synthetic arithmetic project. add must return the sum of two integers. Marker PORCELAIN-822.\n','config.json':'{"standard":20,"marker":"GLACIER-822"}\n','CMakeLists.txt':'cmake_minimum_required(VERSION 3.20)\nproject(V322 LANGUAGES CXX)\nset(CMAKE_CXX_STANDARD 20)\nadd_executable(check calc.cpp test.cpp)\nenable_testing()\nadd_test(NAME arithmetic COMMAND check)\n'}
def save():
 (out/'live.json').write_text(json.dumps({'root':a.root,'provider':'lm-studio','model':'nvidia/nemotron-3-nano-4b','context_window':8192,'cases':cases,'results':a.results,'events':a.events},indent=2))
def drain(t=.05):
 global raw
 end=time.monotonic()+t
 while master is not None and time.monotonic()<end:
  if select.select([master],[],[],.01)[0]:
   try:raw+=os.read(master,65536)
   except OSError:break

def capture(label,stage):drain();(out/(label+'-'+stage+'.ansi')).write_bytes(raw)
def snapshot(root):
 result={}
 for p in Path(root).rglob('*'):
  if p.is_file() and not p.is_symlink():
   data=p.read_bytes();result[str(p.relative_to(root))]={'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest(),'text':data.decode('utf-8',errors='replace') if len(data)<100000 and b'\0' not in data else None}
 return result

def run(label,goal,writes=(),commands=(),bound=180,baseline=False):
 global ui,master,raw
 root=a.root+'/project/'+label;os.makedirs(root)
 for f,c in fixtures.items():Path(root,f).write_text(c)
 if baseline:
  records=[]
  for cmd in [['cmake','-S','.','-B','baseline-build'],['cmake','--build','baseline-build'],['ctest','--test-dir','baseline-build','--output-on-failure']]:
   r=subprocess.run(cmd,cwd=root,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=60,env=dict(os.environ,CCACHE_DISABLE='1'));records.append({'argv':cmd,'returncode':r.returncode,'output':r.stdout})
  (out/(label+'-host-baseline.json')).write_text(json.dumps(records,indent=2));Path(root,'baseline-test.log').write_text(records[-1]['output'])
 before=snapshot(root);sid=a.record(label+'-session','session.create',{'title':'V322 '+label})['payload']['session_id'];wid=a.record(label+'-workspace','workspace.create',{'name':'V322 '+label,'template':'Coding'})['payload']['workspace_id'];a.record(label+'-root','workspace.root',{'workspace_id':wid,'path':root});a.record(label+'-select','workspace.select',{'workspace_id':wid})
 master,slave=pty.openpty();fcntl.ioctl(slave,termios.TIOCSWINSZ,struct.pack('HHHH',40,120,0,0));env=dict(os.environ,TERM='xterm-256color');env.pop('NO_COLOR',None);raw=b'';ui=subprocess.Popen(['cli/target/release/sentinel','--socket',a.path,'tui','--session',sid],stdin=slave,stdout=slave,stderr=slave,env=env,start_new_session=True);os.close(slave)
 drain(1);os.write(master,b'\x07');drain(.2);capture(label,'idle');start_ev=len(a.events);os.write(master,goal.encode()+b'\r');start=time.monotonic();run_id=None;terminal=None;decisions=[];used=set();checked=start_ev;stages=0
 while time.monotonic()-start<bound:
  drain()
  for e in a.events[checked:]:
   p=e.get('payload',{});name=e.get('name')
   if name=='run.started' and p.get('session_id')==sid:run_id=p['run_id']
   if not run_id or p.get('run_id')!=run_id:continue
   if name=='approval.requested':
    tool=p.get('tool');resources=[r.get('resource','') for r in p.get('resources',[])];allow=bool(tool in ('edit-file','write-file') and resources and all(os.path.realpath(x) in [os.path.realpath(root+'/'+f) for f in writes] for x in resources))
    if tool=='run-command':allow=bool(resources and all(x in [hashlib.sha256(c.encode()).hexdigest() for c in commands] for x in resources) and not any(x in used for x in resources));used.update(resources)
    capture(label,'approval-'+str(len(decisions)));os.write(master,b'y' if allow else b'n');decisions.append({'tool':tool,'resources':resources,'allow':allow,'via':'actual TUI y/n'})
   if name in ('tool.result','tool.running'):
    stages+=1;capture(label,name.replace('.','-')+'-'+str(stages))
   if name in ('run.completed','run.failed','run.cancelled'):terminal=e
  checked=len(a.events)
  if terminal:break
  if select.select([a.sock],[],[],.05)[0]:a.read()
 if not terminal:
  capture(label,'timeout');os.write(master,b'\x03');until=time.monotonic()+10
  while time.monotonic()<until:
   drain()
   found=[e for e in a.events if e.get('payload',{}).get('run_id')==run_id and e.get('name') in ('run.completed','run.failed','run.cancelled')]
   if found:terminal=found[-1];break
   if select.select([a.sock],[],[],.05)[0]:a.read()
  outcome='observer-timeout'
 else:outcome=terminal['name']
 drain(.5);capture(label,'final');ui.terminate();ui.wait(5);os.close(master);ui=None;master=None
 after=snapshot(root);diff=''.join(''.join(difflib.unified_diff(before[f]['text'].splitlines(True),after.get(f,{}).get('text','').splitlines(True),fromfile=f,tofile=f)) for f in fixtures if before[f]['text']!=after.get(f,{}).get('text'))
 (out/(label+'.diff')).write_text(diff);a.record(label+'-history','session.messages',{'session_id':sid});a.record(label+'-changes','workspace.changes',{'session_id':sid});case={'label':label,'goal':goal,'root':root,'run_id':run_id,'session_id':sid,'outcome':outcome,'terminal':terminal,'seconds':time.monotonic()-start,'decisions':decisions,'before':before,'after':after,'diff':diff,'unrelated_preserved':before['unrelated.txt']==after['unrelated.txt'],'events':a.events[start_ev:]};cases.append(case);save();print(label,outcome,flush=True)
try:
 a.start();a.results.append(['hello',a.connect()]);a.configure('lm-studio');time.sleep(3);a.record('binding','model.select',{'provider_id':'lm-studio','model_id':'nvidia/nemotron-3-nano-4b'});a.f.close();a.f=a.sock.makefile('rb',buffering=0)
 run('A-read','Inspect calc.h, calc.cpp and test.cpp in the active workspace. Explain how the interface, implementation and test relate, and identify the arithmetic bug. Do not modify files.',bound=180)
 run('B-edit','Fix the arithmetic bug in this C++ project. Inspect the relevant implementation and test, make add return the sum, and update the test to expect add(2,3)==5. Edit only calc.cpp and test.cpp; preserve unrelated.txt. Read the changed files to review the actual result. No shell commands.',writes=('calc.cpp','test.cpp'),bound=300)
 run('C-complete','Fix this C++ project so add returns the sum and its test expects add(2,3)==5. Inspect and edit the implementation and test only; preserve unrelated.txt. Review your actual changes. Verify with cmake -S . -B build, cmake --build build and ctest --test-dir build --output-on-failure. Each named command may be approved once. Report actual build/test evidence and any failure truthfully.',writes=('calc.cpp','test.cpp'),commands=('cmake -S . -B build','cmake --build build','ctest --test-dir build --output-on-failure'),bound=420)
 run('D-recovery','The baseline-test.log records a real failing arithmetic test. Inspect that evidence and the relevant code, correct the implementation and test to assert add(2,3)==5, then review the edited files. Edit only calc.cpp and test.cpp. Preserve unrelated.txt. Report what is and is not verified.',writes=('calc.cpp','test.cpp'),baseline=True,bound=240)
 run('E-pressure','Read README.md, config.json, CMakeLists.txt, calc.h, calc.cpp and test.cpp. Report the two exact project markers and the relationship of the function to the test. Do not modify files. If a file read fails, use a valid scoped path once rather than repeating unchanged calls.',bound=240)
except Exception as e:a.results.append(['harness-error',str(e)]);print(type(e).__name__,str(e),flush=True)
finally:
 save()
 if ui:ui.terminate();ui.wait(5)
 if a.proc and a.proc.poll() is None:a.proc.terminate();a.proc.wait(10)
 a.log.close();print(a.root,flush=True)
