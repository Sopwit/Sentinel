"""CW-02 refinement of the existing CW-01 master, never a scene reset.

Blender 5.2.2: blender --background --python-exit-code 1 --python THIS_FILE
Optional after --: --views front hero --samples 16 --resolution 640
The default produces five views plus 96/128/192px static front tests.
"""
import argparse
import bmesh
import bpy
import hashlib
import json
import math
import sys
import time
from pathlib import Path
from mathutils import Matrix, Vector

ROOT = Path(__file__).resolve().parents[1]
BASELINE = ROOT / 'blender/core_warden_cw01.blend'
REFERENCE = ROOT / 'reference/Core_Warden_Original_Reference.png'
OUT = ROOT / 'renders/cw02'
MASTER = ROOT / 'blender/core_warden_cw02.blend'
parser = argparse.ArgumentParser()
parser.add_argument('--views', nargs='+', default=['front', 'three_quarter', 'side', 'back', 'hero', 'sizes'])
parser.add_argument('--skip-renders', action='store_true', help='Rebuild only the derivative master; leave previews unchanged')
parser.add_argument('--samples', type=int, default=24)
parser.add_argument('--resolution', type=int, default=640)
args = parser.parse_args(sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else [])
assert 4 <= args.samples <= 64 and 96 <= args.resolution <= 1024
assert BASELINE.is_file() and REFERENCE.is_file(), 'Missing CW-01 master or original concept board'
OUT.mkdir(parents=True, exist_ok=True)
def digest(path): return hashlib.sha256(path.read_bytes()).hexdigest()
protected = [BASELINE, REFERENCE, ROOT/'scripts/build_cw01.py', ROOT/'scripts/validate_cw01.py', *sorted((ROOT/'renders/preview').glob('*.png'))]
protected_hashes = {str(p.relative_to(ROOT)): digest(p) for p in protected}
bpy.ops.wm.open_mainfile(filepath=str(BASELINE))
scene = bpy.context.scene
bpy.ops.object.select_all(action='DESELECT')
original_names = [o.name for o in bpy.data.objects if o.type == 'MESH']
core_mesh_hash = {name: hashlib.sha256(b''.join(bytes(str(tuple(v.co)), 'utf8') for v in bpy.data.objects[name].data.vertices)).hexdigest() for name in ['Core_Head', 'Face']}

# Reuse CW-01 material datablocks with their official linearized colors.
def linear_hex(h):
    values = [int(h[i:i+2], 16)/255 for i in (0, 2, 4)]
    return tuple(v/12.92 if v <= .04045 else ((v+.055)/1.055)**2.4 for v in values)
def material(name, h, metal, rough, emission=0):
    m = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    if not m.use_nodes: m.use_nodes = True
    p = m.node_tree.nodes.get('Principled BSDF')
    p.inputs['Base Color'].default_value = (*linear_hex(h), 1)
    p.inputs['Metallic'].default_value = metal
    p.inputs['Roughness'].default_value = rough
    p.inputs['Specular IOR Level'].default_value = .28
    p.inputs['Emission Color'].default_value = (*linear_hex(h), 1)
    p.inputs['Emission Strength'].default_value = emission
    m.diffuse_color = (*linear_hex(h), 1)
    m['official_srgb_hex'] = '#'+h
    return m
obs = material('Obsidian #151719', '151719', .18, .37)
por = material('Porcelain #ECEFEE', 'ECEFEE', .12, .29)
gra = material('Graphite #303438', '303438', .62, .34)
ice = material('Glacier #A9CAD3 emissive', 'A9CAD3', .05, .30, 1.25)
face_mat = obs.copy(); face_mat.name = 'CW02 Face | Obsidian #151719'; face_mat.node_tree.nodes.get('Principled BSDF').inputs['Roughness'].default_value = .32
bpy.data.objects['Face'].data.materials[0] = face_mat

collections = {}
for name in ['Controls', 'Core', 'Armor', 'Mechanical', 'Accents', 'Stage']:
    c = bpy.data.collections.new('CW02 | '+name); scene.collection.children.link(c); collections[name] = c

