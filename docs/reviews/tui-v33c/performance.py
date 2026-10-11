"""macOS production-process CPU/RSS + real PTY bytes, disposable typed IPC fixtures."""
import ctypes,fcntl,hashlib,importlib.util,json,os,pty,re,select,signal,socket,struct,subprocess,sys,tempfile,termios,threading,time
from pathlib import Path
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
class Usage(ctypes.Structure):
    _fields_=[('uuid',ctypes.c_uint8*16)]+[(n,ctypes.c_uint64) for n in ['user','system','idle_wakeups','interrupt_wakeups','pageins','wired','rss','footprint','start','exit','child_user','child_system','child_idle','child_interrupt','child_pageins','child_elapsed','disk_read','disk_write']]
spec=importlib.util.spec_from_file_location('journeys',HERE.parent/'tui-v33b/pty_journeys.py');journeys=importlib.util.module_from_spec(spec);spec.loader.exec_module(journeys)
lib=ctypes.CDLL('/usr/lib/libproc.dylib')
class Timebase(ctypes.Structure):
    _fields_=[('numer',ctypes.c_uint32),('denom',ctypes.c_uint32)]
timebase=Timebase();ctypes.CDLL(None).mach_timebase_info(ctypes.byref(timebase))
seconds_per_tick=timebase.numer/timebase.denom/1e9
def usage(pid):
    u=Usage()
    if lib.proc_pid_rusage(pid,2,ctypes.byref(u))!=0:raise RuntimeError('proc_pid_rusage failed')
    return u

