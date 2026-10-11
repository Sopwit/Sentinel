"""Production release interaction journeys with disposable IPC fixtures, not physical acceptance."""
import os, re, json, time, socket, threading, tempfile, subprocess, pty, fcntl, termios, struct, select, hashlib, unicodedata
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
OUT=Path(__file__).resolve().parent
EXE=ROOT/'cli/target/release/sentinel'

def screen(data,w,h):
    # Bounded text-only VT replay, ignoring graphics payloads and style.
    text=data.decode('utf8','replace')
    text=re.sub(r'\x1b_[\s\S]*?\x1b\\','',text)
    text=re.sub(r'\x1b\][\s\S]*?(?:\x07|\x1b\\)','',text)
    grid=[[' ']*w for _ in range(h)];x=y=0
    for token in re.findall(r'\x1b\[[0-?]*[ -/]*[@-~]|\x1b.|[^\x1b]',text):
        if token.startswith('\x1b['):
            raw=token[2:-1];kind=token[-1];p=[int(v) if v.isdigit() else 0 for v in raw.lstrip('?').split(';')];n=p[0] or 1
            if kind in 'Hf':y=max(0,min(h-1,n-1));x=max(0,min(w-1,(p[1] if len(p)>1 and p[1] else 1)-1))
            elif kind=='G':x=max(0,min(w-1,n-1))
            elif kind=='d':y=max(0,min(h-1,n-1))
            elif kind=='A':y=max(0,y-n)
            elif kind=='B':y=min(h-1,y+n)
            elif kind=='C':x=min(w-1,x+n)
            elif kind=='D':x=max(0,x-n)
            elif kind=='J' and p[0] in [2,3]:grid=[[' ']*w for _ in range(h)]
            elif kind=='K':
                for c in range(0 if p[0] in [1,2] else x,w if p[0] in [0,2] else x+1):grid[y][c]=' '
        elif token.startswith('\x1b'):pass
        elif token=='\r':x=0
        elif token=='\n':y=min(h-1,y+1)
        elif token=='\b':x=max(0,x-1)
        elif token>=' ':
            if x>=w:x=0;y=min(h-1,y+1)
            grid[y][x]=token;x+=2 if unicodedata.east_asian_width(token) in ['W','F'] else 1
    return '\n'.join(''.join(row) for row in grid)

