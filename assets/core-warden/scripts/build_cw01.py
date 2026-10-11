"""Deterministic CW-01 master. Run with Blender --background --python-exit-code 1 --python this_file."""
import bpy, bmesh, math, json, sys
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[1]
bpy.ops.object.select_all(action='SELECT'); bpy.ops.object.delete(use_global=False)
for c in list(bpy.data.collections):
    if c.name != 'Collection': bpy.data.collections.remove(c)
parts=bpy.data.collections.new('CW01 | Independently animatable parts'); bpy.context.scene.collection.children.link(parts)
def mat(name,h,metal,rough,em=0):
    rgb=[int(h[i:i+2],16)/255 for i in (0,2,4)]
    rgb=[v/12.92 if v<=.04045 else ((v+.055)/1.055)**2.4 for v in rgb]
    m=bpy.data.materials.new(name); m.use_nodes=True; p=m.node_tree.nodes.get('Principled BSDF'); p.inputs['Base Color'].default_value=(*rgb,1); p.inputs['Metallic'].default_value=metal; p.inputs['Roughness'].default_value=rough
    p.inputs['Emission Color'].default_value=(*rgb,1); p.inputs['Emission Strength'].default_value=em
    return m
obs=mat('Obsidian #151719','151719',.35,.26); por=mat('Porcelain #ECEFEE','ECEFEE',.18,.27); gra=mat('Graphite #303438','303438',.7,.3); ice=mat('Glacier #A9CAD3 emissive','A9CAD3',.15,.23,5)
def finish(o,name,m):
    o.name=name; o.data.materials.append(m)
    for c in list(o.users_collection): c.objects.unlink(o)
    parts.objects.link(o)
    for p in o.data.polygons:p.use_smooth=True
    return o
def sphere(name,loc,scale,m):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=48,ring_count=32,location=loc); o=bpy.context.object; o.scale=scale; bpy.ops.object.transform_apply(location=False,rotation=False,scale=True); return finish(o,name,m)
sphere('Core_Head',(0,0,0),(1, .86,1),obs)
sphere('Face',(0,-.36,0),(.88,.57,.86),obs)
# Spherical panel patches: angle measured from top, front is negative Y.
def patch(name,a,b,r,depth,m,front=-.48,steps=20):
    verts=[]
    span=.67 if 'Protective_Plate' in name else .24
    for y,rad in [(front,r),(front, r-depth),(front+span,r),(front+span,r-depth)]:
        for i in range(steps+1):
            t=math.radians(a+(b-a)*i/steps); u=i/steps; rr=rad
            if 'Protective_Plate' in name:
                rr = r + .08*math.sin(math.pi*u) if rad==r else r-(.035+.29*math.sin(math.pi*u))
            if name=='Upper_Armor_Left': rr += .19*u
            verts.append((rr*math.sin(t),y,rr*math.cos(t)))
    n=steps+1; faces=[]
    for i in range(steps):
        for s,t in [(0,n),(2*n,3*n),(0,2*n),(n,3*n)]:faces.append((s+i,s+i+1,t+i+1,t+i))
    faces += [(0,n,3*n,2*n),(steps,n+steps,3*n+steps,2*n+steps)]
    mesh=bpy.data.meshes.new(name); mesh.from_pydata(verts,[],faces); mesh.update(); bm=bmesh.new(); bm.from_mesh(mesh); bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces)); bm.to_mesh(mesh); bm.free(); o=bpy.data.objects.new(name,mesh); parts.objects.link(o); o.data.materials.append(m)
    bevel=o.modifiers.new('Manufactured edge bevel','BEVEL'); bevel.width=.035; bevel.segments=3
    o.modifiers.new('Weighted surface normals','WEIGHTED_NORMAL')
    bpy.context.view_layer.objects.active=o; o.select_set(True); bpy.ops.object.origin_set(type='ORIGIN_GEOMETRY',center='BOUNDS'); o.select_set(False)
    return o
