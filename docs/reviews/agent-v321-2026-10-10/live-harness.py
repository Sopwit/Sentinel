import sys,os,json,time,select,hashlib,difflib,threading,http.server,urllib.request,urllib.error
sys.path.insert(0,str(__import__('pathlib').Path(__file__).resolve().parent));import sentinel_v321_accept as a
out='docs/reviews/agent-v321-2026-10-10';metrics=[];cases=[]
class Proxy(http.server.BaseHTTPRequestHandler):
 def log_message(self,*args):pass
 def do_GET(self):self.forward()
 def do_POST(self):self.forward()
 def forward(self):
  data=self.rfile.read(int(self.headers.get('Content-Length',0)));entry=None
  if data:
   b=json.loads(data);entry={'model':b.get('model'),'body_bytes':len(data),'tools_bytes':len(json.dumps(b.get('tools',[])).encode()),'messages_bytes':len(json.dumps(b.get('messages',[])).encode()),'tool_results_bytes':sum(len(str(m.get('content','')).encode()) for m in b.get('messages',[]) if m.get('role')=='tool'),'max_tokens':b.get('max_tokens'),'time':time.time()};metrics.append(entry)
  try:
   r=urllib.request.urlopen(urllib.request.Request('http://127.0.0.1:1234'+self.path,data=data if self.command=='POST' else None,headers={'Content-Type':'application/json'}),timeout=180);body=r.read();status=r.status
  except urllib.error.HTTPError as e:body=e.read();status=e.code
  except Exception as e:body=json.dumps({'error':{'message':str(e)}}).encode();status=502
  if entry:
   try:j=json.loads(body);entry.update({'http_status':status,'usage':j.get('usage'),'error':j.get('error'),'tool_names':[t.get('function',{}).get('name') for c in j.get('choices',[]) for t in c.get('message',{}).get('tool_calls',[])]})
   except ValueError:entry['http_status']=status
  try:self.send_response(status);self.send_header('Content-Type','application/json');self.end_headers();self.wfile.write(body)
  except BrokenPipeError:pass
server=http.server.ThreadingHTTPServer(('127.0.0.1',0),Proxy);threading.Thread(target=server.serve_forever,daemon=True).start()
def save():open(out+'/live.json','w').write(json.dumps({'root':a.root,'provider':'lm-studio','model':'nvidia/nemotron-3-nano-4b','cases':cases,'metrics':metrics,'results':a.results,'events':a.events},indent=2))
def task(label,goal,writes=(),commands=(),cancel=None,bound=180):
 sid=a.record(label+'-new','session.create',{'title':'V321 '+label})['payload']['session_id'];before={f:open(a.root+'/project/'+f).read() for f in ['calc.h','calc.cpp','test.cpp','unrelated.txt']};start=time.monotonic();start_ev=len(a.events);start_metric=len(metrics);r=a.record(label+'-start','agent.start',{'session_id':sid,'text':goal});run=r['payload']['run_id'];terminal=None;decisions=[];checked=start_ev;cancelled=False
 while time.monotonic()-start<bound:
  pending=a.events[checked:];checked=len(a.events)
  for e in pending:
   p=e.get('payload',{});name=e.get('name')
   if p.get('run_id')!=run:continue
   if name in ['run.completed','run.failed','run.cancelled']:terminal=e;break
   if name=='approval.requested':
    tool=p.get('tool');resources=p.get('resources',[]);paths=[x.get('resource','') for x in resources]
    allow=bool(tool in ['edit-file','write-file'] and paths and all(os.path.realpath(x) in [os.path.realpath(a.root+'/project/'+f) for f in writes] for x in paths))
    if tool=='run-command':allow=bool(paths and all(x in [hashlib.sha256(c.encode()).hexdigest() for c in commands] for x in paths))
    if cancel=='approval':a.record(label+'-cancel','run.cancel',{'run_id':run});cancelled=True
    else:decisions.append({'tool':tool,'resources':resources,'allow':allow});a.record(label+'-decision','approval.respond',{'run_id':run,'approval_id':p['approval_id'],'allow':allow})
   if cancel=='process' and name=='tool.running' and p.get('tool')=='run-command' and not cancelled:
    a.record(label+'-cancel','run.cancel',{'run_id':run});cancelled=True
   if cancel=='continuation' and name=='tool.result' and not cancelled:a.record(label+'-cancel','run.cancel',{'run_id':run});cancelled=True
  if terminal:break
  if select.select([a.sock],[],[],.1)[0]:a.read()
 if not terminal:
  a.record(label+'-timeout-cancel','run.cancel',{'run_id':run});terminal={'name':'observer-timeout','payload':{}}
 after={f:open(a.root+'/project/'+f).read() for f in before};diff=''.join(''.join(difflib.unified_diff(before[f].splitlines(True),after[f].splitlines(True),fromfile=f,tofile=f)) for f in before);open(out+'/'+label+'.diff','w').write(diff)
 a.record(label+'-changes','workspace.changes',{'session_id':sid});a.record(label+'-history','session.messages',{'session_id':sid});cases.append({'label':label,'goal':goal,'run_id':run,'session_id':sid,'terminal':terminal,'seconds':time.monotonic()-start,'decisions':decisions,'diff':diff,'unrelated_preserved':before['unrelated.txt']==after['unrelated.txt'],'events':a.events[start_ev:],'metrics':metrics[start_metric:]});save();print(label,terminal['name'],flush=True)
