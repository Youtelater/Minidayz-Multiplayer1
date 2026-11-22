#!/usr/bin/env python3
"""Forensics for magenta world rectangle visible in Azahar screenshots.

Uses root Construct sprite definitions and original assets/gfx, never duplicates.
Outputs original source object/frame identifiers for large purple textures and
Map instance/source layers that could obscure the player and world.
"""
from pathlib import Path
from collections import defaultdict
import sys,json,math
from PIL import Image
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tools"/"stage1"))
from audit_original_world_exhaustive import decode
p=decode((ROOT/"data.deobfuscated.js").read_bytes())
obj=p[3]
candidates=[]
for oi,o in enumerate(obj):
 if not isinstance(o,list) or len(o)<=7 or not isinstance(o[7],list):continue
 for ai,a in enumerate(o[7]):
  if not isinstance(a,list) or len(a)<=7 or not isinstance(a[7],list):continue
  for fi,fr in enumerate(a[7]):
   if not isinstance(fr,list) or len(fr)<=5 or not isinstance(fr[0],str):continue
   x,y,w,h=map(int,fr[2:6])
   if min(w,h)<2:continue
   path=ROOT/"assets/gfx"/Path(fr[0].replace("\\","/")).name
   if not path.is_file():continue
   try:
    with Image.open(path) as img:
     crop=img.convert("RGBA").crop((x,y,x+w,y+h))
     crop.thumbnail((80,80),Image.Resampling.NEAREST)
     pixel=list(crop.getdata())
   except Exception:continue
   if w>=100 and h>=100:
    zero=[q for q in pixel if q[3]<4]
    magenta_hidden=sum(1 for q in zero if q[0]>q[1]*1.35+20 and q[2]>q[1]*1.35+20 and q[0]>100 and q[2]>100)
    if zero and magenta_hidden/max(1,len(pixel))>0.10:
     print("HIDDEN_PURPLE_SPRITE",json.dumps({"object":oi,"animation":ai,"frame":fi,"w":w,"h":h,"sheet":path.name,"hidden_magenta_fraction":round(magenta_hidden/len(pixel),3)}))
   opaque=[t for t in pixel if t[3]>32]
   if not opaque:continue
   purples=[q for q in opaque if q[0]>q[1]*1.35+20 and q[2]>q[1]*1.35+20 and q[0]>100 and q[2]>100]
   frac=len(purples)/len(opaque)
   semi=sum(1 for q in purples if q[3]<245)/len(opaque)
   if frac>=0.08:
    candidates.append((frac,semi,oi,ai,fi,w,h,path.name,str(a[0]),o[0]))
for c in sorted(candidates,reverse=True)[:90]:
 print("PURPLE_SOURCE_SPRITE",json.dumps(c,ensure_ascii=False))
print("PURPLE_SOURCE_TOTAL",len(candidates))
for idx,layout in enumerate(p[5]):
 if layout[0]!="Map":continue
 print("MAP_INDEX",idx)
 for li,layer in enumerate(layout[6]):
  if not isinstance(layer,list) or len(layer)<=14:continue
  high=[]
  for inst in layer[14] if isinstance(layer[14],list) else ():
   if not isinstance(inst,list) or len(inst)<3 or not isinstance(inst[0],list):continue
   t=inst[0]
   if len(t)<8:continue
   x,y,z,w,h,angle,opacity=t[:7]
   if abs(w)>=250 and abs(h)>=200 and opacity>0:
    high.append((inst[1],inst[2],round(x),round(y),round(w),round(h),round(opacity,2),round(angle,2)))
  if high:
   print("LARGE_MAP_SPRITES",json.dumps({"layer":li,"name":layer[0],"visible":layer[3] if len(layer)>3 else None,"instances":high[:50]},ensure_ascii=False))

# Original Construct Tilemap 208/211/418/230 frame colors are not in obj[3]
# Sprite animations; sample their canonical source PNG sheets separately.
for tilepath in sorted((ROOT/"assets/gfx").glob("*tilemap.png")):
 with Image.open(tilepath) as png:
  rgba=png.convert("RGBA")
  orig_size=rgba.size
  rgba.thumbnail((128,128),Image.Resampling.NEAREST)
  pix=list(rgba.getdata())
  coverage=sum(1 for q in pix if q[3]>32)
  purple=sum(1 for q in pix if q[3]>32 and q[0]>q[1]*1.35+20 and q[2]>q[1]*1.35+20 and q[0]>100 and q[2]>100)
  semi=sum(1 for q in pix if 0<q[3]<245)
  magenta_zero=sum(1 for q in pix if q[3]<2 and q[0]>q[1]*1.35+20 and q[2]>q[1]*1.35+20 and q[0]>100 and q[2]>100)
  print("SOURCE_TILEMAP_COLOR",json.dumps({"png":tilepath.name,"dims":orig_size,
      "purple_fraction":round(purple/max(1,coverage),4),
      "translucent_fraction":round(semi/len(pix),4),
      "magenta_transparent_fraction":round(magenta_zero/len(pix),4)}))
