"""Offline deterministic PTY acceptance; never starts daemon/model/Agent work."""
import os, pty, fcntl, termios, struct, select, time, pathlib, json, signal, hashlib
root = pathlib.Path(__file__).resolve().parents[3]
exe = root / 'cli/target/release/sentinel'
out = pathlib.Path(__file__).resolve().parent
results = []
for fallback in [False, True]:
 for w,h in [(80,24),(100,30),(120,40),(160,48)]:
  pid, fd = pty.fork()
  if pid == 0:
   fcntl.ioctl(0, termios.TIOCSWINSZ, struct.pack('HHHH',h,w,0,0))
   os.environ['TERM']='xterm-256color'
   if fallback:
    os.environ['NO_COLOR']='1'; os.environ['SENTINEL_TUI_ASCII']='1'
   os.execv(str(exe), [str(exe),'tui','--socket','/tmp/sentinel-v33a-intentionally-absent.sock'])
  capture = bytearray()
  def drain():
   end = time.monotonic()+0.3
   while time.monotonic()<end:
    if select.select([fd],[],[],0.03)[0]:
     try: capture.extend(os.read(fd,65536))
     except OSError: break
  def send(data):
   os.write(fd,data); drain()
  drain()
  send(b'\x10') # palette
  send(b'\x1b') # dismiss
  send(b'\x1b[200~'+ '/literal\r\nTürkçe'.encode()+b'\x1b[201~')
  send(b'\x0f') # Ctrl+O newline fallback
  send(b'draft')
  send(b'\x03') # preserve idle draft
  name=f'pty-{w}x{h}'+('-fallback' if fallback else '')
  (out/(name+'.ansi')).write_bytes(capture)
  # Clear multiline draft through supported readline keys, then exit idle/empty.
  send(b'\x15\x7f\x15\x7f\x15\x04')
  exited = False
  for _ in range(20):
   got,status=os.waitpid(pid,os.WNOHANG)
   if got:
    exited = os.waitstatus_to_exitcode(status)==0;break
   drain()
  if not exited:
   os.kill(pid,signal.SIGTERM); os.waitpid(pid,0)
  os.close(fd)
  results.append({'size':[w,h],'fallback':fallback,'clean_exit':exited,'capture':name+'.ansi'})
(out/'pty-results.json').write_text(json.dumps({'executable':str(exe),'sha256':hashlib.sha256(exe.read_bytes()).hexdigest(),'cases':results},indent=2))
assert all(r['clean_exit'] for r in results),results
