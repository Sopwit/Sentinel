import os,json,time,urllib.request,re,ast,pathlib
out=pathlib.Path('docs/reviews/agent-v325-2026-10-10');src=pathlib.Path('core/src/agent/ObservationPolicy.cpp').read_text();part=src[src.index('"Classify the USER GOAL'):src.index('.arg(capabilities.join')];prompt=''.join(ast.literal_eval(x) for x in re.findall(r'"(?:[^"\\]|\\.)*"',part));prompt=prompt.replace('%1','filesystem,workspace,process,system,execution').replace('%2','\nACTIVE WORKSPACE ROOT: /tmp/sentinel-v325-fixture').replace('%3','Read calc.cpp in the active workspace and explain the actual behavior of add. Do not modify files or run commands.')
results=[]
for label,p,deadline in [('minimal','Return ONLY JSON: {"requirements":[]}',30),('revised-30',prompt,30)]:
 if os.environ.get('V325_MINIMAL_ONLY') and label != 'minimal':continue
 body={'model':os.environ.get('V325_MODEL','nvidia/nemotron-3-nano-4b'),'messages':[{'role':'user','content':p}],'stream':False,'max_tokens':512};start=time.monotonic();r={'label':label,'deadline_seconds':deadline,'bytes':len(json.dumps(body).encode())}
 try:
  with urllib.request.urlopen(urllib.request.Request('http://127.0.0.1:1234/v1/chat/completions',json.dumps(body).encode(),{'Content-Type':'application/json'}),timeout=deadline) as reply:
   d=json.load(reply);c=d.get('choices',[{}])[0];m=c.get('message',{});r.update(http=reply.status,finish=c.get('finish_reason'),usage=d.get('usage'),content_characters=len(m.get('content') or ''),reasoning_characters=len(m.get('reasoning_content','')))
   try:
    parsed=json.loads(m.get('content') or '');r['valid_requirements_json']=isinstance(parsed,dict) and isinstance(parsed.get('requirements'),list);r['requirement_count']=len(parsed.get('requirements',[])) if r['valid_requirements_json'] else None
   except (ValueError,TypeError):r['valid_requirements_json']=False
 except Exception as e:r['error']=type(e).__name__
 r['seconds']=time.monotonic()-start;results.append(r);(out/os.environ.get('V325_PROBE_OUTPUT','revised-probes.json')).write_text(json.dumps(results,indent=2));print(label,r['seconds'],r.get('finish',r.get('error')),flush=True)
