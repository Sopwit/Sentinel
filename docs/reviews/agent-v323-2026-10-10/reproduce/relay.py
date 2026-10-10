import http.server,http.client,json,sys,time,threading,re
from pathlib import Path
out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True);lock=threading.Lock()
class Handler(http.server.BaseHTTPRequestHandler):
 def log_message(self,*a):pass
 def do_GET(self):self.forward()
 def do_POST(self):self.forward()
 def forward(self):
  raw=self.rfile.read(int(self.headers.get('Content-Length',0)));request={}
  try:request=json.loads(raw) if raw else {}
  except Exception:pass
  t=time.monotonic();conn=http.client.HTTPConnection('127.0.0.1',1234,timeout=180)
  try:
   conn.request(self.command,self.path,raw,{'Content-Type':'application/json'});r=conn.getresponse();data=r.read();status=r.status
   try:body=json.loads(data)
   except Exception:body={}
   if self.command=='POST' and 'chat/completions' in self.path:
    choice=(body.get('choices') or [{}])[0];msg=choice.get('message',{});finish=choice.get('finish_reason');usage=body.get('usage',{})
    safe={'model_requested':request.get('model'),'model_reported':body.get('model'),'request_bytes':len(raw),'output_budget':request.get('max_tokens'),'http_status':status,'seconds':round(time.monotonic()-t,3),'finish_reason':finish if finish in ['stop','length','tool_calls','content_filter',None] else 'other','content_bytes':len(str(msg.get('content') or '').encode()),'reasoning_bytes':len(str(msg.get('reasoning_content') or '').encode()),'tool_calls':len(msg.get('tool_calls') or []),'tool_observation_bytes':[len(str(m.get('content','')).encode()) for m in request.get('messages',[]) if m.get('role')=='tool'],'usage':{k:usage[k] for k in ['prompt_tokens','completion_tokens','total_tokens'] if isinstance(usage.get(k),(int,float))},'reasoning_tokens':usage.get('completion_tokens_details',{}).get('reasoning_tokens'),'context_error':any(v in str(body.get('error','')).lower() for v in ['context','tokens to keep'])}
    catalog={d.get('function',{}).get('name'):d.get('function',{}).get('parameters',{}).get('properties',{}) for d in request.get('tools',[])}
    safe['calls']=[]
    for c in msg.get('tool_calls') or []:
     f=c.get('function',{});name=f.get('name');args=f.get('arguments','');entry={'tool':name if name in catalog else 'unregistered','argument_bytes':len(str(args).encode())}
     try:
      a=json.loads(args);entry['argument_keys']=sorted(a) if isinstance(a,dict) else [];entry['unknown_keys']=[k for k in entry['argument_keys'] if k not in catalog.get(name,{})]
      if isinstance(a,dict):
       path=a.get('path');entry['path_kind']='missing' if path is None else ('invented-workspace-alias' if str(path).startswith('/workspace') else ('absolute' if str(path).startswith('/') else 'relative'))
       if 'oldString' in a:entry['old_string_has_line_numbers']=bool(re.search(r'(?m)^\s*\d+[|:]\s',str(a['oldString'])))
     except Exception:entry['invalid_json']=True
     safe['calls'].append(entry)
    with lock:
     with (out/'provider-proxy.jsonl').open('a') as log:log.write(json.dumps(safe)+'\n')
   self.send_response(status);self.send_header('Content-Type','application/json');self.send_header('Content-Length',str(len(data)));self.end_headers();self.wfile.write(data)
  except Exception as e:
   self.send_error(502,'Local diagnostic relay failed')
  finally:conn.close()
server=http.server.ThreadingHTTPServer(('127.0.0.1',0),Handler);(out/'proxy-port.txt').write_text(str(server.server_port));server.serve_forever()
