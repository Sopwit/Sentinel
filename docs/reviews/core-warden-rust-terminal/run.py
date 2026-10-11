#!/usr/bin/env python3
"""Physical runner: children inherit the terminal directly, with no PTY relay."""
import datetime, json, os, re, select, subprocess, sys, tempfile, termios, time, tty
from pathlib import Path
HERE=Path(__file__).resolve().parent
BINARY=HERE/'target/release/warden-terminal-investigation'
STAGES=['stdout','stdout-alt','raw','raw-alt','ratatui-before','ratatui-after','player']

def probe():
    fd=os.open('/dev/tty',os.O_RDWR|os.O_NOCTTY)
    saved=termios.tcgetattr(fd)
    data=bytearray(); start=time.monotonic()
    try:
        tty.setraw(fd)
        os.write(fd,b'\x1b_Gi=31,a=q,t=d,f=24,s=1,v=1;AAAA\x1b\\\x1b[16t')
        deadline=start+.3
        while time.monotonic()<deadline:
            if not select.select([fd],[],[],max(0,deadline-time.monotonic()))[0]: break
            data.extend(os.read(fd,256))
            if len(data)>4096: raise RuntimeError('capability response exceeded budget')
            if b'\x1b_Gi=31;OK\x1b\\' in data and re.search(rb'\x1b\[6;(\d+);(\d+)t',data): break
    finally:
        termios.tcsetattr(fd,termios.TCSANOW,saved);os.close(fd)
    cell=re.search(rb'\x1b\[6;(\d+);(\d+)t',data)
    result={'kitty_ack':b'\x1b_Gi=31;OK\x1b\\' in data,'cell':[int(cell[2]),int(cell[1])] if cell else None,'response_bytes':len(data),'elapsed_ms':(time.monotonic()-start)*1000,'source':'actual terminal response; preflight restored before Rust child'}
    if not result['kitty_ack'] or not cell: raise RuntimeError('No confirmed Kitty + cell response; no image tests executed')
    return result

def main():
    if not sys.stdin.isatty() or not sys.stdout.isatty(): raise RuntimeError('Run directly inside Ghostty')
    if any(os.environ.get(k) is not None for k in ['TMUX','STY','SSH_CONNECTION','SSH_TTY','NO_COLOR','SENTINEL_TUI_ASCII']): raise RuntimeError('SSH/multiplexer/NO_COLOR/ASCII safety gate')
    size=os.get_terminal_size()
    if size.columns<80 or size.lines<24: raise RuntimeError('Use a terminal at least 80 columns × 24 rows')
    if not BINARY.is_file(): raise RuntimeError('Build the isolated crate first')
    out=Path(tempfile.mkdtemp(prefix='warden-rust-lifecycle-',dir='/tmp'))
    receipt=probe(); cap=out/'capability.json';cap.write_text(json.dumps(receipt));cap.chmod(0o600)
    print('Isolated Rust lifecycle investigation. Production TUI is not launched.')
    print('For each five-second image observation, touch NO keys. Answer only AFTER cleanup and the question appears.')
    print('The five-second hold measures visibility; it is not a presentation fix. Children inherit this terminal directly.')
    stages=['player-probe','player-poll','player-probe-poll','player-probe-poll-no-status'] if '--extended' in sys.argv else STAGES
    if '--extended' in sys.argv: print('Extended existing-input-path comparison: if a stage has not shown an image or finished after eight seconds, press ONE harmless key and note whether it then appears. Do not press during the first five seconds.')
    rows=[]
    for stage in stages:
        input(f'\nReady for {stage}? Press Enter now, then touch no keys during the image observation. ')
        before=termios.tcgetattr(sys.stdin.fileno())
        child=subprocess.Popen([str(BINARY),'--stage',stage,'--capability',str(cap),'--trace',str(out/f'{stage}.jsonl')])
        try: code=child.wait(timeout=25)
        except subprocess.TimeoutExpired:
            child.terminate();child.wait()
            os.write(sys.stdout.fileno(),b'\x1b[?25h\x1b[?1049l')
            termios.tcsetattr(sys.stdin.fileno(),termios.TCSANOW,before)
            raise RuntimeError(f'{stage}: input-wait hang exceeded 25 seconds; terminal restored; logs: {out}')
        after=termios.tcgetattr(sys.stdin.fileno())
        before[3]&=~termios.PENDIN;after[3]&=~termios.PENDIN
        restored=before==after
        if not restored:
            termios.tcsetattr(sys.stdin.fileno(),termios.TCSANOW,before)
            raise RuntimeError(f'{stage}: terminal settings did not restore; runner restored them and stopped')
        fd=sys.stdin.fileno();saved=termios.tcgetattr(fd);tty.setraw(fd)
        try:
            if select.select([fd],[],[],0)[0]: os.read(fd,4096)
        finally: termios.tcsetattr(fd,termios.TCSANOW,saved)
        answer=input(f'{stage} finished. Was Core Warden visible BEFORE any key? [y/n/?] ').strip().lower()
        rows.append({'stage':stage,'exit':code,'visible_without_input':{'y':True,'n':False}.get(answer),'termios_restored_excluding_PENDIN':restored})
        (out/'physical-results.json').write_text(json.dumps(rows,indent=2)+'\n')
        if code: raise RuntimeError(f'{stage} exited {code}; stopping')
    print('\nPhysical observation records:',out)
    failed=next((r['stage'] for r in rows if r['visible_without_input'] is False),None)
    print('First stage reported absent without input:',failed or 'none (or uncertain observations)')
    print('No correction has been applied to production. Keep this directory for operation-order comparison.')
if __name__=='__main__':
    try: main()
    except (Exception,KeyboardInterrupt) as e: print(f'Investigation stopped: {e}',file=sys.stderr);sys.exit(1)
