"""Render TestBackend cell snapshots and canonical source frames for layout review."""
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
import json
root=Path(__file__).resolve().parents[4]
font=ImageFont.truetype('/System/Library/Fonts/Menlo.ttc',12)
folder=root/'docs/reviews/core-warden-cw04/layouts'
for path in folder.glob('*.json'):
 data=json.loads(path.read_text());im=Image.new('RGBA',(data['width']*8,data['height']*16),(24,28,34,255));draw=ImageDraw.Draw(im)
 for y,row in enumerate(data['rows']):
  for x,char in enumerate(row):
   if char.strip():draw.text((x*8,y*16),char,font=font,fill=(232,237,240,255))
 if data['image']:
  info=data['image'];sprite=Image.open(root/'assets/core-warden'/info['frame']).convert('RGBA');sprite.thumbnail((info['width'],info['height']),Image.Resampling.LANCZOS);im.alpha_composite(sprite,(info['x']*8+(info['width']-sprite.width)//2,info['y']*16+(info['height']-sprite.height)//2))
 im.convert('RGB').save(path.with_suffix('.png'),optimize=True)
