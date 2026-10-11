"""Reopen master, select each real Action and export only its unique loop frames."""
import bpy,json,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
s=bpy.context.scene
exec(bpy.data.texts['CW03A_select_state.py'].as_string())
manifest=json.loads((ROOT/'blender/cw03a_manifest.json').read_text())
timings=[]
for state,info in manifest['states'].items():
 select_state(state);directory=ROOT/'animations'/state/'192';directory.mkdir(parents=True,exist_ok=True)
 paths=[]
 for index in range(info['frame_count']):
  s.frame_set(index+1);path=directory/f'frame_{index:03d}.png';s.render.filepath=str(path)
  start=time.perf_counter();bpy.ops.render.render(write_still=True);elapsed=time.perf_counter()-start
  timings.append({'state':state,'frame':index,'seconds':elapsed});paths.append(str(path.relative_to(ROOT)))
 info['frames']={'192':paths}
manifest['render_timings']=timings
(ROOT/'blender/cw03a_manifest.json').write_text(json.dumps(manifest,indent=2))
print('CW03A_EXPORT_OK',sum(x['seconds'] for x in timings),flush=True)
