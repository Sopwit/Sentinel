"""Lossless PNG optimization, Lanczos variants, previews and sequence validation."""
from PIL import Image,ImageDraw,ImageFont,ImageChops
from pathlib import Path
import json,hashlib,statistics
ROOT=Path(__file__).resolve().parents[1];m=json.loads((ROOT/'blender/cw03b_manifest.json').read_text())
preview=ROOT/'previews/cw03b';preview.mkdir(parents=True,exist_ok=True)
font=ImageFont.truetype('/System/Library/Fonts/Supplemental/Arial.ttf',16)
small=ImageFont.truetype('/System/Library/Fonts/Supplemental/Arial.ttf',12)
before=after=0;counts={};duplicates={};seams={}
def backdrop(im,size=None):
 if size:im=im.resize((size,size),Image.Resampling.LANCZOS)
 bg=Image.new('RGBA',im.size,(36,41,48,255));bg.alpha_composite(im);return bg.convert('RGB')
for state,info in m['states'].items():
 frames=[];hashes=[]
 info.update(animation_id='core_warden_'+state,state=state,canvas_dimensions={str(z):[z,z] for z in m['resolutions']},available_resolutions=m['resolutions'])
 for size in [128,96]:info['frames'][str(size)]=[]
 for path in info['frames']['192']:
  p=ROOT/path;im=Image.open(p).convert('RGBA');before+=p.stat().st_size
  for size in [192,128,96]:
   out=p if size==192 else ROOT/'animations'/state/str(size)/p.name
   out.parent.mkdir(parents=True,exist_ok=True)
   variant=im if size==192 else im.resize((size,size),Image.Resampling.LANCZOS)
   if size!=192:
    variant.save(out,compress_level=1);before+=out.stat().st_size
   digest=hashlib.sha256(variant.tobytes()).hexdigest()
   variant.save(out,optimize=True,compress_level=9)
   check=Image.open(out);assert check.mode=='RGBA' and check.size==(size,size)
   assert hashlib.sha256(check.tobytes()).hexdigest()==digest
   alpha=check.getchannel('A');assert alpha.getextrema()==(0,255)
   assert not alpha.crop((0,0,size,1)).getbbox() and not alpha.crop((0,size-1,size,size)).getbbox()
   assert not alpha.crop((0,0,1,size)).getbbox() and not alpha.crop((size-1,0,size,size)).getbbox()
   after+=out.stat().st_size;counts[str(size)]=counts.get(str(size),0)+1
   if size!=192:info['frames'][str(size)].append(str(out.relative_to(ROOT)))
  frames.append(im.copy());hashes.append(hashlib.sha256(im.tobytes()).hexdigest())
 duplicates[state]=len(hashes)-len(set(hashes))
 if info['loop']:assert hashes[-1]!=hashes[0]
 # Difference on an opaque neutral backing; compare seam against every adjacent step.
 rgb=[backdrop(im) for im in frames]
 def delta(a,b):return sum(ImageChops.difference(a,b).convert('L').getdata())/(a.width*a.height)
 steps=[delta(rgb[i],rgb[(i+1)%len(rgb)]) for i in range(len(rgb))]
 seams[state]={'last_to_first_mean_absolute_luma_delta':steps[-1],'adjacent_min':min(steps),'adjacent_max':max(steps),'adjacent_mean':statistics.mean(steps)}
 frames[0].save(preview/(state+'.webp'),save_all=True,append_images=frames[1:],duration=125,loop=0 if info['loop'] else 1,lossless=True,method=6)
 info['preview']=str((preview/(state+'.webp')).relative_to(ROOT))
 for size in [96,128]:
  variants=[Image.open(ROOT/p).convert('RGBA') for p in info['frames'][str(size)]]
  variants[0].save(preview/f'{state}_{size}.webp',save_all=True,append_images=variants[1:],duration=125,loop=0 if info['loop'] else 1,lossless=True,method=6)
 # A browser-friendly GIF has a deliberate solid approval background. PNG/WebP retain alpha.
 gif=[backdrop(im).quantize(colors=128) for im in frames]
 gif[0].save(preview/(state+'.gif'),save_all=True,append_images=gif[1:],duration=[120 if i%2==0 else 130 for i in range(len(gif))],**({'loop':0} if info['loop'] else {}),optimize=False,disposal=2)
 # Representative native 96px views, with source-size views above.
 row=Image.new('RGB',(4*208,340),(25,29,35));draw=ImageDraw.Draw(row)
 for col,index in enumerate([0,len(frames)//4,len(frames)//2,3*len(frames)//4]):
  row.paste(backdrop(frames[index]),(col*208+8,28));row.paste(backdrop(Image.open(ROOT/info['frames']['96'][index])),(col*208+56,233));draw.text((col*208+8,5),f'{state} / {index:03d}',font=small,fill='white')
 row.save(preview/(state+'_strip.png'))
# One native-size review page includes existing CW03A previews without rewriting them.
original=json.loads((ROOT/'animations/manifest.json').read_text())
# Reruns preserve original three descriptors even after the manifest was extended.
existing={k:original['states'][k] for k in ['idle','thinking','tool_running']}
all_states={**existing,**m['states']}
sheet=Image.new('RGB',(5*240,2*350),(25,29,35));draw=ImageDraw.Draw(sheet)
html='<!doctype html><meta charset="utf-8"><title>Core Warden · ten states</title><style>body{background:#242930;color:#ecefee;font:15px system-ui;margin:24px}.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(220px,1fr));gap:14px}section{background:#191d23;padding:16px;border-radius:10px;text-align:center}img{display:block;margin:10px auto}small{color:#b9c4d2}button{padding:10px;background:#a9cad3;color:#151719;border:0;border-radius:6px}</style><h1>Core Warden CW-03B · all ten states · 8 FPS</h1><p>Assets for visual approval only. Existing production event mappings remain unchanged. One-shots hold neutral final pose; replay to inspect again.</p><button onclick="document.querySelectorAll(\'img\').forEach(i=>{let u=i.dataset.src;i.src=\'\';requestAnimationFrame(()=>i.src=u+\'?replay=\'+Date.now())})">Replay previews</button><div class="grid">'
for index,(state,info) in enumerate(all_states.items()):
 folder='cw03a' if state in existing else 'cw03b';label=state.replace('_',' ').title();x=index%5*240;y=index//5*350
 draw.text((x+12,y+10),label,font=font,fill='white')
 frames=info['frames'];sheet.paste(backdrop(Image.open(ROOT/frames['192'][len(frames['192'])//4])),(x+24,y+42));sheet.paste(backdrop(Image.open(ROOT/frames['96'][len(frames['96'])//4])),(x+72,y+245));draw.text((x+12,y+330),'96px / '+('loop' if info['loop'] else 'once → hold'),font=small,fill='white')
 html+=f'<section><h2>{label}</h2><small>{info["frame_count"]} frames · {info["frame_count"]/8:g}s · {"loop" if info["loop"] else "once → hold"}</small>'
 for size,suffix in [(192,''),(128,'_128'),(96,'_96')]:
  url=f'../{folder}/{state}{suffix}.webp';html+=f'<img src="{url}" data-src="{url}" width="{size}" height="{size}" alt="{label} {size}px"><small>{size}px</small>'
 html+='</section>'
html+='</div>';sheet.save(preview/'all_ten_contact_sheet.png');(preview/'approval.html').write_text(html)
times=[x['seconds'] for x in m['render_timings']]
metrics={'png_frames':counts,'total_png_files':sum(counts.values()),'bytes_before_optimization':before,'bytes_after_optimization':after,'bytes_saved':before-after,'saved_percent':100*(before-after)/before,'duplicates_within_states':duplicates,'render_seconds_total':sum(times),'render_seconds_average':statistics.mean(times),'render_seconds_min':min(times),'render_seconds_max':max(times),'decoded_rgba_bytes_by_resolution':{str(z):sum(i['frame_count'] for i in m['states'].values())*z*z*4 for z in m['resolutions']},'decoded_all_variants_bytes':sum(i['frame_count'] for i in m['states'].values())*sum(z*z*4 for z in m['resolutions']),'seam_pixel_deltas':seams,'alpha_and_dimensions_valid':True,'compression_pixel_exact':True,'all_manifest_paths_resolve':True,'preview_bytes':sum(p.stat().st_size for p in preview.glob('*') if p.is_file())}
m['metrics']=metrics;m['memory_note']='Decode one chosen resolution and state as needed. Disk PNG size is not decoded RGBA memory. No terminal runtime performance measured.'
for info in m['states'].values():
 for paths in info['frames'].values():assert len(paths)==info['frame_count'] and all((ROOT/p).is_file() and (ROOT/p).stat().st_size for p in paths)
merged={**original,'version':'CW03B','states':all_states,'cw03b':{'source_master':'blender/core_warden_cw03b.blend','review_only_states':list(m['states']),'metrics':metrics}}
(ROOT/'animations/manifest.json').write_text(json.dumps(merged,indent=2));(ROOT/'blender/cw03b_packaging_validation.json').write_text(json.dumps(metrics,indent=2))
print(json.dumps(metrics,indent=2))

(ROOT/'blender/cw03b_manifest.json').write_text(json.dumps(m,indent=2))