try:
 fixture={'calc.h':'#pragma once\nint add(int a,int b);\n','calc.cpp':'#include "calc.h"\nint add(int a,int b) { return a-b; }\n','test.cpp':'#include "calc.h"\nint main() { return add(2,3)==4 ? 0 : 1; }\n','unrelated.txt':'USER_CHANGE_PRESERVE_821\n','config.json':'{"token":"GLACIER-821","standard":20}\n','README.md':'Build standard is C++20. Project marker PORCELAIN-821.\n','CMakeLists.txt':'cmake_minimum_required(VERSION 3.20)\nproject(V321 LANGUAGES CXX)\nset(CMAKE_CXX_STANDARD 20)\nadd_executable(check calc.cpp test.cpp)\nenable_testing()\nadd_test(NAME arithmetic COMMAND check)\n'}
 for f,c in fixture.items():open(a.root+'/project/'+f,'w').write(c)
 a.start();a.results.append(['hello',a.connect()]);a.record('proxy-endpoint','desktop.setting',{'key':'lmStudioEndpoint','value':'http://127.0.0.1:'+str(server.server_port)});a.configure('lm-studio');time.sleep(3);assert a.record('binding','model.select',{'provider_id':'lm-studio','model_id':'nvidia/nemotron-3-nano-4b'})['type']=='response';wid=a.record('workspace','workspace.create',{'name':'V321 synthetic','template':'Coding'})['payload']['workspace_id'];a.record('root','workspace.root',{'workspace_id':wid,'path':a.root+'/project'});a.record('select','workspace.select',{'workspace_id':wid});a.f.close();a.f=a.sock.makefile('rb',buffering=0)
 task('A-context','Inspect config.json and README.md in the active workspace. Report the project token and marker from those two files. Do not modify files.')
 task('B-reasoning','Inspect calc.h, calc.cpp and test.cpp. Trace how the test calls the declared function and explain what the current implementation computes. Do not modify files.')
 task('C-modify','Fix add in calc.cpp to add its arguments, and update test.cpp so add(2,3) must equal 5. Change only these two files, preserve unrelated.txt, and read the edited files to verify.',writes=('calc.cpp','test.cpp'),bound=240)
 task('D-verify','Run exactly cmake -S . -B build, then cmake --build build, then ctest --test-dir build --output-on-failure in the active workspace. Report actual results; do not change source.',commands=('cmake -S . -B build','cmake --build build','ctest --test-dir build --output-on-failure'),bound=240)
 task('E-pressure','Read config.json and README.md, compare their project markers, then inspect CMakeLists.txt and identify the configured C++ standard. Use current observations; do not modify files.')
 task('F-process-cancel','Run exactly cmake -E sleep 20 once in the active workspace using run-command. This is an authorized synthetic cancellation probe. Do not modify files.',commands=('cmake -E sleep 20',),cancel='process')
 task('G-approval-cancel','Run exactly cmake -E sleep 20 once in the active workspace using run-command. This is a synthetic approval-cancellation probe. Do not modify files.',cancel='approval')
 task('H-continuation-cancel','Read config.json and README.md in the active workspace and report their markers. Do not modify files.',cancel='continuation')
except Exception as e:a.results.append(['harness-error',str(e)]);print(type(e).__name__,str(e),flush=True)
finally:
 save()
 if a.proc and a.proc.poll() is None:a.proc.terminate();a.proc.wait(10)
 server.shutdown();a.log.close();print(a.root,flush=True)
