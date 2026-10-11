"""Unix PTY fixture measurements. Simulated capabilities are NOT physical terminal acceptance."""
import os,pty,fcntl,termios,struct,select,time,json,signal,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[4]
BINARY=ROOT/'cli/crates/sentinel-mascot/target/release/examples/playback'
OUT=ROOT/'docs/reviews/core-warden-cw04';OUT.mkdir(parents=True,exist_ok=True)
def run(name,width,height,mode='auto',respond=True,extra=None,scenario=False,seconds=None):
 master,slave=pty.openpty();fcntl.ioctl(slave,termios.TIOCSWINSZ,struct.pack('HHHH',height,width,width*8,height*16));before=termios.tcgetattr(slave)
 metrics=OUT/(name+'.json');env=os.environ.copy()
 for key in ['TMUX','STY','SSH_CONNECTION','SSH_TTY','NO_COLOR','SENTINEL_TUI_ASCII','SENTINEL_TUI_REDUCED_MOTION','TERM_PROGRAM','KITTY_WINDOW_ID']:env.pop(key,None)
 env.update(TERM='xterm-256color',SENTINEL_TUI_MASCOT=mode);env.update(extra or {})
 seconds=seconds if seconds is not None else (5 if scenario else 3)
 pid=os.fork()
 if pid==0:
  os.close(master);os.setsid()
  for fd in [0,1,2]:os.dup2(slave,fd)
  if slave>2:os.close(slave)
  os.execve(str(BINARY),[str(BINARY),'--seconds',str(seconds),'--metrics',str(metrics)],env)
 start=time.monotonic();data=bytearray();answered=False;sent=False;echo=None;sent_at=None;actions=[]
 if scenario:actions=[(.7,b'm'),(1.0,b'\x1b'),(1.4,b'2'),(2.1,b'3'),(2.8,b'p'),(3.3,b'p'),(3.7,'resize'),(4.2,'restore')]
 actions_done=0
 while time.monotonic()-start<seconds+5:
  elapsed=time.monotonic()-start
  if not sent and elapsed>=.45:
   os.write(master,'é'.encode());sent=True;sent_at=time.monotonic()
  while actions_done<len(actions) and elapsed>=actions[actions_done][0]:
   action=actions[actions_done][1];actions_done+=1
   if action in ['resize','restore']:
    w,h=(60,20) if action=='resize' else (width,height);fcntl.ioctl(slave,termios.TIOCSWINSZ,struct.pack('HHHH',h,w,w*8,h*16));os.kill(pid,signal.SIGWINCH)
   else:os.write(master,action)
  if select.select([master],[],[],.01)[0]:
   try:chunk=os.read(master,65536)
   except OSError:break
   if not chunk:break
   data.extend(chunk)
   if respond and not answered and b'_Gi=31' in data:
    os.write(master,b'\x1b_Gi=31;OK\x1b\\\x1b[?1;2c\x1b[6;16;8t\x1b[0n');answered=True
   if sent and echo is None and 'é'.encode() in chunk:echo=(time.monotonic()-sent_at)*1000
  done,status,usage=os.wait4(pid,os.WNOHANG)
  if done:
   # Collect CPU and maximum resident memory for this child only.
   exit_status=os.waitstatus_to_exitcode(status);break
 else:
  os.kill(pid,signal.SIGTERM);os.waitpid(pid,0);exit_status=-1
 # The slave stays open so terminal flags can be inspected after the child restores it.
 restored=termios.tcgetattr(slave)==before
 os.close(master);os.close(slave)
 result=json.loads(metrics.read_text()) if metrics.exists() else {'missing_metrics':True}
 result.update(cpu_seconds=usage.ru_utime+usage.ru_stime,cpu_percent_one_core=100*(usage.ru_utime+usage.ru_stime)/(time.monotonic()-start),peak_rss_bytes=usage.ru_maxrss if sys.platform=='darwin' else usage.ru_maxrss*1024,name=name,dimensions=[width,height],capability_source='simulated Kitty + cell response' if respond and answered else 'no image capability response',bytes_written=len(data),kitty_transmit_packets=data.count(b'a=T,U=1'),kitty_deletions=data.count(b'a=d,d=I'),alternate_screen_restored=b'\x1b[?1049l' in data,termios_restored=restored,input_to_first_visible_echo_ms=echo,exit_status=exit_status)
 # Evidence retains counts and safe fixture text, not a raw megabyte-scale escape recording.
 metrics.write_text(json.dumps(result,indent=2));print(name,result,flush=True);return result
if '--scenario' in sys.argv:
 result=run('overlay-resize-states',120,40,scenario=True)
 path=OUT/'pty-matrix.json'
 rows=json.loads(path.read_text());rows=[result if r['name']==result['name'] else r for r in rows];path.write_text(json.dumps(rows,indent=2))
 assert result['exit_status']==0 and result['termios_restored']
 sys.exit(0)
if '--steady' in sys.argv:
 results=[run('steady-off-80x24',80,24,mode='off',seconds=12),run('steady-animated-80x24',80,24,seconds=12)]
 (OUT/'steady-idle.json').write_text(json.dumps(results,indent=2))
 assert all(r['exit_status']==0 and r['termios_restored'] for r in results)
 sys.exit(0)
results=[]
for width,height in [(80,24),(100,30),(120,40),(160,48)]:results.append(run(f'animated-{width}x{height}',width,height))
for mode in ['off','static']:results.append(run(mode+'-80x24',80,24,mode))
results.append(run('probe-timeout',80,24,respond=False))
for key in ['NO_COLOR','SENTINEL_TUI_ASCII','SSH_TTY','TMUX','SENTINEL_TUI_REDUCED_MOTION']:
 results.append(run(key.lower(),80,24,extra={key:'1'}))
results.append(run('overlay-resize-states',120,40,scenario=True))
(OUT/'pty-matrix.json').write_text(json.dumps(results,indent=2))
assert all(r['exit_status']==0 and r['alternate_screen_restored'] and r['termios_restored'] for r in results)
assert all(r['protocol']=='kitty' for r in results[:4])
assert all(r['protocol']=='text' for r in results if r['name'] in ['probe-timeout','no_color','sentinel_tui_ascii','ssh_tty','tmux'])
