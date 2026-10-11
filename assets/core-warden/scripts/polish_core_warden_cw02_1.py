"""Non-destructive CW-02.1 polish: load CW-02, edit vertices, retain rig/topology.
Blender --background --python-exit-code 1 --python THIS_FILE
Optional: -- --draft (front/three-quarter only, 16 samples).
"""
import bpy, bmesh, hashlib, json, math, sys, time
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[1]
SOURCE=ROOT/'blender/core_warden_cw02.blend'
TARGET=ROOT/'blender/core_warden_cw02_1.blend'
OUT=ROOT/'renders/cw02_1';OUT.mkdir(parents=True,exist_ok=True)
assert SOURCE.exists() and (ROOT/'reference/Core_Warden_Original_Reference.png').exists()
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
protected=[ROOT/'blender/core_warden_cw01.blend',SOURCE,ROOT/'scripts/build_cw01.py',ROOT/'scripts/build_core_warden_cw02.py',ROOT/'scripts/validate_core_warden_cw02.py',ROOT/'reference/Core_Warden_Original_Reference.png',*sorted((ROOT/'renders/preview').glob('*.png')),*sorted((ROOT/'renders/cw02').glob('*.png'))]
hashes={str(p.relative_to(ROOT)):sha(p) for p in protected}
bpy.ops.wm.open_mainfile(filepath=str(SOURCE));scene=bpy.context.scene;bpy.context.view_layer.update()
meshes=[o for o in scene.objects if o.type=='MESH'];assert len(meshes)==29
# Serialized topology and transforms permit an exact independent reopen comparison.
def structure(o):
 return {'type':o.type,'parent':o.parent.name if o.parent else None,'location':list(o.location),'rotation':list(o.rotation_euler),'scale':list(o.scale),'parent_inverse':[list(r) for r in o.matrix_parent_inverse],'matrix_basis':[list(r) for r in o.matrix_basis],'vertices':len(o.data.vertices) if o.type=='MESH' else None,'edges':len(o.data.edges) if o.type=='MESH' else None,'polygons':hashlib.sha256(json.dumps([list(p.vertices) for p in o.data.polygons]).encode()).hexdigest() if o.type=='MESH' else None,'edge_topology':hashlib.sha256(json.dumps([list(e.vertices) for e in o.data.edges]).encode()).hexdigest() if o.type=='MESH' else None,'collections':sorted(c.name for c in o.users_collection)}
baseline={o.name:structure(o) for o in scene.objects if o.type in ['MESH','EMPTY']}
def shader_values(m):
 p=m.node_tree.nodes.get('Principled BSDF');return {i.name:list(i.default_value) if hasattr(i.default_value,'__len__') else i.default_value for i in p.inputs if hasattr(i,'default_value') and isinstance(i.default_value,(int,float,tuple)) or hasattr(i,'default_value') and hasattr(i.default_value,'__len__')}
obs_values={m.name:shader_values(m) for m in bpy.data.materials if m.get('official_srgb_hex')=='#151719'}
core_geometry={name:[list(v.co) for v in bpy.data.objects[name].data.vertices] for name in ['Core_Head','Face']}

def deform(name,fn):
 o=bpy.data.objects[name];world=o.matrix_world.copy();inv=world.inverted()
 for v in o.data.vertices:v.co=inv@Vector(fn(world@v.co))
 o.data.update()
 bm=bmesh.new();bm.from_mesh(o.data);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(o.data);bm.free()
# Side shell assemblies turn their porcelain exterior toward the front and open
# a controlled floating gap; all three existing parts receive the same mapping.
for label,sign in [('Left',-1),('Right',1)]:
 def shell(p):
  a=-sign*math.radians(19);dx=p.x-sign*1.30;dy=p.y-.08
  x=sign*1.30+math.cos(a)*dx-math.sin(a)*dy+sign*.085
  y=.08+math.sin(a)*dx+math.cos(a)*dy-.035
  return (x,y,p.z)
 for suffix in ['Protective_Plate','Shell_Inner_Support','Plate_Glacier_Edge']:deform(label+'_'+suffix,shell)
# Continuous crown sweep preserves the shared diagonal gap and trim alignment.
def crown(p):
 t=max(0,min(1,(p.z-.65)/.86));return (p.x+.12*t*t,p.y-.035*t,p.z+.015*t)
for name in ['Upper_Armor_Left','Upper_Armor_Crown','Crown_Glacier_Edge']:deform(name,crown)
def small_crown(p):return (p.x+.022,p.y,p.z-.025*max(0,min(1,(p.z-.3)/.94)))
deform('Upper_Armor_Right',small_crown)
# Fuller luminous lenses, with the skin conformed to the untouched black face.
for label,sign in [('Left',-1),('Right',1)]:
 def eye(p):
  old_y=-.36-.57*math.sqrt(max(.01,1-(p.x/.88)**2-(p.z/.86)**2))-.014
  u=sign*p.x-.393;v=p.z-.045;a=math.radians(38)
  longitudinal=(u*math.cos(a)+v*math.sin(a))*1.035
  width=(-u*math.sin(a)+v*math.cos(a))*1.28
  a=math.radians(35.5)
  x=sign*(.393+longitudinal*math.cos(a)-width*math.sin(a));z=.045+longitudinal*math.sin(a)+width*math.cos(a)
  y=-.36-.57*math.sqrt(max(.01,1-(x/.88)**2-(z/.86)**2))-.016+(p.y-old_y)
  return (x,y,z)
 deform(label+'_Eye',eye)
