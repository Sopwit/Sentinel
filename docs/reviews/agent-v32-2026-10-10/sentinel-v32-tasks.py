import sys,json,os,time,select,difflib
sys.path.insert(0,'/tmp');import sentinel_v32_accept as a
out='docs/reviews/agent-v32-2026-10-10';cases=[]
def save():
 open(out+'/natural-tasks.json','w').write(json.dumps({'root':a.root,'provider':'lm-studio','model':'nvidia/nemotron-3-nano-4b','cases':cases,'results':a.results,'events':a.events},indent=2))
def task(label,goal,target=None,commands=(),cancel_running=False):
 sid=a.record(label+'-new','session.create',{'title':'V32 '+label})['payload']['session_id'];start=time.monotonic();event_start=len(a.events);r=a.record(label+'-start','agent.start',{'session_id':sid,'text':goal});run=r.get('payload',{}).get('run_id');terminal=None;decisions=[];before={f:open(a.root+'/project/'+f).read() for f in ['math.h','main.cpp','test_main.cpp','unrelated.txt']};deadline=time.monotonic()+300
 while time.monotonic()<deadline:
  if not select.select([a.sock],[],[],.2)[0]:continue
  e=a.read();p=e.get('payload',{});name=e.get('name')
  if p.get('run_id')!=run:continue
  if name=='approval.requested':
   resources=p.get('resources',[]);tool=p.get('tool');paths=[x.get('resource','') for x in resources]
   safe_write=tool in ['write-file','edit-file'] and target and paths and all(os.path.realpath(x)==os.path.realpath(a.root+'/project/'+target) for x in paths)
   safe_command=tool=='run-command' and commands and paths and all(x in commands or os.path.realpath(x)==os.path.realpath(a.root+'/project') for x in paths) and any(x in commands for x in paths)
   allow=bool(safe_write or safe_command);decisions.append({'tool':tool,'resources':resources,'allow':allow});a.record(label+'-decision','approval.respond',{'run_id':run,'approval_id':p['approval_id'],'allow':allow})
  if name=='tool.running' and p.get('tool')=='run-command' and cancel_running:a.record(label+'-cancel','run.cancel',{'run_id':run});cancel_running=False
  if name in ['run.completed','run.failed','run.cancelled']:terminal=e;break
 if not terminal:
  a.record(label+'-timeout-cancel','run.cancel',{'run_id':run});terminal={'name':'observer-timeout','payload':{'state':'not-accepted'}}
 after={f:open(a.root+'/project/'+f).read() for f in before};diff=''.join(''.join(difflib.unified_diff(before[f].splitlines(True),after[f].splitlines(True),fromfile=f,tofile=f)) for f in before)
 open(out+'/'+label+'.diff','w').write(diff);a.record(label+'-changes','workspace.changes',{'session_id':sid});a.record(label+'-history','session.messages',{'session_id':sid});cases.append({'label':label,'goal':goal,'duration_seconds':time.monotonic()-start,'terminal':terminal,'decisions':decisions,'events':a.events[event_start:],'unrelated_preserved':before['unrelated.txt']==after['unrelated.txt'],'diff':diff});save();print(label,terminal['name'],flush=True)
try:
 for f,content in {'math.h':'#pragma once\ninline int add(int a,int b) { return a-b; }\ninline int multiply(int a,int b) { return a*b; }\n','main.cpp':'#include "math.h"\nint main() { return add(2,3); }\n','test_main.cpp':'#include "math.h"\nint main() { return add(2,3)==4 ? 0 : 1; }\n','unrelated.txt':'UNRELATED USER CHANGE: preserve this exactly.\n','CMakeLists.txt':'cmake_minimum_required(VERSION 3.20)\nproject(V32 LANGUAGES CXX)\nset(CMAKE_CXX_STANDARD 20)\nadd_executable(check test_main.cpp)\nenable_testing()\nadd_test(NAME arithmetic COMMAND check)\n'}.items():open(a.root+'/project/'+f,'w').write(content)
 a.start();a.results.append(['hello',a.connect()]);a.configure('lm-studio');time.sleep(3);binding=a.record('binding','model.select',{'provider_id':'lm-studio','model_id':'nvidia/nemotron-3-nano-4b'});assert binding['type']=='response'
 wid=a.record('workspace-create','workspace.create',{'name':'V32 synthetic coding','template':'Coding'})['payload']['workspace_id'];a.record('workspace-root','workspace.root',{'workspace_id':wid,'path':a.root+'/project'});a.record('workspace-select','workspace.select',{'workspace_id':wid});a.record('readiness','terminal.state',{});a.f.close();a.f=a.sock.makefile('rb',buffering=0)
 task('level2-analysis','Inspect math.h and main.cpp in the active workspace. Explain what main calls and compare what add and multiply currently compute. Do not modify files.')
 task('level3-source','Fix only add in math.h so it adds its arguments instead of subtracting. Preserve multiply and every other file including unrelated.txt. Read the changed file to verify the result.',target='math.h')
 task('level4-verification','Change only test_main.cpp so add(2,3) is expected to equal 5. Preserve other files. Read the changed file, then configure, build and test this project using cmake -S . -B build, cmake --build build, and ctest --test-dir build --output-on-failure in the workspace. Report only observed results.',target='test_main.cpp',commands=('cmake -S . -B build','cmake --build build','ctest --test-dir build --output-on-failure'))
 task('level5-missing','Read missing-config.json in the active workspace. If it is missing, inspect config.json and report the actual project token. Do not create or modify files.')
except Exception as e:a.results.append(['error',str(e)]);print(type(e).__name__,str(e),flush=True)
finally:
 save()
 if a.proc and a.proc.poll() is None:a.proc.terminate();a.proc.wait(10)
 a.log.close();print(a.root,flush=True)