def move(o, group):
    for c in list(o.users_collection): c.objects.unlink(o)
    collections[group].objects.link(o)

def normalize_mesh(mesh):
    bm = bmesh.new(); bm.from_mesh(mesh)
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    bm.to_mesh(mesh); bm.free(); mesh.update()

def set_mesh(name, verts, faces, mats, group='Armor', bevel=.012, smooth=False, indices=None):
    """Refine a CW-01 object in place: same identity, new local editable mesh."""
    mesh = bpy.data.meshes.new('CW02 '+name+' geometry')
    mesh.from_pydata(verts, [], faces); normalize_mesh(mesh)
    o = bpy.data.objects.get(name)
    if o:
        old = o.data; o.data = mesh
        if old.users == 0: bpy.data.meshes.remove(old)
        o.modifiers.clear(); o.parent = None
    else:
        o = bpy.data.objects.new(name, mesh)
    o.matrix_world = Matrix.Identity(4)
    # Center local coordinates explicitly: never operate on accidental selections.
    pivot = sum((Vector(v) for v in verts), Vector()) / len(verts)
    for v in mesh.vertices: v.co -= pivot
    o.location = pivot
    move(o, group)
    for m in mats: mesh.materials.append(m)
    for i, p in enumerate(mesh.polygons):
        p.use_smooth = smooth
        if indices: p.material_index = indices[i]
    if bevel:
        mod = o.modifiers.new('CW02 | restrained machined edges', 'BEVEL')
        mod.width = bevel; mod.segments = 3; mod.limit_method = 'ANGLE'; mod.angle_limit = .38
        mod.harden_normals = True
        mod = o.modifiers.new('CW02 | weighted hard-surface normals', 'WEIGHTED_NORMAL'); mod.keep_sharp = True; mod.weight = 40
    o['source'] = 'CW01 refined in place' if name in original_names else 'CW02 supporting detail'
    return o

# Closed layered surface with real underside, boundary walls and optional facet materials.
def layered_grid(name, grid, inward, mats=(por, gra), group='Armor', bevel=.012, curved=False):
    rows, cols = len(grid), len(grid[0]); n = rows*cols
    outer = [Vector(v) for row in grid for v in row]
    inner = [v + Vector(inward(v)) for v in outer]
    verts = [tuple(v) for v in outer+inner]; faces=[]; ids=[]
    for r in range(rows-1):
        for c in range(cols-1):
            a=r*cols+c; q=(a,a+1,a+cols+1,a+cols)
            faces.extend([q, tuple(i+n for i in reversed(q))]); ids.extend([0, min(1,len(mats)-1)])
    boundary = list(range(cols)) + [r*cols+cols-1 for r in range(1,rows)] + list(range(n-2,n-cols-1,-1)) + [r*cols for r in range(rows-2,0,-1)]
    for a,b in zip(boundary,boundary[1:]+boundary[:1]):
        faces.append((a,b,b+n,a+n)); ids.append(min(1,len(mats)-1))
    obj=set_mesh(name, verts, faces, mats, group, bevel, indices=ids)
    if curved:
        for i,p in enumerate(obj.data.polygons):p.use_smooth=(i<2*(rows-1)*(cols-1) and i%2==0)
    return obj

def ribbon(name, points, thickness=.075, mats=(por, gra), bevel=.013):
    # points = x, z, half-width in XZ, y; a ruled swept blade with a front ridge.
    grid=[]
    for i,(x,z,w,y) in enumerate(points):
        a=points[max(0,i-1)]; b=points[min(len(points)-1,i+1)]
        tangent=Vector((b[0]-a[0],b[1]-a[1])).normalized(); normal=Vector((-tangent.y,tangent.x))
        grid.append([(x+normal.x*w*q, y+1.3*w*q-.035*(1-abs(q)), z+normal.y*w*q) for q in [-1,-.35,.35,1]])
    return layered_grid(name,grid,lambda v:(0,thickness,0),mats,bevel=bevel)

