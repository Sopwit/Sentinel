"""Reproduce an input wait stalled by a leftover terminal response. No production edits."""
import fcntl,json,os,pty,select,struct,subprocess,tempfile,termios,time
from pathlib import Path
HERE=Path(__file__).resolve().parent
rows=[]
for name,stage,split in [('probe-only','player-probe',True),('poll-only','player-poll',False),('probe-poll-coalesced','player-probe-poll',False),('probe-poll-split','player-probe-poll',True),('probe-poll-no-status','player-probe-poll-no-status',True)]:
 with tempfile.TemporaryDirectory(prefix='warden-fragment-') as d:
  d=Path(d);cap=d/'cap.json';trace=d/'trace.jsonl'
  cap.write_text(json.dumps({'kitty_ack':True,'cell':[16,34],'source':'SIMULATED fragmented terminal response fixture'}))
  m,s=pty.openpty();fcntl.ioctl(s,termios.TIOCSWINSZ,struct.pack('HHHH',50,180,2880,1700));before=termios.tcgetattr(s)
  env=os.environ.copy()
  for k in ['TMUX','STY','SSH_CONNECTION','SSH_TTY','NO_COLOR','SENTINEL_TUI_ASCII']:env.pop(k,None)
  p=subprocess.Popen([str(HERE/'target/release/warden-terminal-investigation'),'--stage',stage,'--capability',str(cap),'--trace',str(trace)],stdin=s,stdout=s,stderr=s,env=env,start_new_session=True)
  data=bytearray();start=time.monotonic();answered=False;tail=False;key=False;first=None;before_key_uploads=None;records=[];tail_at=None
  while time.monotonic()-start<12:
   if select.select([m],[],[],.01)[0]:data.extend(os.read(m,65536))
   if not answered and b'_Gi=31' in data:
    os.write(m,b'\x1b_Gi=31;OK\x1b\\\x1b[6;34;16t'+(b'' if split else b'\x1b[?1;2c\x1b[0n'));answered=True
   if trace.exists():
    lines=trace.read_text().splitlines()
    records=[]
    for line in lines:
     try:records.append(json.loads(line))
     except json.JSONDecodeError:pass
   if split and not tail and any(r['operation']=='player.capability_probe.completed' for r in records):
    os.write(m,b'\x1b[0n' if b'\x1b[5n' in data else b'\x1b[?1;2c');tail=True;tail_at=time.monotonic()-start
   if first is None and b'a=t,' in data:first=(time.monotonic()-start)*1000
   if name=='probe-poll-split' and tail and not key and time.monotonic()-start-tail_at>=1.0:
    before_key_uploads=data.count(b'a=t,')
    os.write(m,b'x');key=True
   if p.poll() is not None:break
  if p.poll() is None:p.kill();p.wait();raise RuntimeError(name+' timeout')
  while select.select([m],[],[],.02)[0]:data.extend(os.read(m,65536))
  after=termios.tcgetattr(s);before[3]&=~termios.PENDIN;after[3]&=~termios.PENDIN
  records=[json.loads(line) for line in trace.read_text().splitlines()]
  (HERE/f'{name}-operations.jsonl').write_text(trace.read_text())
  starts=[r['us'] for r in records if r['operation']=='event.poll.begin' and r['detail']['phase']=='loading']
  ends=[r['us'] for r in records if r['operation']=='event.poll.completed' and r['detail']['phase']=='loading']
  result={'case':name,'scope':'deterministic PTY input-response fixture; NOT physical Ghostty','stage':stage,'exit':p.returncode,'split_tail_response':split,'status_requested':b'\x1b[5n' in data,'key_injected':key,'uploads_before_injected_key':before_key_uploads,'first_upload_ms':first,'first_loading_poll_us':ends[0]-starts[0] if starts and ends else None,'requested_loading_poll_us':next((r['detail']['timeout_us'] for r in records if r['operation']=='event.poll.begin'),None),'restored_termios':before==after,'uploads':data.count(b'a=t,'),'placements':data.count(b'a=p,')}
  assert result['exit']==0 and result['restored_termios'] and result['placements']==1,result
  if name=='probe-poll-split':assert before_key_uploads==0 and result['first_loading_poll_us']>900000,result
  else:assert first is not None and not key,result
  print(json.dumps(result),flush=True);rows.append(result);os.close(m);os.close(s)
(HERE/'fragmented-probe-results.json').write_text(json.dumps(rows,indent=2)+'\n')
