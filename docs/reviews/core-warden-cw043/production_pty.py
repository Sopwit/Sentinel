"""Real release TUI + deterministic IPC/terminal fixtures. NOT physical Ghostty acceptance."""
import os, pty, fcntl, termios, struct, select, time, json, socket, threading, tempfile, subprocess, signal
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
OUT=Path(__file__).resolve().parent
BINARY=ROOT/'cli/target/release/sentinel'

def run(name,mode='animated',respond=True,populated=False,extra=None,scenario=False,connected=True, comparison=None):
    requests=[]
    with tempfile.TemporaryDirectory(prefix='cw041-',dir='/private/tmp') as temp:
        endpoint=Path(temp)/'fixture.sock'
        listener=socket.socket(socket.AF_UNIX);listener.bind(str(endpoint));listener.listen();listener.settimeout(.1)
        stop=threading.Event()
        def serve():
            while not stop.is_set():
                try: conn,_=listener.accept()
                except socket.timeout: continue
                with conn:
                    reader=conn.makefile('rb')
                    while not stop.is_set():
                        line=reader.readline()
                        if not line: break
                        req=json.loads(line); command=req['name'];requests.append(command)
                        payload={
                            'hello':{'major':1,'minor':1,'daemon_version':'fixture','capabilities':[]},
                            'terminal.state':{'properties':{},'providers':[],'workspace':{'name':'fixture'},'workspaces':[]},
                            'model.list':{'models':[{'provider_id':'fixture','model_id':'fixture'}]},
                            'session.list':{'sessions':[{'session_id':'s','title':'Fixture'}]},
                            'terminal.attach':{'session_id':'s','state':'idle','run_id':'','output':'','title':'Fixture'},
                            'session.messages':{'session_id':'s','messages':[{'role':'assistant','content':'Existing fixture transcript'}] if populated else []},
                        }.get(command)
                        assert payload is not None, f'Unexpected inference or command: {command}'
                        conn.sendall((json.dumps({'version':{'major':1,'minor':1},'type':'response','id':req['id'],'name':command,'payload':payload})+'\n').encode())
        thread=threading.Thread(target=serve,daemon=True);thread.start()
        master,slave=pty.openpty();fcntl.ioctl(slave,termios.TIOCSWINSZ,struct.pack('HHHH',50,180,2880,1700));before=termios.tcgetattr(slave)
        diagnostic=Path(temp)/'diagnostic.jsonl'
        env=os.environ.copy()
        for key in ['TMUX','STY','SSH_CONNECTION','SSH_TTY','NO_COLOR','SENTINEL_TUI_ASCII','SENTINEL_TUI_REDUCED_MOTION','SENTINEL_CORE_WARDEN_ASSETS']: env.pop(key,None)
        env.update(TERM='xterm-256color',SENTINEL_TUI_MASCOT=mode,SENTINEL_TUI_MASCOT_DIAGNOSTICS=str(diagnostic));env.update(extra or {})
        proc=subprocess.Popen([str(BINARY),'--socket',str(endpoint if connected else Path(temp)/'absent.sock'),'tui'],stdin=slave,stdout=slave,stderr=slave,env=env,start_new_session=True)
        data=bytearray();answered=False;start=time.monotonic();sent=0;exit_at=5.5 if comparison else (3.3 if scenario else 1.5); first_image_ms=None; input_sent=False; transmissions_before_input=0
        actions=[(.55,b'\x1b[200~/settings mascot static\x1b[201~\r'),(.85,b'\x1b[200~/settings mascot animated\x1b[201~\r'),(1.15,b'\x1b[200~/settings mascot off\x1b[201~\r'),(1.4,b'\x1b[200~/settings mascot auto\x1b[201~\r'),(1.7,b'\x10'),(1.95,b'\x1b'),(2.2,'narrow'),(2.55,'restore')] if scenario else []
        exited=False; comparison_done=False; receipts=[]; last_graphics=None
        while time.monotonic()-start<7:
            elapsed=time.monotonic()-start
            if comparison and not comparison_done and elapsed>=2.0:
                comparison_done=True
                if comparison=='key': os.write(master,b'x');input_sent=True
                if comparison=='resize':
                    fcntl.ioctl(slave,termios.TIOCSWINSZ,struct.pack('HHHH',48,180,2880,1632));os.kill(proc.pid,signal.SIGWINCH)

            while sent<len(actions) and elapsed>=actions[sent][0]:
                action=actions[sent][1];sent+=1;input_sent=True
                if action in ['narrow','restore']:
                    w,h=(60,20) if action=='narrow' else (120,40)
                    fcntl.ioctl(slave,termios.TIOCSWINSZ,struct.pack('HHHH',h,w,w*8,h*16));os.kill(proc.pid,signal.SIGWINCH)
                elif action=='draft': os.write(master,'Türkçe draft'.encode())
                else: os.write(master,action)
            if elapsed>=exit_at and not exited:
                input_sent=True
                os.write(master,b'\x1b');time.sleep(.04);os.write(master,b'\x15\x04');exited=True
            if select.select([master],[],[],.01)[0]:
                try: chunk=os.read(master,65536)
                except OSError: break
                data.extend(chunk)
                graphics=(data.count(b'a=T,U=1'),data.count('\U0010eeee'.encode()))
                if graphics != last_graphics and len(receipts)<64:
                    receipts.append({'monotonic_ms':(time.monotonic()-start)*1000,'bytes':len(data),'uploads':graphics[0],'placeholders':graphics[1],'user_input_sent':input_sent})
                    last_graphics=graphics
                if b'a=T,U=1' in chunk and not input_sent:
                    transmissions_before_input += chunk.count(b'a=T,U=1')
                    if first_image_ms is None: first_image_ms=(time.monotonic()-start)*1000
                if respond and not answered and b'_Gi=31' in data:
                    os.write(master,b'\x1b_Gi=31;OK\x1b\\\x1b[?1;2c\x1b[6;34;16t\x1b[0n');answered=True
            if proc.poll() is not None: break
        if proc.poll() is None: proc.terminate();proc.wait();raise RuntimeError(f'{name}: exit timeout')
        for _ in range(5):
            if select.select([master],[],[],.02)[0]: data.extend(os.read(master,65536))
        records=[json.loads(line) for line in diagnostic.read_text().splitlines()]
        (OUT/f'{name}-diagnostics.jsonl').write_text('\n'.join(json.dumps(r) for r in records)+'\n')
        startup=records[0];presentations=[r for r in records if r.get('kind')=='presentation']
        (OUT/f'{name}-byte-receipts.json').write_text(json.dumps(receipts,indent=2)+'\n')
        result={'name':name,'scope':'production release TUI; simulated Kitty + IPC fixtures; NOT physical Ghostty','exit':proc.returncode,'capability':startup['protocol'],'capability_reason':startup['capability_reason'],'assets':startup['assets'],'first_image_before_input_ms':first_image_ms,'transmissions_before_input':transmissions_before_input,'placement_refresh_seen':any(r.get('redraw_reason')=='kitty_placement_refresh' for r in presentations),'max_frames':max((r.get('frames_loaded',0) for r in presentations),default=0),'kitty_transmits':data.count(b'a=T,U=1'),'owned_deletes':data.count(b'a=d,d=I'),'restored_alt':b'\x1b[?1049l' in data,'restored_termios':termios.tcgetattr(slave)==before,'requests':requests,'modes':sorted(set(r.get('mode','') for r in presentations)),'hidden_reasons':sorted(set(r.get('hidden_reason','') for r in presentations)),'draft_echo': 'Türkçe draft'.encode() in data,'populated_visible':b'Existing fixture transcript' in data,'welcome_visible':b'Start a new conversation' in data,'bytes':len(data)}
        os.close(master);os.close(slave);stop.set();listener.close();thread.join(timeout=.2)
        assert result['exit']==0 and result['restored_alt'] and result['restored_termios'],result
        assert 'chat.send' not in requests and 'agent.start' not in requests,result
        if name in ['animated','auto','populated','scenario']: assert result['max_frames']==16 and result['kitty_transmits']>0,result
        if name=='static' or not connected: assert result['max_frames']==1 and result['first_image_before_input_ms'] is not None and result['placement_refresh_seen'],result
        if name in ['off','timeout','missing','no-color','ssh','tmux']: assert result['max_frames']==0,result
        if populated: assert result['populated_visible'],result
        if scenario: assert result['modes']==['animated','auto','off','static'],result
        print(json.dumps(result),flush=True);return result

if __name__=='__main__':
    import sys
    if '--compare' in sys.argv:
        rows=[run('no-input',mode='static',connected=False,comparison='none'),run('one-key',mode='static',connected=False,comparison='key'),run('resize-only',mode='static',connected=False,comparison='resize')]
        (OUT/'comparison-results.json').write_text(json.dumps(rows,indent=2)+'\n');sys.exit(0)

    rows=[run('offline-cold',connected=False),run('offline-warm',connected=False),run('offline-static','static',connected=False),run('offline-auto','auto',connected=False),run('animated'),run('static','static'),run('off','off'),run('auto','auto'),run('populated',populated=True),run('timeout',respond=False),run('missing',extra={'SENTINEL_CORE_WARDEN_ASSETS':'/private/tmp/cw041-missing-assets'}),run('no-color',extra={'NO_COLOR':'1'}),run('ssh',extra={'SSH_TTY':'fixture'}),run('tmux',extra={'TMUX':'fixture'}),run('scenario',scenario=True)]
    (OUT/'production-pty-results.json').write_text(json.dumps(rows,indent=2)+'\n')
