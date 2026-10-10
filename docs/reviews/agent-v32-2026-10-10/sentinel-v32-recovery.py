import sys,os,json,time,shutil,select
sys.path.insert(0,'/tmp');import sentinel_v32_accept as a
x=json.load(open('docs/reviews/agent-v32-2026-10-10/context-ui.json'));a.root=x['root'];a.path=a.root+'/run/daemon.sock';sid=next(r['payload']['session_id'] for n,r in x['results'] if n=='observer-attach')
try:
 shutil.copy2('build/tests/apps/sentinel-daemon/sentinel-daemon',a.root+'/bin/sentinel-daemon');a.start();a.results.append(['hello',a.connect()]);a.record('resume','terminal.attach',{'session_id':sid});a.record('persisted-grounded-history','session.messages',{'session_id':sid});a.record('binding','model.current',{});a.record('readiness','terminal.state',{})
 a.record('closed-provider','desktop.setting',{'key':'lmStudioEndpoint','value':'http://127.0.0.1:1'});a.record('closed-provider-chat','chat.send',{'session_id':sid,'text':'Synthetic provider failure probe'});a.waitrun('closed-provider',seconds=20)
 a.record('restore-provider','desktop.setting',{'key':'lmStudioEndpoint','value':'http://127.0.0.1:1234'});a.configure('lm-studio');time.sleep(3);a.record('restored-binding','model.select',{'provider_id':'lm-studio','model_id':'nvidia/nemotron-3-nano-4b'});a.record('restored-readiness','terminal.state',{})
except Exception as e:a.results.append(['error',str(e)]);print(type(e).__name__,str(e),flush=True)
finally:
 if a.proc and a.proc.poll() is None:a.proc.terminate();a.proc.wait(10)
 a.log.close();open('docs/reviews/agent-v32-2026-10-10/recovery.json','w').write(json.dumps({'root':a.root,'results':a.results,'events':a.events},indent=2))