# Preserve original head/face mesh coordinates; refine response and support, not their identity.
for name in ['Core_Head','Face']:
    o=bpy.data.objects[name]; move(o,'Core'); o['source']='CW01 original mesh preserved'
for label in ['Left','Right']:
    o=bpy.data.objects[label+'_Mechanical_Hinge']; move(o,'Mechanical')
    o.scale=(.68,.52,.68)

# Asymmetric swept crown, with a diagonal separated tip segment.
crown_left = ribbon('Upper_Armor_Left', [(-.98,.28,.024,-.06),(-.99,.60,.055,-.10),(-.88,.93,.105,-.15),(-.65,1.18,.15,-.13),(-.34,1.32,.155,-.09)])
crown_tip = ribbon('Upper_Armor_Crown', [(-.30,1.34,.153,-.08),(-.06,1.43,.13,-.035),(.22,1.49,.018,.025)])
crown_right = ribbon('Upper_Armor_Right', [(.47,1.24,.022,.015),(.64,1.11,.075,-.005),(.83,.90,.09,-.015),(.99,.58,.055,.01),(.99,.30,.018,.065)], .065)

# Side shells: swept, twisted transverse surfaces, not extruded flat arc strips.
side_rows = [(1.23,.93,.030,-.02),(1.31,.72,.24,.01),(1.38,.39,.43,.04),(1.40,.02,.49,.07),(1.35,-.35,.43,.10),(1.24,-.69,.25,.14),(1.09,-.94,.020,.19)]
# Cubic interpolation removes longitudinal flat spots while preserving the loft design.
def refined_rows(rows):
    result=[]
    for i in range(len(rows)-1):
        a=Vector(rows[max(0,i-1)]);b=Vector(rows[i]);c=Vector(rows[i+1]);d=Vector(rows[min(len(rows)-1,i+2)])
        for j in range(4):
            t=j/4;v=.5*((2*b)+(-a+c)*t+(2*a-5*b+4*c-d)*t*t+(-a+3*b-3*c+d)*t*t*t);v[2]=max(.02,v[2]);result.append(tuple(v))
    return result+[rows[-1]]
side_rows=refined_rows(side_rows)
for label,sign in [('Left',-1),('Right',1)]:
    grid=[]; inset=[]
    for x,z,w,cy in side_rows:
        row=[]; inner=[]
        for q in [-1,-.45,0,.45,1]:
            # Angular ridge along the shell, with inward curling front and rear lips.
            xx=x + (.105*(1-abs(q)) - .15*q*q)*(w/.49)
            yy=cy+w*q
            zz=z + .065*q*(1-abs(z))
            row.append((sign*xx,yy,zz))
            inner.append((sign*(xx-.095), yy+.012, zz))
        grid.append(row); inset.append(inner)
    shell=layered_grid(label+'_Protective_Plate',grid,lambda v:(-sign*.065,0,0),bevel=.014,curved=True)
    layered_grid(label+'_Shell_Inner_Support',inset,lambda v:(-sign*.035,0,0),(gra,),group='Mechanical',bevel=.01,curved=True)
    # A restrained illuminated front-lip groove follows the shell's actual 3D twist.
    path=[(row[0][0]+sign*.007,row[0][1]-.008,row[0][2]) for row in grid[1:-1]]
    # Reuse the baseline edge name; tube() below replaces its obsolete mesh.
    shell['floating_gap_at_equator']=.15
    shell['geometry_note']='twisted loft with graphite underside and independent inner support'
    shell['edge_path']=json.dumps(path)

# Cheek armor is now three substantial curved sections with two deliberate seams.
def cheek(name,a,b):
    grid=[]
    for i in range(13):
        t=math.radians(a+(b-a)*i/12)
        width=.115 + .02*abs(math.sin(t))
        row=[]
        for q in [-1,-.25,.4,1]:
            radius=.95+width*q
            row.append((radius*math.sin(t), -.48-.12*abs(math.cos(t))-.025*(1-abs(q))-.18*(1-q)/2, radius*math.cos(t)))
        grid.append(row)
    return layered_grid(name,grid,lambda v:(0,.095,0),bevel=.016)