def run(exe,mode,long=False,cycles=False,repeat=0,diagnostics=False):
    with tempfile.TemporaryDirectory(prefix='v33c-perf-',dir='/private/tmp') as td:
        td=Path(td);endpoint=td/'daemon.sock';listener=socket.socket(socket.AF_UNIX);listener.bind(str(endpoint));listener.listen();listener.settimeout(.1)
        stopping=threading.Event();requests=[];errors=[]
        transcript='\n'.join(f'Line {i:04}: **Sentinel** Türkçe response with /paths and bounded rendering. '+('more text '*5) for i in range(1500))+'\nFINAL TRANSCRIPT ROW'
        def serve():
            try:
                while not stopping.is_set():
                    try:conn,_=listener.accept();break
                    except socket.timeout:continue
                else:return
                conn.settimeout(.1)
                with conn:
                    buf=b''
                    while not stopping.is_set():
                        try:chunk=conn.recv(65536)
                        except socket.timeout:continue
                        if not chunk:break
                        buf+=chunk
                        while b'\n' in buf:
                            line,buf=buf.split(b'\n',1);r=json.loads(line);name=r['name'];requests.append(name)
                            payload={
                              'hello':{'major':1,'minor':1,'daemon_version':'performance-fixture','capabilities':[]},
                              'terminal.state':{'properties':{'activeRuntimeProviderLabel':'fixture','activeRuntimeModelLabel':'fixture','activeRuntimeReadinessState':'Available'},'providers':[],'workspace':{'name':'Fixture'},'workspaces':[]},
                              'model.list':{'models':[{'provider_id':'fixture','model_id':'fixture','health':'Available'}]},
                              'session.list':{'sessions':[{'session_id':'s','title':'Performance fixture'}]},
                              'terminal.attach':{'session_id':'s','state':'idle','run_id':'','output':'','title':'Performance fixture'},
                              'session.messages':{'session_id':'s','messages':[{'role':'assistant','content':transcript}] if long else []}
                            }.get(name)
                            assert payload is not None,'Unexpected inference/mutation '+name
                            conn.sendall((json.dumps({'version':{'major':1,'minor':1},'type':'response','id':r['id'],'name':name,'payload':payload})+'\n').encode())
            except Exception as e:
                if not stopping.is_set():errors.append(repr(e))
        thread=threading.Thread(target=serve,daemon=True);thread.start()
        master,slave=pty.openpty();fcntl.ioctl(slave,termios.TIOCSWINSZ,struct.pack('HHHH',48,180,1440,768));before=termios.tcgetattr(slave)
        env=os.environ.copy()
        for key in ['TMUX','STY','SSH_CONNECTION','SSH_TTY','NO_COLOR','SENTINEL_TUI_ASCII','SENTINEL_TUI_REDUCED_MOTION','SENTINEL_CORE_WARDEN_ASSETS','SENTINEL_TUI_MASCOT_DIAGNOSTICS']:env.pop(key,None)
        env.update(TERM='xterm-256color',SENTINEL_TUI_MASCOT=mode)
        if diagnostics:env['SENTINEL_TUI_MASCOT_DIAGNOSTICS']=str(td/'diagnostic.jsonl')
        child=subprocess.Popen([str(exe),'--socket',str(endpoint),'tui'],stdin=slave,stdout=slave,stderr=slave,env=env,start_new_session=True)
        data=bytearray();receipts=[];replied=False;pending=[];peak=0;status=None;ru=None;start=time.monotonic()
        def drain(seconds):
            nonlocal replied,peak,status,ru
            end=time.monotonic()+seconds
            while time.monotonic()<end:
                while pending and time.monotonic()>=pending[0][0]:_,b=pending.pop(0);os.write(master,b)
                if select.select([master],[],[],.005)[0]:
                    try:chunk=os.read(master,262144)
                    except OSError:break
                    data.extend(chunk);receipts.append((time.monotonic(),len(data)))
                    if not replied and b'_Gi=31' in data:
                        assert b'\x1b[5n' not in data
                        now=time.monotonic();pending.extend([(now+.005,b'\x1b_Gi=31;OK\x1b\\'),(now+.01,b'\x1b[6;16;8t'),(now+.03,b'\x1b[?1;2c')]);replied=True
                if status is None:
                    pid,st,r=os.wait4(child.pid,os.WNOHANG)
                    if pid:status=st;ru=r;child.returncode=os.waitstatus_to_exitcode(st)
                if status is not None:break
                peak=max(peak,usage(child.pid).rss)
        try:
            drain(1.5)
            assert status is None and 'session.messages' in requests,errors
            if long:
                view=journeys.screen(data,180,48)
                (HERE/'last-long-screen.txt').write_text(view)
                (HERE/'last-long-requests.json').write_text(json.dumps(requests))
                assert 'FINAL TRANSCRIPT ROW' in view,'long transcript was not accepted/rendered'
            if mode=='animated': assert data.count(b'a=p,')>=3,'animation did not start'
            a=usage(child.pid);offset=len(data);t=time.monotonic();drain(4);wall=time.monotonic()-t;b=usage(child.pid);steady=bytes(data[offset:])
            late_offset=len(data);drain(.5);late=bytes(data[late_offset:])
            assert b'a=t,' not in late,'unchanged warmed animation must not upload again'
            latency=[]
            for key in [b'Z',b'Q',b'V',b'J',b'X']:
                offset=len(data);sent=time.monotonic();os.write(master,key)
                while key not in data[offset:] and time.monotonic()-sent<1:drain(.002)
                assert key in data[offset:],'input did not become visible'
                first=next(ts for ts,size in receipts if ts>=sent and size>offset and key in data[offset:size])
                latency.append((first-sent)*1000);drain(.05)
            os.write(master,b'\x15');drain(.1)
            resize=[]
            for w,h in [(60,20),(80,24),(100,30),(120,40),(160,48),(180,48)]:
                offset=len(data);sent=time.monotonic();fcntl.ioctl(slave,termios.TIOCSWINSZ,struct.pack('HHHH',h,w,w*8,h*16));os.kill(child.pid,signal.SIGWINCH)
                while (b'\x1b[2J' not in data[offset:] or b'\x1b[?25h' not in data[offset:].split(b'\x1b[2J',1)[-1]) and time.monotonic()-sent<1:drain(.005)
                assert b'\x1b[2J' in data[offset:] and b'\x1b[?25h' in data[offset:].split(b'\x1b[2J',1)[-1],'resize draw completion not observed'
                first=next(ts for ts,size in receipts if ts>=sent and size>offset and b'\x1b[2J' in data[offset:size] and b'\x1b[?25h' in data[offset:size].split(b'\x1b[2J',1)[-1])
                resize.append((first-sent)*1000);drain(.15)
            scroll=[]
            if long:
                for key in [b'\x1b[5~',b'\x1b[6~',b'\x1b[5~',b'\x1b[6~']:
                    offset=len(data);sent=time.monotonic();os.write(master,key)
                    while b'\x1b[?25h' not in data[offset:] and time.monotonic()-sent<1:drain(.005)
                    scroll.append((next(ts for ts,size in receipts if ts>=sent and size>offset and b'\x1b[?25h' in data[offset:size])-sent)*1000);drain(.1)
            memory=[]
            if cycles:
                for _ in range(8):
                    for pref in ['static','animated']:
                        os.write(master,b'\x1b[200~/settings mascot '+pref.encode()+b'\x1b[201~\r');drain(.35)
                    os.write(master,b'\x10');drain(.1);os.write(master,b'\x1b');drain(.3)
                    memory.append(usage(child.pid).rss)
            before_exit=usage(child.pid)
            os.write(master,b'\x04');drain(2)
            assert status is not None and child.returncode==0
            assert not errors,errors
            assert termios.tcgetattr(slave)==before
            row={'executable':str(exe),'sha256':hashlib.sha256(exe.read_bytes()).hexdigest(),'mode':mode,'long':long,'repeat':repeat,'diagnostics':diagnostics,'scope':'production TUI + simulated Kitty and typed IPC; not terminal-emulator CPU','timebase_numer':timebase.numer,'timebase_denom':timebase.denom,'lifetime_proc_cpu_seconds':(before_exit.user+before_exit.system)*seconds_per_tick,'lifetime_wait4_cpu_seconds':ru.ru_utime+ru.ru_stime,'steady_seconds':wall,'steady_cpu_pct':((b.user+b.system)-(a.user+a.system))*seconds_per_tick/wall*100,'steady_draws':steady.count(b'\x1b[?25h')+steady.count(b'\x1b[?25l'),'late_uploads':late.count(b'a=t,'),'steady_full_clears':steady.count(b'\x1b[2J'),'steady_uploads':steady.count(b'a=t,'),'steady_placements':steady.count(b'a=p,'),'steady_bytes':len(steady),'peak_sampled_rss_mib':peak/1048576,'peak_wait4_rss_mib':ru.ru_maxrss/1048576,'input_visible_ms':latency,'resize_draw_complete_ms':resize,'scroll_draw_complete_ms':scroll,'cycle_rss_mib':[v/1048576 for v in memory],'termios_restored':True,'alt_restored':b'\x1b[?1049l' in data,'no_inference':not any(n in requests for n in ['chat.send','agent.start'])}
            assert .75 < row['lifetime_proc_cpu_seconds']/row['lifetime_wait4_cpu_seconds'] < 1.1,'CPU unit cross-check failed'
            if mode in ['off','static']: assert row['steady_draws']==0 and row['steady_bytes']==0
            assert row['steady_full_clears']==0
            if diagnostics:
                target=HERE/('baseline' if 'baseline' in str(exe) else 'after')
                target.mkdir(exist_ok=True);(target/f'{mode}-{long}-diagnostics.jsonl').write_bytes((td/'diagnostic.jsonl').read_bytes())
            return row
        finally:
            stopping.set();listener.close();thread.join(timeout=1)
            if status is None:
                os.kill(child.pid,signal.SIGTERM);_,_,_=os.wait4(child.pid,0);child.returncode=-15
            os.close(master);os.close(slave)

if __name__=='__main__':
    exe=Path(sys.argv[1]).resolve();output=Path(sys.argv[2]);rows=[]
    cases=[(m,False,r) for r in range(2) for m in ['off','static','animated']]+[(m,True,0) for m in ['off','static','animated']]
    for mode,long,repeat in cases:
        row=run(exe,mode,long,cycles=(mode=='animated' and long),repeat=repeat);rows.append(row);print(json.dumps(row),flush=True);output.write_text(json.dumps(rows,indent=2)+'\n')
