# Bounded VT replay for Sentinel captures; image is a replay, not a native screenshot.
import re,sys,unicodedata
from PIL import Image,ImageDraw,ImageFont
path,w,h=sys.argv[1],int(sys.argv[2]),int(sys.argv[3]);s=open(path,'rb').read().decode('utf8','replace');grid=[[(' ',(225,228,232),(18,21,28)) for _ in range(w)] for _ in range(h)];x=y=0;fg=(225,228,232);bg=(18,21,28)
colors=[(0,0,0),(180,65,65),(70,170,110),(180,160,70),(80,120,190),(165,90,160),(100,170,190),(210,210,210)]
for token in re.findall(r'\x1b\[[0-?]*[ -/]*[@-~]|\x1b\][^\x07]*(?:\x07)|\x1b.|[^\x1b]',s):
 if token.startswith('\x1b['):
  raw=token[2:-1];kind=token[-1];p=[int(v) if v.isdigit() else 0 for v in raw.lstrip('?').split(';')];n=p[0] or 1
  if kind in 'Hf':y=max(0,min(h-1,n-1));x=max(0,min(w-1,(p[1] if len(p)>1 and p[1] else 1)-1))
  elif kind=='G':x=max(0,min(w-1,n-1))
  elif kind=='d':y=max(0,min(h-1,n-1))
  elif kind=='A':y=max(0,y-n)
  elif kind=='B':y=min(h-1,y+n)
  elif kind=='C':x=min(w-1,x+n)
  elif kind=='D':x=max(0,x-n)
  elif kind=='J' and p[0] in [2,3]:grid=[[(' ',fg,bg) for _ in range(w)] for _ in range(h)]
  elif kind=='K':
   for c in range(0 if p[0] in [1,2] else x,w if p[0] in [0,2] else x+1):grid[y][c]=(' ',fg,bg)
  elif kind=='m':
   i=0
   while i<len(p):
    v=p[i]
    if v==0:fg=(225,228,232);bg=(18,21,28)
    elif v==39:fg=(225,228,232)
    elif v==49:bg=(18,21,28)
    elif 30<=v<=37:fg=colors[v-30]
    elif 40<=v<=47:bg=colors[v-40]
    elif v in [38,48] and i+4<len(p) and p[i+1]==2:
     color=tuple(p[i+2:i+5]);i+=4
     if v==38:fg=color
     else:bg=color
    elif v in [38,48] and i+2<len(p) and p[i+1]==5:
     v2=p[i+2];i+=2
     color=colors[v2%8] if v2<16 else ((8+(v2-232)*10,)*3 if v2>=232 else tuple([0,95,135,175,215,255][j] for j in [(v2-16)//36,((v2-16)//6)%6,(v2-16)%6]))
     if v==38:fg=color
     else:bg=color
    i+=1
 elif token.startswith('\x1b'):pass
 elif token=='\r':x=0
 elif token=='\n':y=min(h-1,y+1)
 elif token=='\b':x=max(0,x-1)
 elif token>=' ':
  if x>=w:x=0;y=min(h-1,y+1)
  grid[y][x]=(token,fg,bg);x+=2 if unicodedata.east_asian_width(token) in ['W','F'] else 1
font=ImageFont.truetype('/System/Library/Fonts/Menlo.ttc',14);cw,ch=9,20;im=Image.new('RGB',(w*cw+24,h*ch+24),(18,21,28));d=ImageDraw.Draw(im)
for r,row in enumerate(grid):
 for c,(char,fg,bg) in enumerate(row):d.rectangle((12+c*cw,12+r*ch,12+(c+1)*cw,12+(r+1)*ch),fill=bg);d.text((12+c*cw,12+r*ch),char,font=font,fill=fg)
im.save(path+'.png');open(path+'.txt','w').write('\n'.join(''.join(c[0] for c in row) for row in grid))
