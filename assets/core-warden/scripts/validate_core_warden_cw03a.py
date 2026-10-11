"""Read-only reopened master audit, including half-frame evaluated armor clearance."""
import bpy,json,hashlib,math
from pathlib import Path
from mathutils.bvhtree import BVHTree
ROOT=Path(__file__).resolve().parents[1];s=bpy.context.scene
m=json.loads((ROOT/'blender/cw03a_manifest.json').read_text());exec(bpy.data.texts['CW03A_select_state.py'].as_string())
report={'reopened':True,'states':{},'surface_crossings':[]}
for p,h in m['protected_sha256'].items():assert hashlib.sha256((ROOT/p).read_bytes()).hexdigest()==h,p
meshes=[o for o in s.objects if o.type=='MESH'];assert len(meshes)==29
for o in meshes:assert hashlib.sha256(json.dumps([list(v.co) for v in o.data.vertices]).encode()).hexdigest()==m['geometry_sha256'][o.name],o.name
assert len(bpy.data.actions)==3
baseline=json.loads((ROOT/'blender/cw02_1_manifest.json').read_text())['baseline_structure']
for o in meshes:
 b=baseline[o.name]
 assert list(o.location)==b['location'] and list(o.rotation_euler)==b['rotation'] and list(o.scale)==b['scale']
 assert o.parent.name==b['parent'] and [list(r) for r in o.matrix_parent_inverse]==b['parent_inverse']
def tree(o,dg):
 ev=o.evaluated_get(dg);mesh=ev.to_mesh();result=BVHTree.FromPolygons([ev.matrix_world@v.co for v in mesh.vertices],[tuple(p.vertices) for p in mesh.polygons]);ev.to_mesh_clear();return result
neutral_pose()
for name,b in baseline.items():
 o=bpy.data.objects[name]
 assert o.type==b['type'] and (o.parent.name if o.parent else None)==b['parent']
 assert list(o.location)==b['location'] and list(o.rotation_euler)==b['rotation'] and list(o.scale)==b['scale']
 assert [list(r) for r in o.matrix_parent_inverse]==b['parent_inverse']
 assert sorted(c.name for c in o.users_collection)==b['collections']
 if o.type=='MESH':
  assert len(o.data.edges)==b['edges'] and len(o.data.vertices)==b['vertices']
  assert hashlib.sha256(json.dumps([list(p.vertices) for p in o.data.polygons]).encode()).hexdigest()==b['polygons']
  assert hashlib.sha256(json.dumps([list(e.vertices) for e in o.data.edges]).encode()).hexdigest()==b['edge_topology']
def shader_values(mat):
 p=mat.node_tree.nodes.get('Principled BSDF')
 return {i.name:list(i.default_value) if hasattr(i.default_value,'__len__') else i.default_value for i in p.inputs if hasattr(i,'default_value') and (isinstance(i.default_value,(int,float,tuple)) or hasattr(i.default_value,'__len__'))}
