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
  webp=Image.open(ROOT/'previews/cw03a'/f'{state}{suffix}.webp');assert webp.n_frames==info['frame_count']
  duration=0
  for index in range(webp.n_frames):
   webp.seek(index);webp.load();assert webp.size==(size,size);duration+=webp.info['duration']
  assert duration==info['frame_count']*125
 report['states'][state]={'frames_per_resolution':info['frame_count'],'96px_two_eye_islands_every_frame':True,'96px_eye_pixel_area_min':min(min(a) for a in areas),'webp_duration_ms':duration}
report['RGBA_dimensions_alpha_frame_counts_paths_pass']=True
(ROOT/'blender/cw03a_frame_validation.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
