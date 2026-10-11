"""Read-only Blender reopen audit. No file is saved by this validator."""
import bpy
import bmesh
import hashlib
import json
import math
from array import array
from pathlib import Path
from mathutils import Vector
from mathutils.bvhtree import BVHTree
ROOT=Path(__file__).resolve().parents[1]
manifest=json.loads((ROOT/'blender/cw02_1_manifest.json').read_text())
assert bpy.context.scene['asset_version']=='CW02.1'
scene=bpy.context.scene
required=[n for n,d in manifest['baseline_structure'].items() if d['type']=='MESH']
meshes=[o for o in scene.objects if o.type=='MESH']
assert all(n in bpy.data.objects and bpy.data.objects[n].type=='MESH' for n in required)
assert len(meshes)==manifest['mesh_count']
report={'blender_version':bpy.app.version_string,'baseline_preserved':True,'meshes':{},'controls':{},'previews':{},'surface_crossings':[]}
for path,checksum in manifest['protected_sha256'].items():
    assert hashlib.sha256((ROOT/path).read_bytes()).hexdigest()==checksum,path
def structure(o):
 return {'type':o.type,'parent':o.parent.name if o.parent else None,'location':list(o.location),'rotation':list(o.rotation_euler),'scale':list(o.scale),'parent_inverse':[list(r) for r in o.matrix_parent_inverse],'matrix_basis':[list(r) for r in o.matrix_basis],'vertices':len(o.data.vertices) if o.type=='MESH' else None,'edges':len(o.data.edges) if o.type=='MESH' else None,'polygons':hashlib.sha256(json.dumps([list(p.vertices) for p in o.data.polygons]).encode()).hexdigest() if o.type=='MESH' else None,'edge_topology':hashlib.sha256(json.dumps([list(e.vertices) for e in o.data.edges]).encode()).hexdigest() if o.type=='MESH' else None,'collections':sorted(c.name for c in o.users_collection)}

def shader_values(m):
 p=m.node_tree.nodes.get('Principled BSDF');return {i.name:list(i.default_value) if hasattr(i.default_value,'__len__') else i.default_value for i in p.inputs if hasattr(i,'default_value') and isinstance(i.default_value,(int,float,tuple)) or hasattr(i,'default_value') and hasattr(i.default_value,'__len__')}

assert {o.name:structure(o) for o in scene.objects if o.type in ['MESH','EMPTY']}==manifest['baseline_structure'],'CW02 names/topology/pivots/parenting changed'
assert {m.name:shader_values(m) for m in bpy.data.materials if m.get('official_srgb_hex')=='#151719'}==manifest['obsidian_shader'],'Obsidian changed'
for n,h in manifest['core_geometry_sha256'].items():
    assert hashlib.sha256(json.dumps([list(v.co) for v in bpy.data.objects[n].data.vertices]).encode()).hexdigest()==h,n
report['exact_structure_preserved']=True
report['obsidian_unchanged']=True

for o in meshes:
    assert o.data.vertices and o.data.polygons and o.data.materials,o.name
    assert all(m and m.use_nodes for m in o.data.materials),o.name
    assert all(math.isfinite(c) for v in o.data.vertices for c in v.co),o.name
    bm=bmesh.new();bm.from_mesh(o.data)
    nonmanifold=sum(not e.is_manifold for e in bm.edges)
    volume=bm.calc_volume(signed=True)
    bm.free()
    assert not nonmanifold,(o.name,nonmanifold)
    assert volume>1e-9,(o.name,volume)
    assert o.parent and o.parent.name.startswith('CTRL_'),o.name
    report['meshes'][o.name]={'vertices':len(o.data.vertices),'faces':len(o.data.polygons),'manifold':True,'signed_volume':round(volume,7),'parent':o.parent.name}
report['official_materials']={}
for m in bpy.data.materials:
    h=m.get('official_srgb_hex')
    assert h and len(h)==7,m.name
    rgb=[int(h[i:i+2],16)/255 for i in (1,3,5)]
    rgb=[v/12.92 if v<=.04045 else ((v+.055)/1.055)**2.4 for v in rgb]
    p=m.node_tree.nodes.get('Principled BSDF');assert p,m.name
    assert all(abs(a-b)<1e-6 for a,b in zip(p.inputs['Base Color'].default_value[:3],rgb)),m.name
    if h=='#A9CAD3':assert all(abs(a-b)<1e-6 for a,b in zip(p.inputs['Emission Color'].default_value[:3],rgb)),m.name
    report['official_materials'][m.name]={'srgb_hex':h,'roughness':p.inputs['Roughness'].default_value,'metallic':p.inputs['Metallic'].default_value}
assert len(bpy.data.actions)==0 and not any(o.type=='ARMATURE' for o in scene.objects)
assert not any(i.source=='FILE' and not i.packed_file for i in bpy.data.images),'Unexpected external texture dependency'
for name in [n for n,d in manifest['baseline_structure'].items() if d['type']=='EMPTY']:
    ctrl=bpy.data.objects[name];assert ctrl.type=='EMPTY'
    bpy.context.view_layer.update()
    before={o.name:o.matrix_world.copy() for o in meshes};saved=ctrl.location.copy()
    ctrl.location.x += .017;bpy.context.view_layer.update()
    moved=[]
    for o in meshes:
        parents=[];p=o.parent
        while p:parents.append(p);p=p.parent
        expected=ctrl in parents
        actual=(o.matrix_world.translation-before[o.name].translation).length>1e-5
        assert expected==actual,(ctrl.name,o.name,expected,actual)
        if actual:moved.append(o.name)
    ctrl.location=saved;bpy.context.view_layer.update()
    report['controls'][name]={'independent_transform_test':True,'affected_parts':moved}