source=json.loads((ROOT/'blender/cw02_1_manifest.json').read_text())
assert {mat.name:shader_values(mat) for mat in bpy.data.materials if mat.get('official_srgb_hex')=='#151719'}==source['obsidian_shader']
report['topology_names_pivots_parenting_collections_preserved']=True
report['obsidian_shader_unchanged']=True
bpy.context.view_layer.update();dg=bpy.context.evaluated_depsgraph_get()
armor=list(bpy.data.collections['CW02 | Armor'].objects)
resttrees={o.name:tree(o,dg) for o in armor}
existing_pairs={(a.name,b.name) for i,a in enumerate(armor) for b in armor[i+1:] if resttrees[a.name].overlap(resttrees[b.name])}
report['rest_armor_contact_pairs']=sorted(existing_pairs)
report['new_armor_crossings']=[]
for state,info in m['states'].items():
 select_state(state);action=bpy.data.actions[info['action']];assert len(action.slots)==8
 curves=[fc for layer in action.layers for strip in layer.strips for slot in action.slots for fc in strip.channelbag(slot).fcurves]
 for fc in curves:
  first,last=fc.keyframe_points[0],fc.keyframe_points[-1]
  assert abs(first.co.y-last.co.y)<1e-7
  slope0=(first.handle_right.y-first.co.y)/(first.handle_right.x-first.co.x)
  slope1=(last.co.y-last.handle_left.y)/(last.co.x-last.handle_left.x)
  assert abs(slope0-slope1)<2e-5,(state,fc.data_path,'seam velocity')
  assert any(mod.type=='CYCLES' for mod in fc.modifiers)
 for half in range(2*info['frame_count']+1):
  t=1+half/2;s.frame_set(int(t),subframe=t%1);bpy.context.view_layer.update();dg=bpy.context.evaluated_depsgraph_get()
  cores={n:tree(bpy.data.objects[n],dg) for n in ['Core_Head','Face']}
  for o in bpy.data.collections['CW02 | Armor'].objects:
   tr=tree(o,dg)
   for n,ct in cores.items():
    if tr.overlap(ct):report['surface_crossings'].append([state,t,o.name,n])
  armor_trees={o.name:tree(o,dg) for o in armor}
  for i,a in enumerate(armor):
   for b in armor[i+1:]:
    if (a.name,b.name) not in existing_pairs and armor_trees[a.name].overlap(armor_trees[b.name]):report['new_armor_crossings'].append([state,t,a.name,b.name])
  root=bpy.data.objects['CTRL_Float']
  for side in ['left','right']:
   value=root[f'eye_{side}_intensity'];assert 4<value<6
   mat=bpy.data.materials[f'CW02 {side.title()} Eye | Glacier #A9CAD3']
   assert abs(mat.node_tree.nodes['Principled BSDF'].inputs['Emission Strength'].default_value-value)<1e-4
 report['states'][state]={'slots':len(action.slots),'fcurves':len(curves),'periodic_position_and_velocity':True,'clearance_samples':2*info['frame_count']+1,'eye_drivers_functional':True}
neutral_pose()
for name,rest in m['rest']['controls'].items():
 o=bpy.data.objects[name]
 for channel,value in rest.items():assert max(abs(a-b) for a,b in zip(getattr(o,channel),value))<1e-7
# Manual translation and rotation control checks in neutral, without modifying saved file.
for name in m['rest']['controls']:
 ctrl=bpy.data.objects[name]
 for channel in ['location','rotation_euler']:
  bpy.context.view_layer.update();before={o.name:o.matrix_world.copy() for o in meshes};old=getattr(ctrl,channel).copy();getattr(ctrl,channel).z+=.017;bpy.context.view_layer.update()
  for o in meshes:
   p=o.parent;expected=False
   while p:
    expected |= p==ctrl;p=p.parent
   changed=max(abs(o.matrix_world[r][c]-before[o.name][r][c]) for r in range(4) for c in range(4))>1e-6
   assert changed==expected,(name,channel,o.name)
  setattr(ctrl,channel,old)
root=bpy.data.objects['CTRL_Float']
for key,matname in [('eye_left_intensity','CW02 Left Eye | Glacier #A9CAD3'),('eye_right_intensity','CW02 Right Eye | Glacier #A9CAD3'),('accent_intensity','Glacier #A9CAD3 emissive')]:
 old=root[key];root[key]=.73;root.update_tag();s.frame_set(1);bpy.context.view_layer.update()
 assert abs(bpy.data.materials[matname].node_tree.nodes['Principled BSDF'].inputs['Emission Strength'].default_value-.73)<1e-4,key
 root[key]=old;root.update_tag();s.frame_set(1);bpy.context.view_layer.update()
report['three_independent_emission_controls_functional']=True
report.update(meshes=29,controls=8,geometry_unchanged=True,protected_masters_unchanged=True,neutral_return=True,independent_controls_functional=True,transparent=s.render.film_transparent,engine=s.render.engine,samples=s.eevee.taa_render_samples)
(ROOT/'blender/cw03a_validation.json').write_text(json.dumps(report,indent=2))
assert not report['surface_crossings'],report['surface_crossings']
assert not report['new_armor_crossings'],report['new_armor_crossings']
print('CW03A_VALIDATION_OK',flush=True)
