"""Actual release + simulated terminal responses. No physical emulator claim."""
import os,pty,fcntl,termios,struct,select,time,json,socket,threading,tempfile,subprocess,signal,hashlib,importlib.util
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];EXE=HERE/'artifacts/sentinel-cli-1.0.0-rc.8-macos-arm64-evaluation/bin/sentinel'
spec=importlib.util.spec_from_file_location('vt',HERE.parent/'tui-v33b/pty_journeys.py');vt=importlib.util.module_from_spec(spec);spec.loader.exec_module(vt)

def run(name,replies,expected,extra=None,w=80,h=24,offline=False):
 with tempfile.TemporaryDirectory(prefix='v34-',dir='/private/tmp') as td:
  td=Path(td);endpoint=td/'fixture.sock';listener=socket.socket(socket.AF_UNIX);listener.bind(str(endpoint));listener.listen();listener.settimeout(.1)
  stop=threading.Event();requests=[];errors=[]
  def serve():
   try:
    while not stop.is_set():
     try:conn,_=listener.accept();break
     except socket.timeout:continue
    else:return
    with conn:
     conn.settimeout(.1);buf=b''
     while not stop.is_set():
      try:chunk=conn.recv(65536)
      except socket.timeout:continue
      if not chunk:break
      buf+=chunk
      while b'\n' in buf:
       line,buf=buf.split(b'\n',1);r=json.loads(line);n=r['name'];requests.append(n)
       payload={'hello':{'major':1,'minor':1,'daemon_version':'terminal-fixture','capabilities':[]},'terminal.state':{'properties':{},'providers':[],'workspace':{},'workspaces':[]},'model.list':{'models':[]},'session.list':{'sessions':[{'session_id':'s','title':'Compatibility'}]},'terminal.attach':{'session_id':'s','title':'Compatibility','state':'idle','run_id':'','output':''},'session.messages':{'session_id':'s','messages':[{'role':'assistant','content':'AUTOMATIC UPDATE Türkçe'}]}}.get(n)
       assert payload is not None,n
       if n=='session.messages':time.sleep(.45) # Fixture latency, never a presentation fix.
       conn.sendall((json.dumps({'version':{'major':1,'minor':1},'type':'response','id':r['id'],'name':n,'payload':payload})+'\n').encode())
   except Exception as e:
    if not stop.is_set():errors.append(repr(e))
  thread=threading.Thread(target=serve,daemon=True);thread.start()
  master,slave=pty.openpty();fcntl.ioctl(slave,termios.TIOCSWINSZ,struct.pack('HHHH',h,w,w*8,h*16));before=termios.tcgetattr(slave)
  env=os.environ.copy()
  for k in ['TMUX','STY','SSH_CONNECTION','SSH_TTY','NO_COLOR','SENTINEL_TUI_ASCII','SENTINEL_TUI_REDUCED_MOTION','SENTINEL_CORE_WARDEN_ASSETS']:env.pop(k,None)
  env.update(TERM='xterm-256color',TERM_PROGRAM='Apple_Terminal',SENTINEL_TUI_MASCOT='animated',SENTINEL_TUI_MASCOT_DIAGNOSTICS=str(td/'diag.jsonl'));env.update(extra or {})
  p=subprocess.Popen([str(EXE),'--socket',str(td/'absent.sock' if offline else endpoint),'tui'],stdin=slave,stdout=slave,stderr=slave,env=env,start_new_session=True)
  data=bytearray();pending=[];queried=False;start=time.monotonic();checks=[]
  def drain(duration):
   nonlocal queried
   end=time.monotonic()+duration
   while time.monotonic()<end:
    while pending and pending[0][0]<=time.monotonic():_,b=pending.pop(0);os.write(master,b)
    if select.select([master],[],[],.005)[0]:
     try:data.extend(os.read(master,65536))
     except OSError:break
     if not queried and b'_Gi=31' in data:
      queried=True;now=time.monotonic();pending.extend((now+delay,b) for delay,b in replies)
  def send(b,delay=.08):os.write(master,b);drain(delay)
  def view():return vt.screen(data,w,h)
  def expect(text):
   assert text in view(),(name,text,view());checks.append(text)
  try:
   drain(1.3)
   deadline=start+3
   while ('Disconnected' if offline else 'AUTOMATIC UPDATE') not in view() and time.monotonic()<deadline:drain(.05)
   expect('Disconnected' if offline else 'AUTOMATIC UPDATE')
   automatic_ms=(time.monotonic()-start)*1000
   assert b'\x1b[5n' not in data
   send(b'\x10');expect('command · Enter');send(b'\x1b')
   send(b'\x1bOP');expect('help · Enter');send(b'\x1b')
   send(b'\x1b[200~'+ 'Türkçe /path\nsecond'.encode()+b'\x1b[201~')
   expect('second');send(b'\x0f');send('üçüncü'.encode());expect('üçüncü')
   send(b'\x03');expect('üçüncü');send(b'\x04');assert p.poll() is None
   for _ in range(4):send(b'\x15\x7f',.02)
   send(b'\x15')
   send(b'\x1b[200~/settings mascot static\x1b[201~\r');send(b'\x1b[200~/settings\x1b[201~\r');expect('static');send(b'\x1b')
   fcntl.ioctl(slave,termios.TIOCSWINSZ,struct.pack('HHHH',30,100,800,480));os.kill(p.pid,signal.SIGWINCH);drain(.2);w,h=100,30
   expect('Disconnected' if offline else 'AUTOMATIC UPDATE')
   send(b'\x04',.25);p.wait(timeout=2)
   records=[json.loads(x) for x in (td/'diag.jsonl').read_text().splitlines()];startup=records[0]
   assert startup['protocol']==expected,startup
   assert p.returncode==0 and termios.tcgetattr(slave)==before and b'\x1b[?1049l' in data
   assert not errors,errors
   assert not any(n in requests for n in ['chat.send','agent.start'])
   if expected=='text':assert b'a=t,' not in data and b'\x1bPq' not in data
   result={'name':name,'initial_size':[int(v) for v in name.rsplit('-',1)[-1].split('x')] if 'x' in name.rsplit('-',1)[-1] else [80,24],'resized_size':[w,h],'offline':offline,'protocol':startup['protocol'],'reason':startup['capability_reason'],'probe_ms':startup['probe_ms'],'probe_sent':queried,'automatic_update_before_input':True,'no_input_deadline_seconds':3,'observation_window_ms':automatic_ms,'checks':checks,'restored_termios':True,'restored_alt':True,'no_inference':True,'status_query_sent':False,'release_sha256':hashlib.sha256(EXE.read_bytes()).hexdigest()}
   (HERE/f'{name}-screen.txt').write_text(view());(HERE/f'{name}-diagnostics.jsonl').write_text('\n'.join(json.dumps(r) for r in records)+'\n');return result
  except Exception:
   (HERE/f'{name}-failure-screen.txt').write_text(view())
   (HERE/f'{name}-failure-requests.json').write_text(json.dumps(requests))
   (HERE/f'{name}-failure-bytes.hex').write_text(data.hex())
   if (td/'diag.jsonl').exists():(HERE/f'{name}-failure-diagnostics.jsonl').write_bytes((td/'diag.jsonl').read_bytes())
   raise
  finally:
   if p.poll() is None:p.terminate();p.wait(timeout=2)
   stop.set();listener.close();thread.join(timeout=1);os.close(master);os.close(slave)

