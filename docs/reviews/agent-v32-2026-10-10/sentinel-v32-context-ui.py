import sys,pty,fcntl,termios,struct,select,time,subprocess,os,json
sys.path.insert(0,'/tmp');import sentinel_v32_accept as a
out='docs/reviews/agent-v32-2026-10-10';master,slave=pty.openpty();fcntl.ioctl(slave,termios.TIOCSWINSZ,struct.pack('HHHH',30,100,0,0));data=b'';tui=None

def capture(stage,keys=b'',seconds=.5):
 global data
 if keys:os.write(master,keys)
 end=time.monotonic()+seconds
 while time.monotonic()<end:
  if select.select([master],[],[],.02)[0]:
   try:data+=os.read(master,65536)
   except OSError:break
 open(out+'/context-'+stage+'.ansi','wb').write(data)
try:
 a.start();a.connect();a.configure('lm-studio');time.sleep(3);a.record('select','model.select',{'provider_id':'lm-studio','model_id':'nvidia/nemotron-3-nano-4b'});wid=a.record('workspace-create','workspace.create',{'name':'V31 context sandbox','template':'Coding'})['payload']['workspace_id'];a.record('workspace-root','workspace.root',{'workspace_id':wid,'path':a.root+'/project'});a.record('workspace-select','workspace.select',{'workspace_id':wid})
 env=dict(os.environ,TERM='xterm-256color',SENTINEL_TUI_THEME='glacier');env.pop('NO_COLOR',None);tui=subprocess.Popen(['cli/target/release/sentinel','--socket',a.path,'tui'],stdin=slave,stdout=slave,stderr=slave,env=env,start_new_session=True);os.close(slave);capture('connected',seconds=1);capture('new',b'/new V31 grounded context\r');capture('mode',b'/agent\r');capture('files',b'@');capture('selected',b'CMake\r');capture('second-picker',b'@');capture('multiple-references',b'README\r');capture('references',b'/references\r');capture('close-references',b'\x1b',seconds=.3)
 sessions=a.req('session.list',{})['payload']['sessions'];sid=next(x['session_id'] for x in sessions if x['title']=='V31 grounded context');a.record('observer-attach','terminal.attach',{'session_id':sid});a.f.close();a.f=a.sock.makefile('rb',buffering=0)
 capture('running',('Use read-file to inspect '+a.root+'/project/CMakeLists.txt and report the configured C++ standard. Do not modify files. The standard must come from the file, not a guess.\r').encode());start=time.monotonic();terminal=None
 while time.monotonic()-start<240:
  capture('progress',seconds=.1)
  if select.select([a.sock],[],[],.02)[0]:
   e=a.read();name=e.get('name');p=e.get('payload',{})
   if name=='approval.requested':capture('approval-denied',b'n');a.results.append(['unexpected-approval-denied',e])
   if name in ['tool.requested','tool.result']:print(name,p.get('tool'),flush=True);capture(name)
   if name in ['run.completed','run.failed','run.cancelled']:terminal=e;break
 if terminal is None:raise TimeoutError('context Agent')
 a.results.append(['terminal',terminal]);capture('final',seconds=1);a.record('persisted-history','session.messages',{'session_id':sid});capture('exit',b'\x03');tui.wait(5);a.results.append(['exit',tui.returncode])
except Exception as e:a.results.append(['error',str(e)]);print(type(e).__name__,str(e),flush=True)
finally:
 if tui and tui.poll() is None:tui.terminate();tui.wait(5)
 if a.proc and a.proc.poll() is None:a.proc.terminate();a.proc.wait(10)
 os.close(master);a.log.close();open(out+'/context-ui.json','w').write(json.dumps({'root':a.root,'results':a.results,'events':a.events},indent=2));print(a.root,flush=True)
