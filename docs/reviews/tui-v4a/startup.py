"""Production CLI exit/IPC contracts and TUI startup-failure cleanup."""
import os,pty,termios,subprocess,json,socket,tempfile,threading
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];EXE=ROOT/'cli/target/release/sentinel';rows=[]
def check(name,args,code,sock=None):
 p=subprocess.run([str(EXE)]+(['--socket',str(sock)] if sock else [])+args,cwd='/private/tmp',capture_output=True,timeout=12)
 r={'name':name,'exit':p.returncode,'stdout':p.stdout.decode(),'stderr':p.stderr.decode()};assert p.returncode==code,r;rows.append(r)
check('version',['--version'],0);check('help',['--help'],0);check('invalid-command',['unknown-command'],2)
check('missing-daemon',['status'],3,'/private/tmp/sentinel-v4a-absent.sock')
check('tui-invalid-output',['tui','--output','json'],2)
check('empty-run',['run'],2)
for kind,code in [('valid',0),('malformed',4),('incompatible',4),('array-payload',4),('missing-fields',4),('oversized',4),('peer-closed',3)]:
 with tempfile.TemporaryDirectory(prefix='v4a-cli-',dir='/private/tmp') as td:
  path=Path(td)/'socket';ls=socket.socket(socket.AF_UNIX);ls.bind(str(path));ls.listen();errors=[]
  def serve():
   try:
    c,_=ls.accept()
    with c:
     f=c.makefile('rb')
     while line:=f.readline():
      req=json.loads(line);n=req['name']
      if kind=='peer-closed':break
      if kind=='malformed':c.sendall(b'not json\n');break
      if kind=='oversized':c.sendall(b'x'*262145+b'\n');break
      payload={'major':1,'minor':1,'daemon_version':'release-fixture','capabilities':[]} if n=='hello' else {'running':True,'daemon_version':'release-fixture','uptime_ms':0,'active_runs':0,'sessions':0}
      if kind=='array-payload':payload=[]
      if kind=='missing-fields':payload={}
      c.sendall((json.dumps({'version':{'major':2 if kind=='incompatible' else 1,'minor':1},'type':'response','id':req['id'],'name':n,'payload':payload})+'\n').encode())
   except (BrokenPipeError,ConnectionResetError):pass
   except Exception as e:errors.append(str(e))
  t=threading.Thread(target=serve,daemon=True);t.start();check(kind,['status','--output','json'],code,path);ls.close();t.join(timeout=1);assert not errors,errors
m,s=pty.openpty();before=termios.tcgetattr(s);r,w=os.pipe();os.close(r)
p=subprocess.Popen([str(EXE),'--socket','/private/tmp/v4a-absent.sock','tui'],stdin=s,stdout=w,stderr=subprocess.PIPE);os.close(w);_,err=p.communicate(timeout=3)
after=termios.tcgetattr(s);pending=after[3]&getattr(termios,'PENDIN',0);after[3]&=~getattr(termios,'PENDIN',0);before[3]&=~getattr(termios,'PENDIN',0)
r={'name':'tui-broken-output','exit':p.returncode,'macos_pending_retype_flag':bool(pending),'termios_restored':after==before,'stderr':err.decode()};assert r['termios_restored'] and r['exit']==1 and 'Terminal initialization failed' in r['stderr'],r;rows.append(r);os.close(m);os.close(s)
(HERE/'startup-results.json').write_text(json.dumps(rows,indent=2)+'\n');print(len(rows),'startup/CLI cases PASS')
