#!/usr/bin/env python3
"""Diagnostic only: determine which ORIGINAL MiniDAYZ ground tile can repeat
without obvious 60px square seams. Never author or synthesize world pixels.
All candidate pixels are read from canonical assets/gfx/ground_lvl*_tilemap.png.
"""
from pathlib import Path
from PIL import Image
import statistics

ROOT=Path(__file__).resolve().parents[2]
for level in range(1,8):
    filename=f"ground_lvl{level}_tilemap.png"
    art=Image.open(ROOT/"assets/gfx"/filename).convert("RGBA")
    assert art.width%60==0 and art.height%60==0
    columns=art.width//60
    scores=[]
    for index in range(columns*(art.height//60)):
        x=(index%columns)*60;y=(index//columns)*60
        tile=art.crop((x,y,x+60,y+60))
        pixels=tile.load()
        def mismatch(p,q):
            return sum(abs(p[k]-q[k]) for k in range(4))
        horizontal=sum(mismatch(pixels[0,j],pixels[59,j])
                       for j in range(60))/(60*4)
        vertical=sum(mismatch(pixels[i,0],pixels[i,59])
                     for i in range(60))/(60*4)
        transparency=sum(255-pixels[i,j][3] for j in range(60)
                         for i in range(60))/(60*60)
        center=pixels[30,30]
        rgb=tuple(sum(pixels[i,j][k] for j in range(60)
                      for i in range(60))//3600 for k in range(3))
        scores.append((index,round(horizontal,2),round(vertical,2),
                       round(transparency,2),rgb,center))
    fully=[v for v in scores if v[3]<0.5]
    print("GROUND LEVEL",level,"OPAQUE REPEAT CANDIDATES",
          sorted(fully,key=lambda t:t[1]+t[2])[:12])
    print("GROUND LEVEL",level,"CURRENT BACKDROP ID4",scores[4])
    print("GROUND LEVEL",level,"ALL TILES",scores)

# Exact root gameplay event trace: the Array road codes are not a visual
# road texture. Identify the Construct Tilemap SetTile actions that actually
# paint physical scenery before changing any native world render rule.
import sys
from collections import Counter,defaultdict
sys.path.insert(0,str(ROOT/"tools/stage1"))
from audit_original_world_exhaustive import decode
project=decode((ROOT/"data.deobfuscated.js").read_bytes())
terrain_objects={482,687,688,776,980,1138,1139,208,211,418,230}
count=Counter();samples=defaultdict(list)
def walk(nodes,parents=()):
    for e in nodes if isinstance(nodes,list) else ():
        if not isinstance(e,list) or len(e)<7 or e[0]!=0:
            continue
        for a in e[6] if isinstance(e[6],list) else []:
            if not isinstance(a,list) or len(a)<6 or a[0] not in terrain_objects:
                continue
            key=(a[0],a[1]);count[key]+=1
            if len(samples[key])<6:
                samples[key].append((e[4],parents[-5:],str(a[5])[:600],
                                     str(e[5])[:600]))
        if len(e)>7:walk(e[7],parents+(e[4],))
for sheet,nodes in project[6]:
    walk(nodes)
print("ROOT ORIGINAL TERRAIN TILEMAP ACTION COUNTS",sorted(count.items()))
for key in sorted(count):
    if key[1] in (192,193,194,195,199,210,177):
        print("ROOT TERRAIN ACTION",key,"SAMPLES",samples[key])

# Inspect the original Map's giant bg Tilemap instances that the renderer
# does NOT currently include in the four named environment tilemaps. Only
# the root Construct project can tell whether they are authored scenery.
for oid in (208,211,230,418,482,687,688,776,784,980,1138,1139,1375,350,974,976):
    obj=project[3][oid]
    print("ROOT MAP OBJECT",oid,"type",obj[1],"art",repr(obj[6])[:500])
layout=next(x for x in project[5] if x[0]=="Map")
for index,layer in enumerate(layout[6]):
    if layer[0] not in ("bg","asphalt","Water","ground_enviroment","Forest","Forests"):
        continue
    for inst in layer[14] if len(layer)>14 and isinstance(layer[14],list) else []:
        if not isinstance(inst,list) or len(inst)<6 or inst[1] not in (
             208,211,230,418,482,687,688,776,784,980,1138,1139,1375,974,976):
            continue
        t,attrs=inst[0],inst[5]
        summary={"object":inst[1],"layer":layer[0],"x":t[0],
                 "y":t[1],"w":t[2],"h":t[3],"attrs":attrs[:5]}
        if len(t)>13 and isinstance(t[13],list):
            summary["tilemap_header"]=t[13][:2]
            import re
            tiles={}
            for match in re.finditer(r"(\\d+)x(-?\\d+)|(-?\\d+)",str(t[13][2])):
                n=int(match[1] or 1);v=int(match[2] if match[2] is not None else match[3])
                tiles[v]=tiles.get(v,0)+n
            summary["rle_top"]=sorted(tiles.items(),key=lambda a:-a[1])[:8]
        print("ROOT MAP ORIGINAL BG INSTANCE",summary)
