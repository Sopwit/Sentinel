"""Render deterministic TestBackend text snapshots (not native terminal screenshots)."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
root=Path(__file__).resolve().parent
font=ImageFont.truetype('/System/Library/Fonts/Menlo.ttc',14)
for phase in ['before','after']:
 rows=(root/phase/'terminal-100x30.txt').read_text().splitlines()
 im=Image.new('RGB',(924,624),(21,24,31));d=ImageDraw.Draw(im)
 for y,row in enumerate(rows): d.text((12,12+y*20),row,font=font,fill=(232,235,240))
 im.save(root/phase/'terminal-100x30.png')