cheek('Lower_Armor',161,199)
cheek('Lower_Cheek_Right',116,160)
cheek('Lower_Cheek_Left',200,244)

# Conforming rear shell panels: no pinwheel/fan or flat empty back.
def rear_panel(name,outline):
    center=Vector((sum(p[0] for p in outline)/len(outline),sum(p[1] for p in outline)/len(outline)))
    boundary=[]
    for a,b in zip(outline,outline[1:]+outline[:1]):
        for i in range(3): boundary.append(Vector(a).lerp(Vector(b),i/3))
    points=[center]+[center.lerp(p,f) for f in [.25,.5,.75,1] for p in boundary]
    n=len(boundary); outer=[]
    for p in points:
        y=.86*math.sqrt(max(.025,1-p.length_squared))+.092
        outer.append((p.x,y,p.y))
    N=len(outer);verts=outer+[(x,y-.065,z) for x,y,z in outer]; faces=[];ids=[]
    surf=[]
    for i in range(n):
        j=(i+1)%n;surf.append((0,1+i,1+j))
        for ring in range(3):surf.append((1+ring*n+i,1+(ring+1)*n+i,1+(ring+1)*n+j,1+ring*n+j))
    for f in surf:faces.extend([f,tuple(i+N for i in reversed(f))]);ids.extend([0,1])
    for i in range(n):
        a=1+3*n+i;b=1+3*n+(i+1)%n;faces.append((a,b,b+N,a+N));ids.append(1)
    obj=set_mesh(name,verts,faces,(por,gra),bevel=.008,indices=ids)
    for i,p in enumerate(obj.data.polygons):p.use_smooth=(i<2*len(surf) and i%2==0)
    return obj
left_panels=[
    [(-.025,.06),(-.025,.65),(-.22,.88),(-.57,.77),(-.79,.42),(-.65,.08)],
    [(-.685,.055),(-.81,.375),(-.93,.08),(-.87,-.32),(-.66,-.49),(-.65,-.04)],
    [(-.025,.025),(-.62,.04),(-.63,-.50),(-.44,-.81),(-.16,-.91),(-.025,-.71)],
]
for index,poly in zip([0,2,4],left_panels):
    rear_panel('Rear_Armor_%02d'%index,poly)
    rear_panel('Rear_Armor_%02d'%(index+1),[(-x,z) for x,z in reversed(poly)])

# Closed polygonal extrusion with deliberate hard-surface cross-section.
def prism(name,outline,front,depth,mats=(por,gra),bevel=.012,group='Armor'):
    verts=[(x,front(x,z),z) for x,z in outline]
    n=len(verts);verts += [(x,y+depth,z) for x,y,z in verts]
    faces=[tuple(range(n)),tuple(range(2*n-1,n-1,-1))]
    faces += [(i,(i+1)%n,(i+1)%n+n,i+n) for i in range(n)]
    return set_mesh(name,verts,faces,mats,group,bevel,indices=[0, min(1,len(mats)-1)]+[min(1,len(mats)-1)]*n)
v_outline=[(-.47,-1.21),(-.33,-1.22),(0,-1.43),(.33,-1.22),(.47,-1.21),(.40,-1.36),(.065,-1.59),(0,-1.62),(-.065,-1.59),(-.40,-1.36)]
prism('Floating_V_Component',v_outline,lambda x,z:-.27-.06*(1-abs(x)/.48),.12,bevel=.022)
prism('Floating_V_Inner_Support',[(x*1.035,(z+1.42)*1.04-1.42) for x,z in v_outline],lambda x,z:-.185,.07,(gra,),.016,'Mechanical')

