# Transport-only diagnostic. This does not exercise AgentRuntime or tool authority.
import json,sys,time,urllib.request,urllib.error
from pathlib import Path
model,out=sys.argv[1:]
body=json.dumps({'model':model,'messages':[{'role':'user','content':'Reply with OK.'}],'max_tokens':128,'stream':False}).encode()
r={'model_requested':model,'output_budget':128,'request_bytes':len(body),'transport':'direct local HTTP; no relay','agent_acceptance':False}
start=time.monotonic()
try:
 request=urllib.request.Request('http://127.0.0.1:1234/v1/chat/completions',data=body,headers={'Content-Type':'application/json'})
 with urllib.request.urlopen(request,timeout=60) as response:
  r['http_status']=response.status;j=json.loads(response.read())
 choice=(j.get('choices') or [{}])[0];m=choice.get('message',{});usage=j.get('usage',{});finish=choice.get('finish_reason')
 r.update(model_reported=j.get('model'),finish_reason=finish if finish in ['stop','length','tool_calls','content_filter',None] else 'other',content_bytes=len(str(m.get('content') or '').encode()),reasoning_bytes=len(str(m.get('reasoning_content') or '').encode()),tool_calls=len(m.get('tool_calls') or []),usage={k:usage[k] for k in ['prompt_tokens','completion_tokens','total_tokens'] if isinstance(usage.get(k),(int,float))})
except urllib.error.HTTPError as e:
 r.update(http_status=e.code,error_kind='HTTPError')
except Exception as e:
 r.update(error_kind=type(e).__name__)
r['seconds']=round(time.monotonic()-start,3)
Path(out).write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r))