patch('Upper_Armor_Left',-73,-18,1.15,.19,por, -.24)
patch('Upper_Armor_Crown',-18,8,1.15,.19,por,-.24)
patch('Upper_Armor_Right',27,69,1.13,.13,por,-.18)
patch('Lower_Armor',126,234,1.01,.22,por,-.67)
for side,s in [('Left',-1),('Right',1)]:
    patch(side+'_Protective_Plate',s*65,s*130,1.4,.19,por,-.11)
    patch(side+'_Plate_Glacier_Edge',s*68,s*125,1.405,.025,ice,-.14)
    sphere(side+'_Mechanical_Hinge',(s*.96,.06,0),(.11,.25,.19),gra)
# Rear segmented cap, with a graphite seam between each plate.
for i,(a,b) in enumerate([(-62,-2),(2,62),(65,119),(122,178),(182,238),(241,295)]):patch('Rear_Armor_%02d'%i,a,b,.97,.68,por,.58)
def tube(name,coords,r,m):
    curve=bpy.data.curves.new(name,'CURVE'); curve.dimensions='3D'; curve.resolution_u=12; curve.bevel_depth=r; curve.bevel_resolution=4; curve.use_fill_caps=True
    sp=curve.splines.new('BEZIER'); sp.bezier_points.add(len(coords)-1)
    for p,co in zip(sp.bezier_points,coords):p.co=co;p.handle_left_type='AUTO';p.handle_right_type='AUTO'
    if 'Floating_V' in name:
        for p in sp.bezier_points:p.handle_left_type='VECTOR';p.handle_right_type='VECTOR'
    o=bpy.data.objects.new(name,curve); parts.objects.link(o);o.data.materials.append(m.copy() if name.endswith('_Eye') else m)
    bpy.context.view_layer.objects.active=o;o.select_set(True);bpy.ops.object.convert(target='MESH');bm=bmesh.new();bm.from_mesh(o.data);bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=.00001);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(o.data);bm.free();bpy.ops.object.origin_set(type='ORIGIN_GEOMETRY',center='BOUNDS');o.select_set(False);return o
for s,label in [(-1,'Left'),(1,'Right')]:tube(label+'_Eye',[(s*.25,-.916,-.08),(s*.38,-.91,.02),(s*.53,-.87,.14)],.047,ice)
tube('Floating_V_Component',[(-.43,-.12,-1.16),(0,-.2,-1.49),(.43,-.12,-1.16)],.075,por)
tube('Floating_V_Glacier_Accent',[(-.32,-.205,-1.27),(0,-.28,-1.49),(.32,-.205,-1.27)],.022,ice)
scene=bpy.context.scene; scene.render.engine='CYCLES'; scene.cycles.samples=16
# CPU Cycles is portable in headless mode; Eevee is saved for interactive previews.
scene.render.resolution_x=640; scene.render.resolution_y=640;scene.render.resolution_percentage=100;scene.render.image_settings.file_format='PNG';scene.render.image_settings.color_mode='RGBA';scene.render.film_transparent=True
scene.world.color=(.18,.18,.18)
def aim(o,p=(0,0,-.12)):o.rotation_euler=(Vector(p)-o.location).to_track_quat('-Z','Y').to_euler()
for name,loc,power,size in [('Key',(-3,-4,5),650,4),('Fill',(4,-2,1),400,3),('Rim',(0,3,4),850,3)]:
    bpy.ops.object.light_add(type='AREA',location=loc);o=bpy.context.object;o.name=name;o.data.energy=power;o.data.shape='DISK';o.data.size=size;aim(o)
views={'front':(0,-6,0),'three_quarter':(3,-6,1.2),'side':(6,0,0),'back':(0,6,0),'hero':(2.4,-6,1)}
for name,loc in views.items():
    bpy.ops.object.camera_add(location=loc);o=bpy.context.object;o.name='Camera_'+name;o.data.type='ORTHO';o.data.ortho_scale=3.65;aim(o);scene.camera=o
    scene.render.filepath=str(ROOT/'renders'/'preview'/('cw01_'+name+'.png'));
    if '--skip-preview-renders' not in sys.argv: bpy.ops.render.render(write_still=True)
scene.camera=bpy.data.objects['Camera_hero'];scene.render.engine='BLENDER_EEVEE'
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'blender'/'core_warden_cw01.blend'))
print('CW01_BUILD_OK',json.dumps({'version':bpy.app.version_string,'mesh_objects':len([o for o in parts.objects if o.type=='MESH']),'vertices':sum(len(o.data.vertices) for o in parts.objects if o.type=='MESH')}))
