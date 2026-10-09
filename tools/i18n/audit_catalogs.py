#!/usr/bin/env python3
"""Read-only TS coverage and structural audit; semantic accuracy requires review."""
import argparse, json, re
from pathlib import Path
import xml.etree.ElementTree as ET
parser=argparse.ArgumentParser(); parser.add_argument('--output',type=Path,required=True); args=parser.parse_args()
root=Path(__file__).resolve().parents[2]
base=ET.parse(root/'translations/sentinel_en.ts').getroot()
def keys(tree):
    return {(c.findtext('name',''),m.findtext('source',''),m.findtext('comment','')) for c in tree.findall('context') for m in c.findall('message') if m.find('translation') is None or m.find('translation').get('type') not in ('obsolete','vanished')}
expected=keys(base); result={}
for path in sorted((root/'translations').glob('sentinel_*.ts')):
    tree=ET.parse(path).getroot(); rows=[]; complete=0; identical=0
    for c in tree.findall('context'):
        for m in c.findall('message'):
            t=m.find('translation'); src=m.findtext('source','')
            if t is not None and t.get('type') in ('obsolete','vanished'):continue
            parts=[''.join(x.itertext()) for x in t.findall('numerusform')] if t is not None and t.findall('numerusform') else [''.join(t.itertext()) if t is not None else '']
            unfinished=t is None or t.get('type')=='unfinished' or any(not x.strip() for x in parts)
            if not unfinished:complete+=1
            if not unfinished and all(x==src for x in parts):identical+=1
            problems=[]
            placeholders=sorted(re.findall(r'%(?:L?\d+|n)',src))
            if any(sorted(re.findall(r'%(?:L?\d+|n)',x))!=placeholders for x in parts if x.strip()):problems.append('placeholder-mismatch')
            if any(len(x)>max(40,len(src)*1.8) for x in parts):problems.append('expansion-review')
            if problems: rows.append({'context':c.findtext('name'),'source':src,'translation':parts,'issues':problems})
    total=sum(1 for c in tree.findall('context') for m in c.findall('message') if m.find('translation') is None or m.find('translation').get('type') not in ('obsolete','vanished'))
    missing=sorted(expected-keys(tree))
    result[path.stem]={'language':tree.get('language'),'active_messages':total,'finished':complete,'finished_percent':round(100*complete/max(total,1),2),'english_source_messages':len(expected),'missing_source_keys':len(missing),'source_coverage_percent':round(100*(len(expected)-len(missing))/max(len(expected),1),2),'identical_to_source':identical,'missing':[{'context':x[0],'source':x[1]} for x in missing],'review':rows}
args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n')
for key,row in result.items():print(f"{key}: {row['finished']}/{row['active_messages']} finished ({row['finished_percent']}%), {row['missing_source_keys']} missing source keys, {len(row['review'])} structural/expansion reviews")
