#!/usr/bin/env python3
"""Extract original roof object 1049 Imagepoint 1 for house transparency.
Source event: House_transparenting, player collision Base (181) compared
against Roof.ImagePointY(1), opacity 25 or 100 (Construct percent).
"""
from pathlib import Path
import sys
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tools"/"stage1"))
from audit_original_world_exhaustive import decode
p=decode((ROOT/"data.deobfuscated.js").read_bytes())
anims=p[3][1049][7]
def f(v):
    s=format(float(v),".9g")
    if "." not in s and "e" not in s and "E" not in s:s+=".0"
    return s+"f"
rows=[]
for ai,animation in enumerate(anims):
    frames=animation[7] if len(animation)>7 and isinstance(animation[7],list) else []
    for fi,frame in enumerate(frames):
        if not isinstance(frame,list):continue
        pts=frame[9] if len(frame)>9 and isinstance(frame[9],list) else []
        point=next((item for item in pts if item[0]=="Imagepoint 1"),None)
        if not point:continue
        rows.append((ai,fi,frame[4],frame[5],frame[7],frame[8],point[1],point[2]))
if not rows:raise SystemExit("No source roof imagepoints found")
out=ROOT/"include/minidayz/generated/source_roof_points.hpp"
out.parent.mkdir(parents=True,exist_ok=True)
lines=["#pragma once","#include <cstdint>","#include <cstddef>",
"namespace minidayz::world_source {",
"// Original Construct object 1049 roof frames + Imagepoint 1, source-root only.",
"struct SourceRoofPoint { std::uint16_t animation; std::uint32_t frame;",
" float width,height,origin_x,origin_y,point_x,point_y; };",
"inline constexpr SourceRoofPoint kSourceRoofPoints[]={"]
for row in rows:
    lines.append(" {"+",".join([str(row[0]),str(row[1])]+[f(v) for v in row[2:]])+"},")
lines += ["};","inline constexpr std::size_t kSourceRoofPointCount=sizeof(kSourceRoofPoints)/sizeof(kSourceRoofPoints[0]);",
"inline const SourceRoofPoint* source_roof_point(std::uint16_t animation,std::uint32_t frame) noexcept {",
" for(const auto& p:kSourceRoofPoints)if(p.animation==animation && p.frame==frame)return &p;",
" return nullptr;","}","}"]
out.write_text("\n".join(lines)+"\n",encoding="utf-8")
print("Root original 1049 roof imagepoint records:",len(rows))
