import os,tempfile,shutil,subprocess,socket,json,time,signal
root=tempfile.mkdtemp(prefix='sentinel-v322-isolated-');os.chmod(root,0o700)
os.mkdir(root+'/bin');os.mkdir(root+'/run');os.chmod(root+'/run',0o700);os.mkdir(root+'/project');os.mkdir(root+'/project/src')
for name,body in [('README.md','Sentinel synthetic acceptance project. Project token: GLACIER-731.\n'),('CMakeLists.txt','cmake_minimum_required(VERSION 3.20)\nproject(SyntheticAcceptance LANGUAGES CXX)\nset(CMAKE_CXX_STANDARD 20)\n'),('src/main.cpp','int main() { return 0; }\n'),('config.json','{"project_token":"GLACIER-731","standard":20}\n')]:open(root+'/project/'+name,'w').write(body)
shutil.copy2(os.environ.get('SENTINEL_V32_DAEMON','build/tests/apps/sentinel-daemon/sentinel-daemon'),root+'/bin/sentinel-daemon');path=root+'/run/daemon.sock';log=open(root+'/daemon.log','w');proc=None;results=[];events=[]
def start():
 global proc
 proc=subprocess.Popen([root+'/bin/sentinel-daemon','--portable','--profile-name','SentinelV322-'+os.path.basename(root),'--socket',path],stdout=log,stderr=log)
 for _ in range(100):
  if os.path.exists(path):return
  if proc.poll() is not None:raise RuntimeError('daemon startup failed')
  time.sleep(.05)
 raise RuntimeError('socket timeout')
def connect():
 global sock,f,seq
 sock=socket.socket(socket.AF_UNIX);sock.settimeout(180);sock.connect(path);f=sock.makefile('rb');seq=0
 return req('hello',{'client_id':'v31-isolated-validation','major':1,'minor':1,'capabilities':[]})
def read():
 x=json.loads(f.readline())
 if x.get('type')=='event':events.append(x)
 return x
def req(name,payload):
 global seq
 seq+=1;id=str(seq);sock.sendall((json.dumps({'version':{'major':1,'minor':1},'type':'request','id':id,'name':name,'payload':payload})+'\n').encode())
 while True:
  x=read()
  if x.get('id')==id:return x

def record(name,command,payload):
 x=req(command,payload);results.append([name,x]);print(name,x.get('type'),flush=True);return x

def waitrun(label,allow=None,cancel=False,seconds=240):
 started=time.monotonic()
 target=next((r['payload']['run_id'] for _,r in reversed(results) if isinstance(r,dict) and r.get('type')=='response' and 'run_id' in r.get('payload',{})),None)
 checked=0
 while time.monotonic()-started<seconds:
  pending=events[checked:];checked=len(events)
  for e in pending:
   if e['payload'].get('run_id')==target and e['name'] in ['run.completed','run.failed','run.cancelled']:
    results.append([label+'-terminal',e]);print(label,e['name'],e['payload'].get('text','')[:200],flush=True);return e
  x=read();name=x.get('name','');p=x.get('payload',{})
  if p.get('run_id')!=target:continue
  if name=='approval.requested':
   print(label,'approval',p.get('tool'),flush=True)
   if allow is not None:
    safe=p.get('tool') in ['read-file','glob','list-directory'] and os.path.realpath(root+'/project') in json.dumps(p.get('resources',[]))
    record(label+'-approval','approval.respond',{'run_id':p['run_id'],'approval_id':p['approval_id'],'allow':bool(allow and safe)})
  if cancel and name=='output.delta':record(label+'-cancel','run.cancel',{'run_id':p['run_id']});cancel=False
  if name in ['run.completed','run.failed','run.cancelled']:
   results.append([label+'-terminal',x]);print(label,name,p.get('text','')[:200],flush=True);return x
 raise TimeoutError(label)

def configure(provider):
 record('provider-'+provider,'provider.select',{'provider_id':provider});record('discover-'+provider,'desktop.action',{'session_id':'','action':'refreshModelDiscovery','arguments':[]})
 for _ in range(20):
  x=req('model.list',{})
  if x.get('payload',{}).get('models'):break
  time.sleep(.25)
 results.append(['catalog-'+provider,x]);return x
