"""Actual external signals to the production release; synthetic IPC/Kitty only."""
import os,pty,termios,fcntl,struct,subprocess,signal,select,time,json,socket,threading,tempfile,re,hashlib
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];EXE=ROOT/'cli/target/release/sentinel'
def run(name,stage,mode='animated',connected=False,sig=signal.SIGTERM,repeat=False):
 with tempfile.TemporaryDirectory(prefix='v4a1-signal-',dir='/private/tmp') as td:
  endpoint=Path(td)/'socket';stop=threading.Event();requests=[];errors=[];listener=None
  if connected:
   listener=socket.socket(socket.AF_UNIX);listener.bind(str(endpoint));listener.listen();listener.settimeout(.1)
   def serve():
    try:
     while not stop.is_set():
      try:c,_=listener.accept();break
      except socket.timeout:continue
     else:return
     with c:
      c.settimeout(.1);buf=b''
      while not stop.is_set():
       try:chunk=c.recv(65536)
       except socket.timeout:continue
       if not chunk:break
       buf+=chunk
       while b'\n' in buf:
        line,buf=buf.split(b'\n',1);r=json.loads(line);n=r['name'];requests.append(n)
        payload={'hello':{'major':1,'minor':1,'daemon_version':'signal-fixture','capabilities':[]},'terminal.state':{'properties':{},'providers':[],'workspace':{},'workspaces':[]},'model.list':{'models':[]},'session.list':{'sessions':[{'session_id':'s','title':'Signal fixture'}]},'terminal.attach':{'session_id':'s','state':'idle','run_id':'','output':'','title':'Signal fixture'},'session.messages':{'session_id':'s','messages':[]}}.get(n)
        assert payload is not None,n
        c.sendall((json.dumps({'version':{'major':1,'minor':1},'type':'response','id':r['id'],'name':n,'payload':payload})+'\n').encode())
    except Exception as e:
     if not stop.is_set():errors.append(str(e))
   threading.Thread(target=serve,daemon=True).start()
  master,slave=pty.openpty();fcntl.ioctl(slave,termios.TIOCSWINSZ,struct.pack('HHHH',40,120,960,640));before=termios.tcgetattr(slave)
  env=os.environ.copy()
  for k in ['NO_COLOR','TMUX','STY','SSH_TTY','SSH_CONNECTION','SENTINEL_TUI_ASCII','SENTINEL_TUI_REDUCED_MOTION','SENTINEL_CORE_WARDEN_ASSETS','SENTINEL_TUI_MASCOT_DIAGNOSTICS']:env.pop(k,None)
  env.update(TERM='xterm-256color',SENTINEL_TUI_MASCOT=mode)
  p=subprocess.Popen([str(EXE),'--socket',str(endpoint),'tui'],stdin=slave,stdout=slave,stderr=slave,env=env,start_new_session=True)
  data=bytearray();answered=False
  def drain(seconds,answer=True):
   nonlocal answered
   end=time.monotonic()+seconds
   while time.monotonic()<end:
    if select.select([master],[],[],.002)[0]:
     try:data.extend(os.read(master,262144))
     except OSError:break
     if answer and not answered and b'_Gi=31' in data:
      os.write(master,b'\x1b_Gi=31;OK\x1b\\\x1b[6;16;8t\x1b[?1;2c');answered=True
  def until(predicate,answer=True):
   end=time.monotonic()+4
   while not predicate() and time.monotonic()<end:drain(.002,answer)
   assert predicate(),(name,'milestone not reached',bytes(data[-100:]))
  try:
   if stage=='alt':until(lambda:b'\x1b[?1049h' in data,False)
   elif stage=='enhancements':until(lambda:b'\x1b[?2004h' in data,False)
   elif stage=='probe':until(lambda:b'_Gi=31' in data,False)
   elif stage!='immediate':
    until(lambda:b'a=p,' in data if mode!='off' else b'Start a new conversation' in data)
    if connected and mode in ['animated','auto']:until(lambda:data.count(b'a=p,')>=4)
    if stage=='overlay':os.write(master,b'\x10');drain(.1)
    if stage=='resize':
     fcntl.ioctl(slave,termios.TIOCSWINSZ,struct.pack('HHHH',24,80,640,384));os.kill(p.pid,signal.SIGWINCH)
   sent=time.monotonic();count=0
   for _ in range(10 if repeat else 1):
    try:os.kill(p.pid,sig);count+=1
    except ProcessLookupError:break
   while p.poll() is None and time.monotonic()-sent<3:drain(.005,stage!='probe')
   p.wait(timeout=.5);exit_ms=(time.monotonic()-sent)*1000;drain(.05)
   after=termios.tcgetattr(slave);a=after.copy();b=before.copy();pendin=getattr(termios,'PENDIN',0);a[3]&=~pendin;b[3]&=~pendin
   enabled=b'\x1b[?2004h' in data;entered=b'\x1b[?1049h' in data
   headers=re.findall(rb'\x1b_G([^;\x1b]+)',data)
   uploads={re.search(rb'(?:^|,)i=(\d+)',h).group(1) for h in headers if b'a=t,' in h}
   deletes=set(re.findall(rb'a=d,d=I,i=(\d+)',data))
   r={'name':name,'stage':stage,'mode':mode,'connected':connected,'signal':signal.Signals(sig).name,'signals_sent':count,'exit':p.returncode,'termination_ms':round(exit_ms,2),'termios_restored_except_transient_pendin':a==b,'echo_restored':bool(after[3]&termios.ECHO),'canonical_restored':bool(after[3]&termios.ICANON),'entered_alt':entered,'left_alt':b'\x1b[?1049l' in data,'cursor_restored':b'\x1b[?25l' not in data or data.rfind(b'\x1b[?25h')>data.rfind(b'\x1b[?25l'),'paste_enabled':enabled,'paste_disabled':b'\x1b[?2004l' in data,'keyboard_popped':b'\x1b[<1u' in data,'upload_ids':sorted(x.decode() for x in uploads),'delete_ids':sorted(x.decode() for x in deletes),'owned_images_deleted':uploads<=deletes,'no_global_delete':b'd=A' not in data,'no_status_query':b'\x1b[5n' not in data,'signal_reported':signal.Signals(sig).name.encode() in data,'no_inference':not any(x in requests for x in ['chat.send','agent.start']),'release_sha256':hashlib.sha256(EXE.read_bytes()).hexdigest()}
   assert a==b and r['echo_restored'] and r['canonical_restored'],r
   assert (not entered or r['left_alt']) and (not enabled or r['paste_disabled'] and r['keyboard_popped']) and uploads<=deletes,r
   assert r['no_global_delete'] and r['no_status_query'] and r['no_inference'],r
   assert data.count(b'\x1b[?1049l')<=1 and data.count(b'\x1b[?2004l')<=1,'duplicate cleanup'
   if stage!='immediate':assert p.returncode==1 and r['signal_reported'] and r['cursor_restored'],r
   else:assert p.returncode in [1,-sig],r
   if stage!='immediate' and mode!='off' and stage not in ['alt','enhancements','probe']:assert uploads,r
   (HERE/(name+'-operations.json')).write_text(json.dumps({'kitty_headers':[h.decode() for h in headers[:256]],'total_bytes':len(data),'alt_leave_count':data.count(b'\x1b[?1049l'),'paste_disable_count':data.count(b'\x1b[?2004l')},indent=2)+'\n');print(json.dumps(r),flush=True);return r
  finally:
   stop.set()
   if p.poll() is None:p.kill();p.wait()
   termios.tcsetattr(slave,termios.TCSANOW,before);os.close(master);os.close(slave)
   if listener:listener.close()
rows=[]
for i in range(5):rows.append(run(f'immediate-{i}','immediate'))
for stage in ['alt','enhancements','probe','idle','overlay','resize']:
 rows.append(run(stage,stage))
for mode in ['off','static','animated','auto']:rows.append(run('connected-'+mode,'idle',mode,True))
rows.append(run('repeated','probe',repeat=True));rows.append(run('hup','idle',sig=signal.SIGHUP));rows.append(run('hup-overlay','overlay',connected=True,sig=signal.SIGHUP))
(HERE/'signal-results.json').write_text(json.dumps(rows,indent=2)+'\n')
