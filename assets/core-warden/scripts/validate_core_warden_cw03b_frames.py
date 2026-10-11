"""Read-only PNG/preview audit with bounded eye-island readability measurements."""
from PIL import Image
from pathlib import Path
import json,hashlib
ROOT=Path(__file__).resolve().parents[1];m=json.loads((ROOT/'animations/manifest.json').read_text());report={'states':{},'frame_files':0}
for state,info in m['states'].items():
 areas=[]
 for size,paths in info['frames'].items():
  z=int(size);assert len(paths)==info['frame_count']
  assert sorted((ROOT/'animations'/state/size).glob('frame_*.png'))==[ROOT/p for p in paths]
  for p in paths:
   im=Image.open(ROOT/p);im.verify();im=Image.open(ROOT/p);assert im.mode=='RGBA' and im.size==(z,z)
   a=im.getchannel('A');assert a.getextrema()==(0,255)
   assert all(a.getpixel((x,y))==0 for x in range(z) for y in [0,z-1])
   assert all(a.getpixel((x,y))==0 for y in range(z) for x in [0,z-1]);report['frame_files']+=1
   if z==96:
    points=set()
    for y in range(40,55):
     for x in range(25,56):
      r,g,b,a=im.getpixel((x,y))
      if a>240 and min(r,g,b)>170 and b-r>3:points.add((x,y))
    islands=[]
    while points:
     start=points.pop();stack=[start];island={start}
     while stack:
      x,y=stack.pop()
      for dx in [-1,0,1]:
       for dy in [-1,0,1]:
        q=(x+dx,y+dy)
        if q in points:points.remove(q);stack.append(q);island.add(q)
     if len(island)>=4:islands.append(island)
    assert len(islands)==2,(p,[len(a) for a in islands])
    areas.append(sorted(len(a) for a in islands))
 for suffix,size in [('',192),('_128',128),('_96',96)]:
  webp=Image.open(ROOT/'previews'/('cw03a' if state in ['idle','thinking','tool_running'] else 'cw03b')/f'{state}{suffix}.webp');assert webp.n_frames==info['frame_count']
  duration=0
  for index in range(webp.n_frames):
   webp.seek(index);webp.load();assert webp.size==(size,size);duration+=webp.info['duration']
   source=Image.open(ROOT/info['frames'][str(size)][index]).convert('RGBA')
   decoded=webp.convert('RGBA');assert source.getchannel('A').tobytes()==decoded.getchannel('A').tobytes()
   bg=Image.new('RGBA',source.size,(36,41,48,255))
   assert Image.alpha_composite(bg,source).tobytes()==Image.alpha_composite(bg,decoded).tobytes()
  assert duration==info['frame_count']*125
  assert webp.info.get('loop',1)==(0 if info['loop'] else 1)
 report['states'][state]={'frames_per_resolution':info['frame_count'],'96px_two_eye_islands_every_frame':True,'96px_eye_pixel_area_min':min(min(a) for a in areas),'webp_duration_ms':duration}
report['RGBA_dimensions_alpha_frame_counts_paths_pass']=True
(ROOT/'blender/cw03b_frame_validation.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))

protected=json.loads((ROOT.parents[1]/'docs/reviews/core-warden-cw03b/protected-before.json').read_text())
for path,expected in protected.items():assert hashlib.sha256((ROOT.parents[1]/path).read_bytes()).hexdigest()==expected,path
before=json.loads((ROOT.parents[1]/'docs/reviews/core-warden-cw03b/manifest-before.json').read_text())
for state in ['idle','thinking','tool_running']:assert m['states'][state]==before['states'][state],state
contract=json.loads((ROOT/'state-contract.json').read_text());old=json.loads((ROOT.parents[1]/'docs/reviews/core-warden-cw03b/contract-before.json').read_text());assert contract['states']==old['states']
report['protected_hashes_unchanged']=len(protected);report['old_descriptors_and_mappings_unchanged']=True
(ROOT/'blender/cw03b_frame_validation.json').write_text(json.dumps(report,indent=2))
for state,review in contract['review_only_animations'].items():
 info=m['states'][state]
 assert contract['states'][state]['animation'] is None
 assert review['animation']==state and review['fps']==info['fps']==8
 assert review['frame_count']==info['frame_count']
 assert review['playback_mode']==('loop' if info['loop'] else 'once_then_hold')
 assert review['hold_frame_index']==(None if info['loop'] else info['frame_count']-1)
 assert review['activation']=='pending_visual_and_event_mapping_approval'
assert len(contract['review_only_animations'])==7
report['review_metadata_matches_manifest']=True
report['new_96px_sequence_signatures']={state:hashlib.sha256(b''.join(Image.open(ROOT/p).tobytes() for p in m['states'][state]['frames']['96'])).hexdigest() for state in contract['review_only_animations']}
assert len(set(report['new_96px_sequence_signatures'].values()))==7
inventory={path:hashlib.sha256((ROOT/path).read_bytes()).hexdigest() for info in m['states'].values() for paths in info['frames'].values() for path in paths}
(ROOT.parents[1]/'docs/reviews/core-warden-cw03b/frame-sha256.json').write_text(json.dumps(inventory,indent=2)+'\n')
(ROOT/'blender/cw03b_frame_validation.json').write_text(json.dumps(report,indent=2))
