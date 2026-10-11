"""Wire/order checks only. This PTY does not emulate Ghostty painting."""
import fcntl, json, os, pty, re, select, struct, subprocess, tempfile, termios, time
from pathlib import Path
HERE=Path(__file__).resolve().parent
STAGES=['stdout','stdout-alt','raw','raw-alt','ratatui-before','ratatui-after','player']
rows=[]
for stage in STAGES:
    with tempfile.TemporaryDirectory(prefix='warden-lifecycle-') as d:
        d=Path(d);cap=d/'cap.json';trace=d/'trace.jsonl'
        cap.write_text(json.dumps({'kitty_ack':True,'cell':[16,34],'source':'SIMULATED PTY RECEIPT — NOT physical support evidence'}))
        m,s=pty.openpty();fcntl.ioctl(s,termios.TIOCSWINSZ,struct.pack('HHHH',50,180,2880,1700))
        before=termios.tcgetattr(s);env=os.environ.copy()
        for k in ['TMUX','STY','SSH_CONNECTION','SSH_TTY','NO_COLOR','SENTINEL_TUI_ASCII']:env.pop(k,None)
        p=subprocess.Popen([str(HERE/'target/release/warden-terminal-investigation'),'--stage',stage,'--capability',str(cap),'--trace',str(trace)],stdin=s,stdout=s,stderr=s,env=env,start_new_session=True)
        data=bytearray();start=time.monotonic();first_upload=None;first_place=None
        while time.monotonic()-start<12:
            if select.select([m],[],[],.05)[0]:
                chunk=os.read(m,65536);data.extend(chunk)
                if first_upload is None and b'a=t,' in data:first_upload=(time.monotonic()-start)*1000
                if first_place is None and b'a=p,' in data:first_place=(time.monotonic()-start)*1000
            if p.poll() is not None:break
        if p.poll() is None:p.kill();p.wait();raise RuntimeError(stage+' timeout')
        while select.select([m],[],[],.02)[0]:data.extend(os.read(m,65536))
        after=termios.tcgetattr(s);before[3]&=~termios.PENDIN;after[3]&=~termios.PENDIN
        records=[json.loads(x) for x in trace.read_text().splitlines()]
        (HERE/f'{stage}-operations.jsonl').write_text(trace.read_text())
        operations=[r['operation'] for r in records]
        headers=[h.decode('ascii') for h in re.findall(rb'\x1b_G([^;\x1b]{1,256})',data) if b'a=' in h]
        begin=next(r['us'] for r in records if r['operation']=='observation.begin')
        end=next(r['us'] for r in records if r['operation']=='observation.end')
        idle=[r for r in records if begin<r['us']<end and r['operation'].startswith('stdout.')]
        result={'stage':stage,'scope':'PTY bytes only, no physical visibility claim','exit':p.returncode,'keyboard_input_sent':False,'first_upload_ms':first_upload,'first_placement_ms':first_place,'headers':headers,'alternate_entries':data.count(b'\x1b[?1049h'),'alternate_exits':data.count(b'\x1b[?1049l'),'cursor_hide':data.count(b'\x1b[?25l'),'cursor_show':data.count(b'\x1b[?25h'),'global_clears':data.count(b'\x1b[2J'),'termios_restored_excluding_PENDIN':before==after,'output_operations_during_hold':len(idle),'trace_records':len(records),'bytes':len(data)}
        assert p.returncode==0 and before==after and first_place is not None and not idle,result
        assert result['global_clears']==0,result
        assert any('a=d,d=I,' in h for h in headers),result
        if stage in ['stdout','stdout-alt','raw','raw-alt']:assert 'ratatui.terminal.new.begin' not in operations
        if stage=='ratatui-before':assert 'first_draw.begin' not in operations
        if stage=='ratatui-after':assert operations.index('backend.explicit_flush.completed')<operations.index('upload.begin')
        print(json.dumps(result),flush=True);rows.append(result)
        os.close(m);os.close(s)
(HERE/'pty-results.json').write_text(json.dumps(rows,indent=2)+'\n')
