import json,collections,datetime,pathlib,sys
root=pathlib.Path(sys.argv[1]);rows=[]
for batch,model in (('nemotron','nemotron'),('nemotron-final','nemotron'),('qwen','qwen')):
 p=root/batch/'live.json'
 if not p.exists():continue
 x=json.loads(p.read_text())
 for c in x['cases']:
  events=[e for e in c['events'] if e.get('payload',{}).get('run_id')==c['run_id']]
  reads=[e for e in events if e['name']=='tool.result' and e['payload'].get('tool')=='read-file']
  requested=[e for e in events if e['name']=='tool.requested']
  classifier=[d for d in c['native_diagnostics'] if d.get('classification_result')]
  hist=next((r['payload'].get('messages',[]) for label,r in x['results'] if label==c['label']+'-history'),[])
  answer=next((m['content'] for m in reversed(hist) if m.get('role')=='assistant'),'')
  user=next((m['timestamp'] for m in hist if m.get('role')=='user'),None)
  first=requested[0]['payload'].get('timestamp') if requested else None
  seconds=(datetime.datetime.fromisoformat(first)-datetime.datetime.fromisoformat(user)).total_seconds() if user and first else None
  row={'batch':batch,'model':model,'label':c['label'],'outcome':c['outcome'],'terminal':c.get('terminal',{}).get('name'), 'seconds':round(c['seconds'],3),'classification':classifier[-1] if classifier else None,'time_to_first_tool_seconds':seconds,'tools':[e['payload'].get('tool') for e in requested],'read_results':len(reads),'accepted_final':bool(c.get('terminal') and c['terminal']['name']=='run.completed'),'answer':answer,'unchanged':c['before']==c['after'],'decisions':c['decisions'],'terminal_events':sum(e['name'] in ('run.completed','run.failed','run.cancelled') for e in events),'quality':'requires_manual_review' if 'Interpretation:' in answer else ('receipt_only' if c.get('terminal') and c['terminal']['name']=='run.completed' else 'no_accepted_answer')}
  rows.append(row)
(root/'attempts.json').write_text(json.dumps(rows,indent=2))
print(json.dumps({'attempts':len(rows),'per_model':{m:{'runs':len(v),'classified':sum(bool(r['classification'] and r['classification']['classification_result']=='classified') for r in v),'with_reads':sum(r['read_results']>0 for r in v),'accepted_final':sum(r['accepted_final'] for r in v),'unchanged':all(r['unchanged'] for r in v)} for m in ('nemotron','qwen') if (v:=[r for r in rows if r['model']==m])}},indent=2))