def tube(name,points,r,mat,group='Accents'):
    closed=(Vector(points[0])-Vector(points[-1])).length<1e-6
    if closed:points=points[:-1]
    curve=bpy.data.curves.new('CW02 '+name,'CURVE');curve.dimensions='3D';curve.resolution_u=8;curve.bevel_depth=r;curve.bevel_resolution=3;curve.use_fill_caps=True
    s=curve.splines.new('POLY');s.use_cyclic_u=closed;curve.use_fill_caps=not closed;s.points.add(len(points)-1)
    for p,co in zip(s.points,points): p.co=(*co,1)
    tmp=bpy.data.objects.new('CW02_TEMP',curve);collections[group].objects.link(tmp)
    bpy.ops.object.select_all(action='DESELECT');tmp.select_set(True);bpy.context.view_layer.objects.active=tmp;bpy.ops.object.convert(target='MESH')
    bm=bmesh.new();bm.from_mesh(tmp.data);bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=.000001);bm.to_mesh(tmp.data);bm.free()
    verts=[tuple(v.co) for v in tmp.data.vertices];faces=[tuple(p.vertices) for p in tmp.data.polygons]
    bpy.data.objects.remove(tmp,do_unlink=True)
    return set_mesh(name,verts,faces,(mat,),group,0,True)
for label in ['Left','Right']:
    tube(label+'_Plate_Glacier_Edge',json.loads(bpy.data.objects[label+'_Protective_Plate']['edge_path']),.009,ice)
tube('Floating_V_Glacier_Accent',[(-.28,-.337,-1.29),(0,-.35,-1.49),(.28,-.337,-1.29)],.012,ice)
tube('Crown_Glacier_Edge',[(-.94,-.15,.43),(-.9,-.18,.78),(-.72,-.19,1.05)],.008,ice)
# Recessed graphite orbital support behind the face, with a subtle visible edge.
points=[(.944*math.sin(2*math.pi*i/96),-.32,.944*math.cos(2*math.pi*i/96)) for i in range(97)]
tube('Core_Orbital_Support',points,.026,gra,'Mechanical')

# Eyes follow the actual preserved face ellipsoid, with tapered rounded outlines.
eye_materials=[]
for label,sign in [('Left',-1),('Right',1)]:
    m=material('CW02 '+label+' Eye | Glacier #A9CAD3','A9CAD3',0,.38,3.2);eye_materials.append(m)
    outline=[]
    angle=math.radians(38)
    for i in range(40):
        t=2*math.pi*i/40;u=.183*math.cos(t);v=.043*math.sin(t)*(1-.20*math.cos(t))
        x=sign*(.393+u*math.cos(angle)-v*math.sin(angle));z=.045+u*math.sin(angle)+v*math.cos(angle)
        outline.append((x,z))
    def eye_front(x,z):return -.36-.57*math.sqrt(max(.01,1-(x/.88)**2-(z/.86)**2))-.014
    # Subdivided conforming lens skin avoids a long planar ngon sinking into the face.
    center=Vector((sign*.393,.045)); boundary=[Vector(v) for v in outline]
    points=[center]+[center.lerp(p,f) for f in [.33,.66,1] for p in boundary]
    outer=[(p.x,eye_front(p.x,p.y),p.y) for p in points];n=len(boundary);N=len(outer)
    verts=outer+[(x,y+.025,z) for x,y,z in outer];faces=[]
    surf=[]
    for i in range(n):
        j=(i+1)%n;surf.append((0,1+i,1+j))
        for ring in range(2):surf.append((1+ring*n+i,1+(ring+1)*n+i,1+(ring+1)*n+j,1+ring*n+j))
    for f in surf:faces.extend([f,tuple(i+N for i in reversed(f))])
    for i in range(n):
        a=1+2*n+i;b=1+2*n+(i+1)%n;faces.append((a,b,b+N,a+N))
    set_mesh(label+'_Eye',verts,faces,(m,),'Core',.004,True)

# Controls are plain empties. No actions, keyframes, armatures or animation states.
def control(name,loc,parent=None,size=.22):
    o=bpy.data.objects.new(name,None);collections['Controls'].objects.link(o)
    o.empty_display_type='CIRCLE';o.empty_display_size=size;o.location=loc
    if parent:o.parent=parent
    o['usage']='Local transform control; CW02 has no authored animation'
    return o
