"""Build three periodic, independently selectable Actions from the approved master."""
import bpy,json,math,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
bpy.ops.wm.open_mainfile(filepath=str(ROOT/'blender/core_warden_cw02_1.blend'))
s=bpy.context.scene
controls=sorted((o for o in s.objects if o.type=='EMPTY'),key=lambda o:o.name)
rest={o.name:{'location':list(o.location),'rotation_euler':list(o.rotation_euler),'scale':list(o.scale)} for o in controls}
props={k:float(bpy.data.objects['CTRL_Float'][k]) for k in ['eye_left_intensity','eye_right_intensity','accent_intensity']}
protected={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in (ROOT/'blender').glob('core_warden_cw0*.blend') if p.name!='core_warden_cw03a.blend'}
geometry={o.name:hashlib.sha256(json.dumps([list(v.co) for v in o.data.vertices]).encode()).hexdigest() for o in s.objects if o.type=='MESH'}
# (sine amplitude, one-minus-cosine amplitude, harmonic)
def motion(state,n,path,i):
 sign=-1 if 'Left' in n else 1
 if path.startswith('['):
  if 'eye_' in path:return ((.22 if state=='idle' else (.55*(-1 if 'left' in path else 1) if state=='thinking' else .35)),.08 if state=='idle' else .15,2 if state=='tool_running' else 1)
  return (.06,.12 if state=='idle' else .3,1)
 if path=='scale':return (0,0,1)
 if n=='CTRL_Float' and path=='location' and i==2:return (.025 if state=='idle' else .014,0,1)
 if n=='CTRL_HeadMotion' and path=='rotation_euler':
  if state=='idle':return ((.008 if i==2 else 0),.006 if i==0 else 0,1)
  if state=='thinking':return (.018 if i==1 else 0,.012 if i==2 else 0,1)
  return (.003 if i==2 else 0,0,1)
 if 'Shell' in n:
  if path=='location' and i==0:return (sign*.007,sign*(.012 if state=='tool_running' else .004),1)
  if path=='location' and i==2:return (.004 if state=='idle' else .008,0,2)
  if path=='rotation_euler' and i==1:return (sign*(.005 if state=='idle' else .012),0,1)
 if 'Upper_Armor' in n and state=='thinking' and path=='location':
  if i==0 and 'Crown' not in n:return (0,sign*.008,1)
  if i==2:return (0,.004,1)
 if n=='CTRL_LowerV':
  if path=='location' and i==2:return (.008,.004 if state=='idle' else .009,1)
  if path=='rotation_euler' and i==1:return (.006 if state=='idle' else .012,0,1)
 return (0,0,1)
states={}
for state,count in [('idle',16),('thinking',16),('tool_running',12)]:
 action=bpy.data.actions.new('CW03A_'+state.upper());action.use_fake_user=True;action['state_id']=state;action['loop_frames']=count
 slots={}
 for o in controls:
  o.animation_data_create();o.animation_data.action=action
  slot=action.slots.new(id_type='OBJECT',name=o.name);o.animation_data.action_slot=slot;slots[o.name]=slot.identifier
  channels=[(p,i,v) for p in ['location','rotation_euler','scale'] for i,v in enumerate(rest[o.name][p])]
  if o.name=='CTRL_Float':channels += [('['+json.dumps(k)+']',-1,v) for k,v in props.items()]
  for path,i,base in channels:
   a,b,k=motion(state,o.name,path,i)
   for f in range(1,count+2):
    theta=(f-1)*2*math.pi/count;offset=0 if f in (1,count+1) else a*math.sin(k*theta)+b*(1-math.cos(k*theta))
    if i<0:o[path[2:-2]]=base+offset;o.keyframe_insert(data_path=path,frame=f)
    else:getattr(o,path)[i]=base+offset;o.keyframe_insert(data_path=path,index=i,frame=f)
  bag=action.layers[0].strips[0].channelbag(slot)
  for fc in bag.fcurves:
   a,b,k=motion(state,o.name,fc.data_path,fc.array_index if not fc.data_path.startswith('[') else -1)
   for key in fc.keyframe_points:
    f,y=key.co;theta=(f-1)*2*math.pi/count;slope=2*math.pi*k/count*(a*math.cos(k*theta)+b*math.sin(k*theta))
    key.interpolation='BEZIER';key.handle_left_type=key.handle_right_type='FREE';key.handle_left=(f-1/3,y-slope/3);key.handle_right=(f+1/3,y+slope/3)
   fc.modifiers.new('CYCLES')
 states[state]={'action':action.name,'slots':slots,'frame_count':count,'fps':8,'loop':True,'duration_seconds':count/8,'playback_mode':'loop','source_scene_frames':[1,count],'closing_keyframe':count+1}
helper='''import bpy,json

def select_state(state):
    scene=bpy.context.scene
    info=json.loads(bpy.data.texts['CW03A_states.json'].as_string())[state]
    action=bpy.data.actions[info['action']]
    for name,identifier in info['slots'].items():
        obj=bpy.data.objects[name]
        obj.animation_data_create()
        obj.animation_data.action=action
        obj.animation_data.action_slot=next(slot for slot in action.slots if slot.identifier==identifier)
    scene.frame_start=1
    scene.frame_end=info['frame_count']
    scene['active_state']=state
    scene.frame_set(1)
    bpy.context.view_layer.update()

def neutral_pose():
    scene=bpy.context.scene
    rest=json.loads(bpy.data.texts['CW03A_rest.json'].as_string())
    for name,values in rest['controls'].items():
        obj=bpy.data.objects[name]
        obj.animation_data_clear()
        for channel,value in values.items():setattr(obj,channel,value)
    for key,value in rest['properties'].items():bpy.data.objects['CTRL_Float'][key]=value
    bpy.data.objects['CTRL_Float'].update_tag()
    scene['active_state']='neutral'
    scene.frame_set(1)
    bpy.context.view_layer.update()
'''
for name,data in [('CW03A_states.json',json.dumps(states)),('CW03A_rest.json',json.dumps({'controls':rest,'properties':props})),('CW03A_select_state.py',helper)]:bpy.data.texts.new(name).write(data)
exec(helper);select_state('idle')
s.render.engine='BLENDER_EEVEE';s.eevee.taa_render_samples=32;s.render.fps=8;s.render.fps_base=1
s.render.resolution_x=s.render.resolution_y=192;s.render.resolution_percentage=100;s.render.film_transparent=True;s.render.image_settings.file_format='PNG';s.render.image_settings.color_mode='RGBA';s.render.image_settings.color_depth='8';s.render.image_settings.compression=15
s.camera=bpy.data.objects['Camera_hero'];s['asset_version']='CW03A';s['source_asset']='CW02.1'
manifest={'asset_id':'sentinel_core_warden','version':'CW03A','source':'CW02.1','blender_version':bpy.app.version_string,'engine':s.render.engine,'samples':32,'camera':s.camera.name,'ortho_scale':s.camera.data.ortho_scale,'canvas':[192,192],'resolutions':[96,128,192],'protected_sha256':protected,'geometry_sha256':geometry,'states':states,'rest':{'controls':rest,'properties':props}}
(ROOT/'blender/cw03a_manifest.json').write_text(json.dumps(manifest,indent=2))
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'blender/core_warden_cw03a.blend'))
print('CW03A_BUILD_OK',flush=True)
