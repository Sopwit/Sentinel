"""Pillow comparison/readability audit; never modifies source previews."""
from pathlib import Path
import json, math, statistics
from PIL import Image, ImageDraw, ImageFont
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'renders/cw02_1';BEFORE=ROOT/'renders/cw02'
BG=(10,17,23);TEXT=(232,239,239);MUTED=(159,186,195);BORDER=(43,63,73)
def font(n):
 for p in ['/System/Library/Fonts/Supplemental/Arial.ttf','/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf']:
  if Path(p).exists():return ImageFont.truetype(p,n)
 return ImageFont.load_default(size=n)
canvas=Image.new('RGB',(1536,1140),BG);d=ImageDraw.Draw(canvas)
d.text((32,24),'SENTINEL  /  CORE WARDEN CW-02.1',font=font(30),fill=TEXT)
d.text((32,66),'Same 29 meshes, topology, pivots and controls • Obsidian unchanged • static visual polish',font=font(17),fill=MUTED)
def panel(x,y,w,h,title,path,crop=None):
 d.rounded_rectangle((x,y,x+w,y+h),radius=12,outline=BORDER,width=1);d.text((x+16,y+14),title,font=font(20),fill=TEXT)
 im=Image.open(path).convert('RGBA')
 if crop:im=im.crop(crop)
 im.thumbnail((w-24,h-60),Image.Resampling.LANCZOS);canvas.paste(im,(x+(w-im.width)//2,y+50+(h-60-im.height)//2),im)
panel(24,106,480,480,'APPROVED CONCEPT',ROOT/'reference/Core_Warden_Original_Reference.png',(274,25,655,392))
panel(528,106,480,480,'CW-02  /  BEFORE',BEFORE/'hero.png')
panel(1032,106,480,480,'CW-02.1  /  AFTER',OUT/'hero.png')
for i,(title,p) in enumerate([('BEFORE / FRONT',BEFORE/'front.png'),('AFTER / FRONT',OUT/'front.png'),('BEFORE / SIDE',BEFORE/'side.png'),('AFTER / SIDE',OUT/'side.png')]):panel(24+i*258,610,240,305,title,p)
d.rounded_rectangle((1060,610,1512,1108),radius=12,outline=BORDER,width=1)
d.text((1076,625),'NATIVE SPRITE SIZES',font=font(20),fill=TEXT)
for n,y in [(96,700),(128,875)]:
 for x,folder,label in [(1090,BEFORE,'BEFORE'),(1300,OUT,'AFTER')]:
  im=Image.open(folder/('size_'+str(n)+'.png')).convert('RGBA');assert im.size==(n,n)
  canvas.paste(im,(x,y),im);d.text((x,y-30),str(n)+' px / '+label,font=font(15),fill=MUTED)
d.text((32,960),'Visible changes: fuller eyes, broader shell faces and floating gaps, longer swept crown and sharper V.',font=font(19),fill=TEXT)
d.text((32,1003),'Remaining differences: angular crown, regular cheek contour and simplified shell/rear panel shapes.',font=font(17),fill=MUTED)
d.text((32,1040),'Transparent assets are composited here only for review. Original concept crop retains source lettering.',font=font(16),fill=MUTED)
canvas.save(OUT/'comparison.png')
results={'sizes':{},'face_readability':{}}
def eye_counts(im):
 n=im.width;counts=[]
 for sign in [-1,1]:
  lo,hi=sorted([.16*sign,.64*sign]);count=0
  for y in range(int(n*(.5-(.27+.07)/3.65)),int(n*(.5-(-.16+.07)/3.65))+1):
   for x in range(int(n*(.5+lo/3.65)),int(n*(.5+hi/3.65))+1):
    r,g,b,a=im.getpixel((x,y))
    if a>200 and b>160 and b-r>8:count+=1
  counts.append(count)
 return counts
for n in [96,128]:
 before=eye_counts(Image.open(BEFORE/('size_'+str(n)+'.png')).convert('RGBA'));after=eye_counts(Image.open(OUT/('size_'+str(n)+'.png')).convert('RGBA'))
 assert min(after)>min(before),(n,before,after)
 results['sizes'][str(n)]={'before_bright_glacier_pixels':before,'after_bright_glacier_pixels':after,'two_distinct_regions':True}
for label,folder in [('before',BEFORE),('after',OUT)]:
 im=Image.open(folder/'front.png').convert('RGBA').resize((512,512),Image.Resampling.LANCZOS);vals=[]
 for py in range(512):
  z=(.5-py/512)*3.65-.07
  for px in range(512):
   x=(px/512-.5)*3.65
   if x*x+z*z>.74**2 or (.15<abs(x)<.65 and -.16<z<.27):continue
   r,g,b,a=im.getpixel((px,py))
   if a>250:vals.append(.2126*r+.7152*g+.0722*b)
 results['face_readability'][label]={'mean_display_luminance_0_255':round(statistics.mean(vals),2),'p95_display_luminance':round(sorted(vals)[int(.95*len(vals))],2),'note':'Matched front-view face region excluding eyes; lighting proxy, not a quality score'}
(ROOT/'blender/cw02_1_readability.json').write_text(json.dumps(results,indent=2))
print('CW021_COMPARISON_OK',json.dumps(results))