if __name__=='__main__':
 profiles=[
 ('apple-no-images',[(.01,b'\x1b[?1;2c')],'text',{}),
 ('silent',[],'text',{'TERM_PROGRAM':'ghostty'}),
 ('negative',[(.01,b'\x1b_Gi=31;ENOTSUP\x1b\\'),(.02,b'\x1b[6;16;8t')],'text',{'TERM':'xterm-kitty'}),
 ('kitty-fragmented',[(.01,b'\x1b_Gi=31;'),(.02,b'OK\x1b\\'),(.03,b'\x1b[6;16;'),(.04,b'8t')],'kitty',{}),
 ('sixel-fragmented',[(.01,b'\x1b[?1;'),(.02,b'4c'),(.03,b'\x1b[6;16;8t')],'sixel',{}),
 ('late-da-cell',[(.35,b'\x1b[?1;2c'),(.4,b'\x1b[6;16;8t')],'text',{}),
 ('near-budget-kitty',[(.2,b'\x1b_Gi=31;'),(.24,b'OK\x1b\\'),(.28,b'\x1b[6;16;8t')],'kitty',{}),
 ('missing-cell',[(.01,b'\x1b_Gi=31;OK\x1b\\')],'text',{}),
 ('invalid-cell',[(.01,b'\x1b_Gi=31;OK\x1b\\\x1b[6;0;8t')],'text',{}),
 ]
 for key,extra in [('ssh',{'SSH_TTY':'fixture'}),('tmux',{'TMUX':'fixture'}),('screen',{'TERM':'screen-256color'}),('no-color',{'NO_COLOR':'1'}),('ascii',{'SENTINEL_TUI_ASCII':'1'})]:profiles.append((key,[],'text',extra))
 rows=[]
 for w,h in [(60,20),(120,40)]:
  for name,replies,expected,extra in profiles:
   row=run(f'{name}-{w}x{h}',replies,expected,extra,w,h);rows.append(row);print(json.dumps(row),flush=True);(HERE/'compatibility-results.json').write_text(json.dumps(rows,indent=2)+'\n')
 rows.append(run('disconnected',[],'text',{},80,24,True));(HERE/'compatibility-results.json').write_text(json.dumps(rows,indent=2)+'\n')
