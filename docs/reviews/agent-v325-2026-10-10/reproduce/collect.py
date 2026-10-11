import json,pathlib,shutil,sys
src=pathlib.Path(sys.argv[1]);dest=pathlib.Path(sys.argv[2]);dest.mkdir(parents=True,exist_ok=True)
categories=('sentinel.provider.diagnostics:','sentinel.agent.diagnostics:','sentinel.classification.diagnostics:','sentinel.agent.loop.diagnostics:','sentinel.agent.ipc.diagnostics:')
audit={}
for batch in ('nemotron','nemotron-final','qwen'):
 p=src/batch/'live.json'
 if not p.exists():continue
 x=json.loads(p.read_text());out=dest/batch;out.mkdir(exist_ok=True);shutil.copy2(p,out/'live.json')
 for f in (src/batch).glob('*-final.ansi*'):shutil.copy2(f,out/f.name)
 records=[]
 for line in pathlib.Path(x['root'],'daemon.log').read_text().splitlines():
  if any(c in line for c in categories) and '{' in line:
   records.append({'category':line.split(':',1)[0],'data':json.loads(line[line.index('{'):])})
 (out/'trace.json').write_text(json.dumps(records,indent=2))
 terminal={};late=[];seen=set();replays=0;tool_owners={};cross=[]
 for e in x['events']:
  p=e.get('payload',{});key=(p.get('server_generation'),p.get('event_sequence'))
  if key in seen:replays+=1;continue
  seen.add(key);rid=p.get('run_id');n=e.get('name');call=p.get('tool_call_id')
  if call and rid:
   if call in tool_owners and tool_owners[call]!=rid:cross.append(call)
   tool_owners[call]=rid
  if rid in terminal and n in ('tool.result','tool.running','tool.requested','run.completed','run.failed','run.cancelled'):late.append([rid,n])
  if n in ('run.completed','run.failed','run.cancelled'):terminal[rid]=n
 audit[batch]={'terminal_runs':len(terminal),'late_results_or_duplicate_terminals':late,'cross_run_tool_ids':cross,'replayed_events':replays,'all_fixture_hashes_unchanged':all(c['before']==c['after'] for c in x['cases']),'allow_decisions':[d for c in x['cases'] for d in c['decisions'] if d['allow']]}
(dest/'lifecycle-audit.json').write_text(json.dumps(audit,indent=2))
shutil.copy2(src/'attempts.json',dest/'attempts.json')
print(json.dumps(audit,indent=2))