# Verify rotation channels as well as translation, without keyframes or saves.
for name in [n for n,d in manifest['baseline_structure'].items() if d['type']=='EMPTY']:
    ctrl=bpy.data.objects[name];bpy.context.view_layer.update()
    before={o.name:o.matrix_world.copy() for o in meshes};saved=ctrl.rotation_euler.copy()
    ctrl.rotation_euler.z+=.041;bpy.context.view_layer.update()
    for o in meshes:
        parents=[];p=o.parent
        while p:parents.append(p);p=p.parent
        changed=max(abs(o.matrix_world[r][c]-before[o.name][r][c]) for r in range(4) for c in range(4))>1e-5
        assert changed==(ctrl in parents),(ctrl.name,o.name,'rotation control')
    ctrl.rotation_euler=saved;bpy.context.view_layer.update()
    report['controls'][name]['independent_rotation_test']=True
root=bpy.data.objects['CTRL_Float']
left=bpy.data.objects['Left_Eye'].data.materials[0];right=bpy.data.objects['Right_Eye'].data.materials[0]
assert left!=right
leftsocket=left.node_tree.nodes.get('Principled BSDF').inputs['Emission Strength']
rightsocket=right.node_tree.nodes.get('Principled BSDF').inputs['Emission Strength']
old=root['eye_left_intensity'];root['eye_left_intensity']=.73;root.update_tag();scene.frame_set(scene.frame_current);bpy.context.view_layer.update()
assert abs(leftsocket.default_value-.73)<1e-4,leftsocket.default_value
assert abs(rightsocket.default_value-root['eye_right_intensity'])<1e-4,rightsocket.default_value
root['eye_left_intensity']=old;root.update_tag();scene.frame_set(scene.frame_current);bpy.context.view_layer.update()
# Both eye channels and the shared accent channel retain their original bindings.
for key,mat in [('eye_left_intensity',left),('eye_right_intensity',right),('accent_intensity',bpy.data.materials['Glacier #A9CAD3 emissive'])]:
    old=root[key];root[key]=.65;root.update_tag();scene.frame_set(scene.frame_current);bpy.context.view_layer.update()
    assert abs(mat.node_tree.nodes.get('Principled BSDF').inputs['Emission Strength'].default_value-.65)<1e-4,key
    root[key]=old;root.update_tag();scene.frame_set(scene.frame_current);bpy.context.view_layer.update()
report['all_emission_channels_functional']=True
report['independent_eye_emission']=True
cameras=[o for o in scene.objects if o.type=='CAMERA'];assert len(cameras)==5
assert len({tuple(o.location) for o in cameras})==5
assert all(abs(o.data.ortho_scale-3.65)<1e-4 for o in cameras)
report['cameras']={o.name:{'location':list(o.location),'ortho_scale':o.data.ortho_scale} for o in cameras}
for name,size,transparent in [('front',512,True),('three_quarter',512,True),('side',512,True),('hero',768,True),('size_96',96,True),('size_128',128,True)]:
    path=ROOT/'renders/cw02_1'/(name+'.png');assert path.is_file(),str(path)
    image=bpy.data.images.load(str(path),check_existing=False)
    assert tuple(image.size)==(size,size) and image.channels==4,(name,tuple(image.size),image.channels)
    pixels=array('f',[0])*(size*size*4);image.pixels.foreach_get(pixels);alpha=pixels[3::4]
    assert max(alpha)>.99,name
    if transparent:assert min(alpha)==0,name
    else:assert min(alpha)>.99,name
    # Check transparent silhouette never hits the image frame.
    if transparent:
        edge=[alpha[i] for i in range(size)]+[alpha[(size-1)*size+i] for i in range(size)]+[alpha[i*size] for i in range(size)]+[alpha[i*size+size-1] for i in range(size)]
        assert max(edge)==0,(name,'clipped image frame')
    report['previews'][name]={'size':size,'RGBA':True,'transparent':transparent,'frame_clear':True}
    bpy.data.images.remove(image)
# Evaluate real modifier output in world space. Embedded eyes and mechanical
# supports are intentionally excluded; only detached outer armor vs head/face.
bpy.context.view_layer.update();dg=bpy.context.evaluated_depsgraph_get()
def tree(o):
    ev=o.evaluated_get(dg);mesh=ev.to_mesh();verts=[ev.matrix_world@v.co for v in mesh.vertices];faces=[tuple(p.vertices) for p in mesh.polygons]
    result=BVHTree.FromPolygons(verts,faces);ev.to_mesh_clear();return result
coretrees={n:tree(bpy.data.objects[n]) for n in ['Core_Head','Face']}
for o in bpy.data.collections['CW02 | Armor'].objects:
    t=tree(o)
    for name,ct in coretrees.items():
        pairs=t.overlap(ct)
        if pairs:report['surface_crossings'].append({'outer_armor':o.name,'core':name,'triangle_pairs':len(pairs)})
report['external_file_dependencies']=[]
report['base_vertices']=sum(len(o.data.vertices) for o in meshes)
triangles=0
for o in meshes:
    ev=o.evaluated_get(dg);mesh=ev.to_mesh();mesh.calc_loop_triangles();triangles+=len(mesh.loop_triangles);ev.to_mesh_clear()
report['evaluated_triangles']=triangles
(ROOT/'blender/cw02_1_validation.json').write_text(json.dumps(report,indent=2))
assert not report['surface_crossings'],report['surface_crossings']
print('CW021_VALIDATION_OK',json.dumps({'mesh_objects':len(meshes),'base_vertices':report['base_vertices'],'controls':len(report['controls']),'previews':len(report['previews']),'surface_crossings':report['surface_crossings']}),flush=True)
