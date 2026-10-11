"""Static review montage and pixel-scale readability audit; uses Pillow only.
Run with a Python environment containing Pillow. Does not alter source renders.
"""
from pathlib import Path
import json
from PIL import Image, ImageDraw, ImageFont
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'renders/cw02'
BG=(10,17,23);BORDER=(43,63,73);TEXT=(232,239,239);MUTED=(159,186,195)
def font(size):
    for p in ['/System/Library/Fonts/Supplemental/Arial.ttf','/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf']:
        if Path(p).exists(): return ImageFont.truetype(p,size)
    return ImageFont.load_default(size=size)
canvas=Image.new('RGB',(1536,1080),BG);d=ImageDraw.Draw(canvas)
d.text((32,24),'SENTINEL  /  CORE WARDEN CW-02',font=font(30),fill=TEXT)
d.text((32,65),'Static concept fidelity review • no animation • original concept crop shown at left',font=font(17),fill=MUTED)
def panel(x,y,w,h,title,source,crop=None):
    d.rounded_rectangle((x,y,x+w,y+h),radius=12,outline=BORDER,width=1)
    d.text((x+18,y+14),title,font=font(20),fill=TEXT)
    im=Image.open(source).convert('RGBA')
    if crop: im=im.crop(crop)
    im.thumbnail((w-24,h-64),Image.Resampling.LANCZOS)
    dest=(x+(w-im.width)//2,y+52+(h-64-im.height)//2)
    canvas.paste(im,dest,im)
panel(24,106,480,462,'APPROVED CONCEPT',ROOT/'reference/Core_Warden_Original_Reference.png',(274,25,655,392))
panel(528,106,480,462,'CW-01  /  BASELINE',ROOT/'renders/preview/cw01_hero.png')
panel(1032,106,480,462,'CW-02  /  REFINEMENT',OUT/'hero.png')
for i,(name,title) in enumerate([('front','FRONT'),('three_quarter','THREE-QUARTER'),('side','SIDE'),('back','BACK')]):
    panel(24+i*378,590,354,304,title,OUT/(name+'.png'))
d.text((32,918),'NATIVE PIXEL SIZE TESTS',font=font(18),fill=TEXT)
for n,x in [(96,340),(128,570),(192,840)]:
    im=Image.open(OUT/('size_'+str(n)+'.png')).convert('RGBA')
    # Exactly 1:1 pixels; no upscaling in the readability strip.
    canvas.paste(im,(x,897),im)
    d.text((x+n+12,966),str(n)+' px',font=font(17),fill=MUTED)
d.text((1110,933),'Remaining: crown curvature,',font=font(16),fill=MUTED)
d.text((1110,960),'cheek contour and rear layout',font=font(16),fill=MUTED)
d.text((1110,987),'need visual approval.',font=font(16),fill=MUTED)
canvas.save(OUT/'comparison.png')
# Quantitative evidence of two bright, cool-colored eye regions at native sizes.
results={}
for n in [96,128,192]:
    im=Image.open(OUT/('size_'+str(n)+'.png')).convert('RGBA')
    eye_counts=[]
    for sign in [-1,1]:
        lo,hi=sorted([.18*sign,.61*sign]);count=0
        for y in range(int(n*(.5-(.25+.07)/3.65)),int(n*(.5-(-.14+.07)/3.65))+1):
            for x in range(int(n*(.5+lo/3.65)),int(n*(.5+hi/3.65))+1):
                r,g,b,a=im.getpixel((x,y))
                if a>200 and b>160 and b-r>8:count+=1
        eye_counts.append(count)
    assert min(eye_counts)>=2,(n,eye_counts)
    results[str(n)]={'distinct_eye_regions':2,'bright_glacier_pixels_per_eye':eye_counts,'native_pixel_size':True}
(ROOT/'blender/cw02_readability.json').write_text(json.dumps(results,indent=2))
print('CW02_COMPARISON_OK',json.dumps(results))
