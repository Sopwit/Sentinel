"""No-input wire capture for the independent direct Ratatui/Kitty example."""
import re
import os, pty, fcntl, termios, struct, subprocess, select, time, json
from pathlib import Path
root=Path(__file__).resolve().parents[3]
m,s=pty.openpty()
fcntl.ioctl(s,termios.TIOCSWINSZ,struct.pack('HHHH',50,180,2880,1700))
before=termios.tcgetattr(s)
env=os.environ.copy()
for key in ['NO_COLOR','TMUX','STY','SSH_TTY','SSH_CONNECTION','SENTINEL_TUI_ASCII']: env.pop(key,None)
env['TERM']='xterm-256color'
import sys
direct='--direct' in sys.argv
p=subprocess.Popen([str(root/'cli/target/release/examples/kitty_first_paint')]+(['--direct'] if direct else []),stdin=s,stdout=s,stderr=s,env=env,start_new_session=True)
data=bytearray(); answered=False; start=time.monotonic(); first=None
while time.monotonic()-start<10:
    if select.select([m],[],[],.05)[0]:
        data.extend(os.read(m,65536))
        if not answered and b'_Gi=31' in data:
            os.write(m,b'\x1b_Gi=31;OK\x1b\\\x1b[?1;2c\x1b[6;34;16t\x1b[0n');answered=True
        if first is None and (b'a=t,' if direct else b'a=T,U=1') in data: first=(time.monotonic()-start)*1000
    if p.poll() is not None: break
if p.poll() is None: p.kill();p.wait()
after=termios.tcgetattr(s)
pending=bool(after[3] & termios.PENDIN)
after[3] &= ~termios.PENDIN
before[3] &= ~termios.PENDIN
headers=[h.decode('ascii') for h in re.findall(rb'\x1b_G([^;\x1b]{1,256})', data) if b'a=' in h and b'a=q' not in h]
r={'scope':'independent direct Ratatui/Kitty; simulated capability response, NOT physical Ghostty','exit':p.returncode,'keyboard_input':False,'graphics_headers':headers,'direct_cursor_anchor_emitted':b'\x1b[6;6H' in data,'placement_rectangle':[5,5,16,8],'first_upload_ms':first,'variant':'B-direct' if direct else 'A-unicode','uploads':data.count(b'a=t,' if direct else b'a=T,U=1'),'direct_placements':data.count(b'a=p,'),'owned_deletes':data.count(b'a=d,d=I'),'placeholders':data.count('\U0010eeee'.encode()),'restored_alt':b'\x1b[?1049l' in data,'restored_termios_excluding_kernel_PENDIN':after==before,'kernel_PENDIN':pending}
assert r['exit']==0 and r['uploads']==1 and (r['direct_placements']==1 and r['placeholders']==0 if direct else r['placeholders']>0 and r['direct_placements']==0) and r['restored_alt'] and r['restored_termios_excluding_kernel_PENDIN'],r
Path(__file__).with_name('B-direct-results.json' if direct else 'A-unicode-results.json').write_text(json.dumps(r,indent=2)+'\n')
print(json.dumps(r))
os.close(m);os.close(s)
