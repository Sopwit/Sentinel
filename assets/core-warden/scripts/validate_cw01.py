import bpy, bmesh, json
from pathlib import Path
root=Path(__file__).resolve().parents[1]
required=['Core_Head','Face','Left_Eye','Right_Eye','Upper_Armor_Left','Lower_Armor','Left_Protective_Plate','Right_Protective_Plate','Rear_Armor_00','Floating_V_Component']
for name in required:
    o=bpy.data.objects.get(name)
    assert o and o.type=='MESH' and len(o.data.vertices)>0,name
    assert len(o.data.materials)>0,name
assert bpy.data.objects['Left_Eye'].data.materials[0] != bpy.data.objects['Right_Eye'].data.materials[0]
for o in bpy.data.collections['CW01 | Independently animatable parts'].objects:
    bm=bmesh.new();bm.from_mesh(o.data);assert all(e.is_manifold for e in bm.edges),o.name;bm.free()
assert len({tuple(o.location) for o in bpy.data.objects if o.type=='CAMERA'})==5
assert len([o for o in bpy.data.objects if o.type=='CAMERA'])==5
results={}
for name in ['front','three_quarter','side','back','hero']:
    p=root/'renders'/'preview'/('cw01_'+name+'.png');assert p.exists(),str(p)
    im=bpy.data.images.load(str(p),check_existing=False)
    assert im.size[:]==(640,640) and im.channels==4
    alpha=list(im.pixels)[3::4];assert min(alpha)==0 and max(alpha)>.99
    results[name]={'width':im.size[0],'height':im.size[1],'transparent':True}
print('CW01_VALIDATION_OK',json.dumps(results))
