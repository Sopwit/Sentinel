import sys,time,json,select
sys.path.insert(0,str(__import__('pathlib').Path(__file__).resolve().parent));import sentinel_v321_accept as a
out='docs/reviews/agent-v321-2026-10-10/recovery-live.json';cases=[]
def observe(label,run,bound=90,cancel_delta=False):
 start=time.monotonic();sent=False;terminal=None
 while time.monotonic()-start<bound:
  for e in list(a.events):
   p=e.get('payload',{})
   if p.get('run_id')!=run:continue
   if cancel_delta and e.get('name')=='output.delta' and not sent:
    sent=True;a.record(label+'-cancel','run.cancel',{'run_id':run})
   if e.get('name') in ('run.completed','run.failed','run.cancelled'):terminal=e
  if terminal:break
  if select.select([a.sock],[],[],.1)[0]:a.read()
 if not terminal:a.record(label+'-timeout','run.cancel',{'run_id':run});terminal={'name':'observer-timeout'}
 cases.append({'label':label,'run_id':run,'terminal':terminal,'cancel_after_output_delta':sent,'seconds':time.monotonic()-start})
try:
 a.start();a.results.append(['hello',a.connect()]);a.configure('lm-studio');time.sleep(3);a.record('binding','model.select',{'provider_id':'lm-studio','model_id':'nvidia/nemotron-3-nano-4b'});a.f.close();a.f=a.sock.makefile('rb',buffering=0)
 sid=a.record('stream-session','session.create',{'title':'V321 synthetic streaming'})['payload']['session_id'];r=a.record('stream-send','chat.send',{'session_id':sid,'text':'Write a numbered list of 100 short imaginary planet names. This is synthetic streaming cancellation testing.'});observe('chat-stream',r['payload']['run_id'],cancel_delta=True)
 time.sleep(1)
 a.record('offline-endpoint','desktop.setting',{'key':'lmStudioEndpoint','value':'http://127.0.0.1:1'})
 sid=a.record('offline-session','session.create',{'title':'V321 synthetic unavailable'})['payload']['session_id'];r=a.record('offline-start','agent.start',{'session_id':sid,'text':'Read README.md in the active workspace without modifications.'})
 if r.get('type')=='response':
  a.record('offline-cancel','run.cancel',{'run_id':r['payload']['run_id']});observe('provider-unavailable-cancel',r['payload']['run_id'],bound=15)
 else:cases.append({'label':'provider-unavailable-start','response':r})
 a.record('restore-endpoint','desktop.setting',{'key':'lmStudioEndpoint','value':'http://127.0.0.1:1234'});a.record('restored-state','terminal.state',{})
except Exception as e:a.results.append(['harness-error',str(e)])
finally:
 open(out,'w').write(json.dumps({'root':a.root,'cases':cases,'results':a.results,'events':a.events},indent=2))
 if a.proc and a.proc.poll() is None:a.proc.terminate();a.proc.wait(10)
 a.log.close();print(a.root,flush=True)
