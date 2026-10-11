"""Production unavailable -> connected -> disconnected -> reconnected, no inference."""
import importlib.util,socket,threading,tempfile,json,os,pty,termios,fcntl,struct,subprocess,select,time,signal
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];EXE=ROOT/'cli/target/release/sentinel'
s=importlib.util.spec_from_file_location('vt',HERE.parent/'tui-v33b/pty_journeys.py');vt=importlib.util.module_from_spec(s);s.loader.exec_module(vt)
with tempfile.TemporaryDirectory(prefix='v4a-recover-',dir='/private/tmp') as td:
 endpoint=Path(td)/'socket';master,slave=pty.openpty();fcntl.ioctl(slave,termios.TIOCSWINSZ,struct.pack('HHHH',30,100,800,480));before=termios.tcgetattr(slave)
 env=os.environ.copy();env['SENTINEL_TUI_MASCOT']='off'
 p=subprocess.Popen([str(EXE),'--socket',str(endpoint),'tui'],stdin=slave,stdout=slave,stderr=slave,env=env,cwd='/private/tmp');data=bytearray();requests=[];checks=[]
 def drain(n):
  end=time.monotonic()+n
  while time.monotonic()<end:
   if select.select([master],[],[],.01)[0]:data.extend(os.read(master,65536))
 def expect(text):
  end=time.monotonic()+4
  while text not in vt.screen(data,100,30) and time.monotonic()<end:drain(.05)
  assert text in vt.screen(data,100,30),(text,vt.screen(data,100,30));checks.append(text)
 stop=threading.Event();first_done=threading.Event();release_first=threading.Event();errors=[]
 def serve():
  try:
   ls=socket.socket(socket.AF_UNIX);ls.bind(str(endpoint));ls.listen();ls.settimeout(.1)
   count=0
   while not stop.is_set():
    try:c,_=ls.accept()
    except socket.timeout:continue
    count+=1
    with c:
     c.settimeout(.1);buf=b''
     while not stop.is_set():
      if count==1 and release_first.is_set():break
      try:chunk=c.recv(65536)
      except socket.timeout:continue
      if not chunk:break
      buf+=chunk
      while b'\n' in buf:
       line,buf=buf.split(b'\n',1);r=json.loads(line);n=r['name'];requests.append(n)
       payload={'hello':{'major':1,'minor':1,'daemon_version':'recovery-fixture','capabilities':[]},'terminal.state':{'properties':{},'providers':[],'workspace':{},'workspaces':[]},'model.list':{'models':[]},'session.list':{'sessions':[{'session_id':'s','title':'Recovery'}]},'terminal.attach':{'session_id':'s','state':'idle','run_id':'','output':'','title':'Recovery'},'session.messages':{'session_id':'s','messages':[{'role':'assistant','content':f'RECOVERY HISTORY {count}'}]}}.get(n)
       assert payload is not None,n
       c.sendall((json.dumps({'version':{'major':1,'minor':1},'type':'response','id':r['id'],'name':n,'payload':payload})+'\n').encode())
     if count==1:first_done.set()
   ls.close()
  except Exception as e:errors.append(str(e))
 try:
  expect('Disconnected');t=threading.Thread(target=serve,daemon=True);t.start();expect('RECOVERY HISTORY 1');release_first.set();expect('Disconnected');expect('RECOVERY HISTORY 2')
  os.write(master,b'\x04');drain(.3);p.wait(timeout=2)
  assert p.returncode==0 and termios.tcgetattr(slave)==before and b'\x1b[?1049l' in data
  assert not errors and not any(x in requests for x in ['chat.send','agent.start'])
  (HERE/'recovery-results.json').write_text(json.dumps({'checks':checks,'no_keyboard_before_reconnection':True,'no_inference':True,'restored_termios':True,'restored_alt':True,'exit':p.returncode,'requests':requests},indent=2)+'\n')
 finally:
  stop.set()
  if p.poll() is None:p.terminate();p.wait(timeout=2);termios.tcsetattr(slave,termios.TCSANOW,before)
  os.close(master);os.close(slave)