root=control('CTRL_Float',(0,0,0),size=1.65)
head=control('CTRL_HeadMotion',(0,0,0),root,1.05)
def attach(o,parent):
    bpy.context.view_layer.update()
    matrix=o.matrix_world.copy();o.parent=parent;o.matrix_parent_inverse=parent.matrix_world.inverted();o.matrix_world=matrix
    bpy.context.view_layer.update()
for o in list(collections['Core'].objects):attach(o,head)
for o in list(collections['Armor'].objects)+list(collections['Mechanical'].objects)+list(collections['Accents'].objects):
    if not o.parent:attach(o,head)
for label,sign in [('Left',-1),('Right',1)]:
    ctrl=control('CTRL_'+label+'Shell',(sign*1.27,.05,0),root)
    for name in [label+'_Protective_Plate',label+'_Shell_Inner_Support',label+'_Plate_Glacier_Edge']:attach(bpy.data.objects[name],ctrl)
for name in ['Upper_Armor_Left','Upper_Armor_Crown','Upper_Armor_Right']:
    o=bpy.data.objects[name];ctrl=control('CTRL_'+name,tuple(o.location),head,.14);attach(o,ctrl)
    if name=='Upper_Armor_Left':attach(bpy.data.objects['Crown_Glacier_Edge'],ctrl)
ctrl=control('CTRL_LowerV',(0,-.24,-1.42),root,.20)
for name in ['Floating_V_Component','Floating_V_Inner_Support','Floating_V_Glacier_Accent']:attach(bpy.data.objects[name],ctrl)
root['eye_left_intensity']=3.2;root['eye_right_intensity']=3.2;root['accent_intensity']=1.25
for key in ['eye_left_intensity','eye_right_intensity','accent_intensity']:
    root.id_properties_ui(key).update(min=0,max=8,soft_max=5,description='Emission strength; keyframe this property in the future animation phase')
for m,key in [(eye_materials[0],'eye_left_intensity'),(eye_materials[1],'eye_right_intensity'),(ice,'accent_intensity')]:
    socket=m.node_tree.nodes.get('Principled BSDF').inputs['Emission Strength'];socket.driver_remove('default_value')
    driver=socket.driver_add('default_value').driver;driver.type='AVERAGE';var=driver.variables.new();var.name='intensity';var.type='SINGLE_PROP';var.targets[0].id=root;var.targets[0].data_path='["'+key+'"]'

# Evaluate newly added property drivers before render dependency graphs are built.
root.update_tag();scene.frame_set(1);bpy.context.view_layer.update()

# Existing five cameras retained, using a stable +Z up / -Y forward coordinate system.
for o in bpy.data.objects:
    if o.type in ['CAMERA','LIGHT']:move(o,'Stage')
for c in list(bpy.data.collections):
    if not c.objects and not c.children and c.name not in [x.name for x in collections.values()]: bpy.data.collections.remove(c)
def aim(o,p=(0,0,-.07)):o.rotation_euler=(Vector(p)-o.location).to_track_quat('-Z','Y').to_euler()
views={'front':(0,-6,0),'three_quarter':(3,-6,.85),'side':(6,0,0),'back':(0,6,0),'hero':(2.4,-6,.85)}
for name,loc in views.items():
    o=bpy.data.objects['Camera_'+name];o.location=loc;aim(o);o.data.type='ORTHO';o.data.ortho_scale=3.65;o.data.lens=50
light_config={'Key':((-3,-4,5),430,4.5),'Fill':((3,-4,.6),150,3.5),'Rim':((1.5,3,3.2),650,3)}
for name,(loc,power,size) in light_config.items():
    o=bpy.data.objects[name];o.location=loc;o.data.energy=power;o.data.size=size;o.data.color=(1,1,1);aim(o)
