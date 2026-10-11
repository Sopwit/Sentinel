import os,pty,subprocess,select,time,termios,fcntl,json
from pathlib import Path
HERE=Path(__file__).resolve().parent;EXE=HERE.parents[2]/'cli/target/release/examples/input_poll'
rows=[]
for bounded,initial_nonblocking in [(False,False),(True,False),(True,True)]:
 master,slave=pty.openpty()
 if initial_nonblocking:fcntl.fcntl(slave,fcntl.F_SETFL,fcntl.fcntl(slave,fcntl.F_GETFL)|os.O_NONBLOCK)
 before=termios.tcgetattr(slave);flags=fcntl.fcntl(slave,fcntl.F_GETFL)
 p=subprocess.Popen([str(EXE)]+(['--bounded'] if bounded else []),stdin=slave,stdout=slave,stderr=slave)
 data=b'';sent=False;key=False;start=time.monotonic()
 while p.poll() is None and time.monotonic()-start<2:
  if select.select([master],[],[],.005)[0]:data+=os.read(master,4096)
  if b'READY' in data and not sent:os.write(master,b'\x1b[6;16;8t');sent=True
  if time.monotonic()-start>.6 and not key:os.write(master,b'x');key=True
 p.wait(timeout=1)
 if select.select([master],[],[],.02)[0]:data+=os.read(master,4096)
 assert termios.tcgetattr(slave)==before and (fcntl.fcntl(slave,fcntl.F_GETFL)&os.O_NONBLOCK)==(flags&os.O_NONBLOCK)
 row={'adapter':bounded,'initial_nonblocking':initial_nonblocking,'key_recovery_required':key,'output':data.decode(),'restored_termios_and_nonblocking_flag':True};rows.append(row);print(row)
 assert key != bounded
 os.close(master);os.close(slave)
(HERE/'poll-control-results.json').write_text(json.dumps(rows,indent=2)+'\n')