def run(w,h,fallback=False,offline=False):
    name=f'{w}x{h}'+('-fallback' if fallback else '')+('-offline' if offline else '')
    requests=[];errors=[];checks=[];stop=threading.Event();selected='model-alpha';fail_next=True;created=0
    with tempfile.TemporaryDirectory(prefix='v33b-',dir='/private/tmp') as temp:
        endpoint=Path(temp)/'fixture.sock'
        listener=socket.socket(socket.AF_UNIX);listener.bind(str(endpoint));listener.listen();listener.settimeout(.1)
        sessions=[{'session_id':'session-a','title':'Alpha conversation'},{'session_id':'session-b','title':'Beta conversation'}]
        def serve():
            nonlocal selected,fail_next,created
            try:
                while not stop.is_set():
                    try:conn,_=listener.accept()
                    except socket.timeout:continue
                    with conn:
                        conn.settimeout(.2);buffer=b''
                        while not stop.is_set():
                            try:chunk=conn.recv(65536)
                            except socket.timeout:continue
                            if not chunk:break
                            buffer+=chunk
                            while b'\n' in buffer:
                                line,buffer=buffer.split(b'\n',1);req=json.loads(line);cmd=req['name'];args=req['payload'];requests.append({'name':cmd,'payload':args})
                                if cmd=='hello':payload={'major':1,'minor':1,'daemon_version':'fixture','capabilities':[]}
                                elif cmd=='terminal.state':payload={'properties':{'effective_selection':{'provider_id':'fixture','model_id':selected},'activeRuntimeProviderLabel':'fixture','activeRuntimeModelLabel':selected,'activeRuntimeReadinessState':'Available','activeRuntimeReadinessSummary':'Catalog only; inference unverified'},'providers':[{'provider_id':'fixture','catalog':'Available','reason':'Fixture catalog'}],'workspace':{'name':'Fixture'},'workspaces':[]}
                                elif cmd=='model.list':payload={'models':[{'provider_id':'fixture','model_id':m,'health':'Available','reason':'Discovered; inference not attested','capabilities':'chat'} for m in ['model-alpha','model-beta']]}
                                elif cmd=='session.list':payload={'sessions':sessions}
                                elif cmd=='session.create':
                                    created+=1;sid=f'created-{created}';sessions.append({'session_id':sid,'title':args['title']});payload={'session_id':sid,'type':'conversation','created_at':'fixture','title':args['title'],'state':'idle','run_id':'','output':''}
                                elif cmd=='terminal.attach':payload={'session_id':args['session_id'],'title':next(s['title'] for s in sessions if s['session_id']==args['session_id']),'state':'idle','run_id':'','output':''}
                                elif cmd=='session.messages':payload={'session_id':args['session_id'],'messages':[{'role':'assistant','content':'History '+args['session_id']}]}
                                elif cmd=='model.select':
                                    if fail_next:
                                        fail_next=False
                                        conn.sendall((json.dumps({'version':{'major':1,'minor':1},'type':'error','id':req['id'],'name':cmd,'payload':{'code':'model-unavailable','message':'Fixture selection refused'}})+'\n').encode());continue
                                    selected=args['model_id'];payload={'provider_id':'fixture','model_id':selected}
                                elif cmd=='desktop.action':payload={'accepted':True,'result':{'value':True}}
                                else:raise AssertionError('Unexpected inference or unsupported command: '+cmd)
                                conn.sendall((json.dumps({'version':{'major':1,'minor':1},'type':'response','id':req['id'],'name':cmd,'payload':payload})+'\n').encode())
            except Exception as e:errors.append(repr(e))
        thread=threading.Thread(target=serve,daemon=True);thread.start()
        master,slave=pty.openpty();fcntl.ioctl(slave,termios.TIOCSWINSZ,struct.pack('HHHH',h,w,w*8,h*16));before=termios.tcgetattr(slave)
        env=os.environ.copy()
        for key in ['TMUX','STY','SSH_CONNECTION','SSH_TTY','NO_COLOR','SENTINEL_TUI_ASCII','SENTINEL_TUI_REDUCED_MOTION']:env.pop(key,None)
        env.update(TERM='xterm-256color',SENTINEL_TUI_MASCOT='animated',SENTINEL_TUI_MASCOT_DIAGNOSTICS=str(Path(temp)/'mascot.jsonl'))
        if fallback:env.update(NO_COLOR='1',SENTINEL_TUI_ASCII='1')
        process=subprocess.Popen([str(EXE),'--socket',str(endpoint if not offline else Path(temp)/'absent.sock'),'tui'],stdin=slave,stdout=slave,stderr=slave,env=env,start_new_session=True)
        data=bytearray();replied=False;pending=[]
        def drain(seconds=.18):
            nonlocal replied
            end=time.monotonic()+seconds
            while time.monotonic()<end:
                while pending and time.monotonic()>=pending[0][0]:_,reply=pending.pop(0);os.write(master,reply)
                if select.select([master],[],[],.01)[0]:
                    try:data.extend(os.read(master,65536))
                    except OSError:break
                    if not replied and b'_Gi=31' in data:
                        assert b'\x1b[5n' not in data,'CW-04.5 regression'
                        now=time.monotonic();pending.extend([(now+.01,b'\x1b_Gi=31;'),(now+.02,b'OK\x1b\\'),(now+.03,b'\x1b[6;16;8t'),(now+.08,b'\x1b[?1;2c')]);replied=True
        def send(value,wait=.18):os.write(master,value);drain(wait)
        def paste(text):send(b'\x1b[200~'+text.encode()+b'\x1b[201~')
        def command(text):paste(text);send(b'\r',.25)
        def expect(text,label):
            until=time.monotonic()+2
            view=screen(data,w,h)
            while text not in view and time.monotonic()<until:
                drain(.05);view=screen(data,w,h)
            (OUT/f'{name}-{label}.txt').write_text(view)
            assert text in view,(name,label,text,view)
            checks.append(label)
        def palette(text):
            send(b'\x10');expect('command · Enter','palette-open');send(text.encode());expect('Filter: '+text,'palette-query');send(b'\r',.25);expect(('session' if text=='sessions' else 'help')+' · Enter','palette-selected')
        try:
            drain(.65 if not offline else .25)
            if not offline:
                expect('History session-a','initial-history')
                # Slash discovery, arrows and Esc retain original draft.
                send(b'/');expect('Commands','slash-discovery');send(b'\x1b[B');send(b'\x1b');expect('/','slash-dismiss');send(b'\x15')
                paste('first line\nsecond /path');send(b'\x0f');send(b'third')
                palette('sessions');send(b'session-b');expect('Filter: session-b','session-identity');send(b'\r',.35)
                expect('History session-b','session-switched');assert 'History session-a' not in screen(data,w,h)
                palette('sessions');send(b'session-a');expect('Filter: session-a','return-query');send(b'\r',.35);expect('second /path','draft-restored');expect('third','multiline-retained')
                send(b'\x15\x7f\x15\x7f\x15')
                command('/settings mascot static');command('/settings theme glacier');command('/settings');expect('static','settings-mascot');expect('temporary override','settings-scope');send(b'\x1b')
                command('/model');send(b'model-beta');expect('Filter: model-beta','model-query-fail');send(b'\r',.3);expect('Model selection failed','model-error')
                command('/model');send(b'model-beta');expect('Filter: model-beta','model-query-success');send(b'\r',.35)
                command('/model');send(b'model-beta');expect('Selected · fixture / model-beta','model-authority');send(b'\x1b')
                command('/archive');expect('Confirm archive','archive-confirm');send(b'\x1b');assert not any(r['name']=='desktop.action' for r in requests)
                command('/new Created via TUI');expect('History created-1','new-session')
                command('/settings mascot auto');command('/settings');expect('auto','settings-auto');send(b'\x1b')
                send(b'/not-a-command');send(b'\r');expect('Unknown command','invalid-local');send(b'\x15')
                command('/not-a-command');expect('Unknown command','invalid-pasted-local');send(b'\x15')
            else:
                paste('offline\nsecond /path');palette('help');send(b'\x1b');expect('second /path','offline-draft');send(b'\x15\x7f\x15')
                command('/settings mascot off');command('/settings');expect('off','offline-settings');send(b'\x1b')
            send(b'\x04',.25)
            process.wait(timeout=3)
            assert process.returncode==0
            assert termios.tcgetattr(slave)==before
            assert b'\x1b[?1049l' in data
            assert not errors,errors
            assert not any(r['name'] in ['chat.send','agent.start'] for r in requests)
            diagnostic=Path(temp)/'mascot.jsonl'
            if diagnostic.exists():(OUT/f'{name}-mascot.jsonl').write_bytes(diagnostic.read_bytes())
            (OUT/f'{name}.ansi').write_bytes(data)
            row={'size':[w,h],'fallback':fallback,'offline':offline,'checks':checks,'clean_exit':True,'termios_restored':True,'alt_restored':True,'no_inference':True,'requests':requests,'graphics_uploads':data.count(b'a=t,'),'owned_deletes':data.count(b'a=d,d=I')}
            print(json.dumps(row),flush=True);return row
        finally:
            (OUT/f'{name}-requests.json').write_text(json.dumps(requests,indent=2))
            (OUT/f'{name}.ansi').write_bytes(data)
            if process.poll() is None:process.terminate();process.wait(timeout=2)
            stop.set();listener.close();thread.join(timeout=1);os.close(master);os.close(slave)

if __name__=='__main__':
    rows=[run(w,h,fallback) for fallback in [False,True] for w,h in [(80,24),(100,30),(120,40),(160,48)]]
    rows+=[run(80,24,True,True)]
    (OUT/'pty-results.json').write_text(json.dumps({'executable':str(EXE),'sha256':hashlib.sha256(EXE.read_bytes()).hexdigest(),'scope':'Simulated IPC and terminal PTY, NOT physical acceptance','cases':rows},indent=2)+'\n')
