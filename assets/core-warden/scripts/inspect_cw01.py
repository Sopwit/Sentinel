import bpy, json
from pathlib import Path
out=Path(__file__).resolve().parents[1]/'blender'/'cw01_scene_inspection.json'
data={'version':bpy.app.version_string,'objects':[]}
for o in bpy.data.objects:
    data['objects'].append({'name':o.name,'type':o.type,'location':list(o.location),'scale':list(o.scale),'parent':o.parent.name if o.parent else None,'vertices':len(o.data.vertices) if o.type=='MESH' else None,'materials':[m.name for m in o.data.materials] if o.type=='MESH' else [],'modifiers':[m.type for m in o.modifiers],'camera_scale':o.data.ortho_scale if o.type=='CAMERA' else None})
out.write_text(json.dumps(data,indent=2));print('CW01_INSPECT_OK',out)
