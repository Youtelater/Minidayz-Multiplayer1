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
