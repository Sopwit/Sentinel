import json,pathlib,collections
root=pathlib.Path('docs/reviews/agent-v325-2026-10-10');rows=json.loads((root/'attempts.json').read_text());major=[r for r in rows if r['label'][0] in 'ABCDE'];controls=[r for r in rows if r not in major]
assert not any(r['quality']=='requires_manual_review' for r in rows), 'Review actual explanations before assigning task success'
assert len(major)==31 and len(controls)==4, 'All planned attempts must be preserved'
lines=['\n## Complete measured results\n','| Model / batch | Task attempts | Classified | Any tools | File-read results | Runtime completed | Correct requested task |','| --- | ---: | ---: | ---: | ---: | ---: | ---: |']
for label,rs in [('Nemotron initial', [r for r in major if r['batch']=='nemotron']),('Nemotron final-source extra A',[r for r in major if r['batch']=='nemotron-final']),('Qwen final source',[r for r in major if r['batch']=='qwen']),('Total',major)]:
 lines.append(f"| {label} | {len(rs)} | {sum(bool(r['classification'] and r['classification']['classification_result']=='classified') for r in rs)} | {sum(bool(r['tools']) for r in rs)} | {sum(r['read_results'] for r in rs)} | {sum(r['accepted_final'] for r in rs)} | 0 |")
lines += ['','All 30 planned A–E repeats (three per journey per model), one extra final-source A, and four cancellation/reconnection controls are recorded. Correct user-task success is **0/31 (0%)**; the two receipt-only runtime completions must not be promoted to task successes. Independent production-classifier probes are additional failures, outside this actual-TUI denominator.','', '| Model | Journey | Attempts | Runtime completions | Task successes |','| --- | --- | ---: | ---: | ---: |']
for m in ('nemotron','qwen'):
 for j in 'ABCDE':
  rs=[r for r in major if r['model']==m and r['label'].startswith(j+'-')];lines.append(f"| {m} | {j} | {len(rs)} | {sum(r['accepted_final'] for r in rs)} | 0 |")
lines += ['','Every attempt, its measured duration, classifier metadata/request ID, first-tool timing, tool names, accepted state, actual answer, unchanged-hash result and terminal-event count is in [attempts.json](../reviews/agent-v325-2026-10-10/attempts.json). The lifecycle audit records no late tool results, duplicate terminal states, cross-run tool IDs or granted write/process approvals. All fixture hashes remain unchanged. Concrete read-path/repeated-read counts cannot be reconstructed from empty IPC resource arrays on failed runs; final-source diagnostics expose verified/repeated read counts when planning is reached. Missing measurements are not zero-valued successes.']
p=pathlib.Path('docs/development/TUI_V3_2_5_LIVE_ACCEPTANCE.md');p.write_text(p.read_text()+'\n'.join(lines)+'\n')
print('Report metrics written')