root=bpy.data.objects['CTRL_Float'];root['eye_left_intensity']=4.8;root['eye_right_intensity']=4.8;root['accent_intensity']=1.8
# Wider, slightly deeper chevron with its existing support and accent unchanged in topology.
def lower_v(p):return (p.x*1.10,-.24+(p.y+.24)*1.10,-1.42+(p.z+1.42)*1.055)
for name in ['Floating_V_Component','Floating_V_Inner_Support','Floating_V_Glacier_Accent']:deform(name,lower_v)
for mod in bpy.data.objects['Floating_V_Component'].modifiers:
 if mod.type=='BEVEL':mod.width=.016
root.update_tag();scene.frame_set(1);bpy.context.view_layer.update()
# Lighting creates sphere readability without recoloring/re-roughening Obsidian.
def aim(o,target=(0,0,-.07)):o.rotation_euler=(Vector(target)-o.location).to_track_quat('-Z','Y').to_euler()
settings={'Key':((-3.5,-4,2.8),430,(1,1,1),5.0,4.0),'Fill':((3.5,-4,.8),240,(.88,.95,1),5.0,4.0),'Rim':((2,2.2,3.5),720,(.84,.94,1),3.0,2.0)}
for name,(loc,power,color,sx,sy) in settings.items():
 o=bpy.data.objects[name];o.location=loc;o.data.energy=power;o.data.color=color;o.data.shape='RECTANGLE';o.data.size=sx;o.data.size_y=sy;aim(o)
bg=scene.world.node_tree.nodes.get('Background');bg.inputs['Color'].default_value=(.12,.12,.12,1);bg.inputs['Strength'].default_value=.50
scene.view_settings.exposure=0.0
scene.render.engine='CYCLES';scene.cycles.device='CPU';scene.cycles.samples=16 if '--draft' in sys.argv else 24;scene.cycles.seed=2;scene.cycles.use_denoising=True
scene.render.resolution_percentage=100;scene.render.image_settings.file_format='PNG';scene.render.image_settings.color_mode='RGBA';scene.render.image_settings.color_depth='8';scene.render.film_transparent=True
scene['asset_version']='CW02.1';scene['source_master']=SOURCE.name;scene['review_status']='Final visual polish; static approval pending'
assert {o.name:structure(o) for o in scene.objects if o.type in ['MESH','EMPTY']}==baseline,'Topology, naming, pivot or rig structure changed'
assert {m.name:shader_values(m) for m in bpy.data.materials if m.get('official_srgb_hex')=='#151719'}==obs_values,'Obsidian shader changed'
assert all([list(v.co) for v in bpy.data.objects[n].data.vertices]==coords for n,coords in core_geometry.items())
scene.camera=bpy.data.objects['Camera_hero'];scene.render.resolution_x=scene.render.resolution_y=512
bpy.ops.wm.save_as_mainfile(filepath=str(TARGET))
times={}
views=[('front','front',512),('three_quarter','three_quarter',512)] if '--draft' in sys.argv else [('front','front',512),('three_quarter','three_quarter',512),('side','side',512),('hero','hero',768),('size_96','front',96),('size_128','front',128)]
for name,camera,size in views:
 scene.camera=bpy.data.objects['Camera_'+camera];scene.render.resolution_x=scene.render.resolution_y=size;scene.render.film_transparent=True
 scene.render.filepath=str(OUT/(name+'.png'));started=time.monotonic();bpy.ops.render.render(write_still=True);times[name]=round(time.monotonic()-started,3);print('CW021_RENDER_OK',name,flush=True)
scene.camera=bpy.data.objects['Camera_hero'];scene.render.resolution_x=scene.render.resolution_y=512;scene.render.engine='BLENDER_EEVEE';scene.render.film_transparent=True
bpy.ops.wm.save_as_mainfile(filepath=str(TARGET))
assert all(sha(ROOT/p)==h for p,h in hashes.items())
manifest={'version':bpy.app.version_string,'protected_sha256':hashes,'baseline_structure':baseline,'obsidian_shader':obs_values,'core_geometry_sha256':{n:hashlib.sha256(json.dumps(coords).encode()).hexdigest() for n,coords in core_geometry.items()},'mesh_count':29,'base_vertices':sum(len(o.data.vertices) for o in meshes),'render_seconds':times,'samples':scene.cycles.samples,'transparent_views':[v[0] for v in views],'script_sha256':sha(Path(__file__))}
(ROOT/'blender/cw02_1_manifest.json').write_text(json.dumps(manifest,indent=2))
print('CW021_BUILD_OK',json.dumps({'meshes':29,'vertices':manifest['base_vertices'],'render_seconds':times}),flush=True)