scene.world.use_nodes=True
background=scene.world.node_tree.nodes.get('Background');background.inputs['Color'].default_value=(.12,.12,.12,1);background.inputs['Strength'].default_value=.3
scene.render.engine='CYCLES';scene.cycles.device='CPU';scene.cycles.samples=args.samples;scene.cycles.seed=2;scene.cycles.use_animated_seed=False;scene.cycles.use_denoising=True
scene.render.resolution_percentage=100;scene.render.image_settings.file_format='PNG';scene.render.image_settings.color_mode='RGBA';scene.render.image_settings.color_depth='8'
scene.render.film_transparent=True;scene.render.use_file_extension=True
scene.view_settings.view_transform='AgX';scene.view_settings.look='AgX - Medium High Contrast';scene.view_settings.exposure=-.25
scene.render.fps=24
scene['asset_version']='CW02';scene['source_master']='core_warden_cw01.blend';scene['coordinate_system']='+Z up, -Y forward, +X character left in front view'
scene['baseline_sha256']=protected_hashes['blender/core_warden_cw01.blend']
scene['review_status']='Static refinement: visual approval required before animation'
scene['performance']='No textures, simulations, armature or subdivision. Eevee viewport / modest CPU Cycles previews.'
# Save a recovery master before rendering; a failed preview never destroys the geometry work.
scene.camera=bpy.data.objects['Camera_hero'];scene.render.resolution_x=scene.render.resolution_y=args.resolution
bpy.ops.wm.save_as_mainfile(filepath=str(MASTER))
render_times={}
def render(name,camera,size,hero=False):
    scene.camera=bpy.data.objects['Camera_'+camera];scene.render.resolution_x=scene.render.resolution_y=size
    scene.render.film_transparent=not hero
    background.inputs['Color'].default_value=(.009,.014,.022,1) if hero else (.12,.12,.12,1)
    background.inputs['Strength'].default_value=.6 if hero else .3
    scene.render.filepath=str(OUT/(name+'.png'))
    started=time.monotonic();bpy.ops.render.render(write_still=True);render_times[name]=round(time.monotonic()-started,2)
    print('CW02_RENDER_OK',name,flush=True)
for view in ([] if args.skip_renders else args.views):
    if view=='sizes':
        for size in [96,128,192]:render('size_'+str(size),'front',size)
    else:
        assert view in views,view
        render(view,view,args.resolution,view=='hero')
scene.camera=bpy.data.objects['Camera_hero'];scene.render.resolution_x=scene.render.resolution_y=640;scene.render.film_transparent=True
background.inputs['Color'].default_value=(.12,.12,.12,1);background.inputs['Strength'].default_value=.3
scene.render.engine='BLENDER_EEVEE'
bpy.ops.object.select_all(action='DESELECT');root.select_set(True);bpy.context.view_layer.objects.active=root
# Remove unused CW01 material/mesh blocks from the derivative only.
for mesh in list(bpy.data.meshes):
    if mesh.users==0:bpy.data.meshes.remove(mesh)
for curve in list(bpy.data.curves):
    if curve.users==0:bpy.data.curves.remove(curve)
for m in list(bpy.data.materials):
    if m.users==0:bpy.data.materials.remove(m)
bpy.ops.wm.save_as_mainfile(filepath=str(MASTER))
assert all(digest(ROOT/p)==h for p,h in protected_hashes.items()),'Protected CW01 asset was changed'
assert all(hashlib.sha256(b''.join(bytes(str(tuple(v.co)), 'utf8') for v in bpy.data.objects[n].data.vertices)).hexdigest()==h for n,h in core_mesh_hash.items()),'Baseline core topology changed'
meshes=[o for o in scene.objects if o.type=='MESH']
summary={'blender_version':bpy.app.version_string,'baseline_sha256':protected_hashes,'core_meshes_preserved':True,'retained_baseline_objects':original_names,'mesh_objects':len(meshes),'base_vertices':sum(len(o.data.vertices) for o in meshes),'base_faces':sum(len(o.data.polygons) for o in meshes),'control_objects':[o.name for o in collections['Controls'].objects],'render_seconds':render_times,'samples':args.samples,'resolution':args.resolution,'script_sha256':digest(Path(__file__))}
(ROOT/'blender/cw02_build_manifest.json').write_text(json.dumps(summary,indent=2))
print('CW02_BUILD_OK',json.dumps(summary),flush=True)
