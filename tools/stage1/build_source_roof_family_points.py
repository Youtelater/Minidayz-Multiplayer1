#!/usr/bin/env python3
"""Extract roof member sprite imagepoints from the root Construct family 1049.

Object 1049 itself is an abstract Construct family (no animation frames).
Every real roof animation/atlas rectangle belongs to a member in project[4].
"""
from pathlib import Path
import sys
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tools"/"stage1"))
from audit_original_world_exhaustive import decode
p=decode((ROOT/"data.deobfuscated.js").read_bytes())
family=next((row for row in p[4] if isinstance(row,list) and row and row[0]==1049),None)
if family is None:raise SystemExit("Source roof family 1049 missing")
def f(v):
    s=format(float(v),".9g")
    if "." not in s and "e" not in s and "E" not in s:s+=".0"
    return s+"f"
rows=[]
without=[]
for oid in family[1:]:
    obj=p[3][oid]
    animations=obj[7] if len(obj)>7 and isinstance(obj[7],list) else []
    member_count=0
    for ai,anim in enumerate(animations):
        frames=anim[7] if len(anim)>7 and isinstance(anim[7],list) else []
        for fi,frame in enumerate(frames):
            if not isinstance(frame,list) or len(frame)<=9:continue
            points=frame[9] if isinstance(frame[9],list) else []
            pt=next((point for point in points
                if isinstance(point,list) and point and point[0]=="Imagepoint 1"),None)
            if pt is None:continue
            if not isinstance(frame[4],(int,float)) or not isinstance(frame[5],(int,float)) or frame[4]<=0 or frame[5]<=0:continue
            rows.append((oid,ai,fi,frame[4],frame[5],frame[7],frame[8],pt[1],pt[2]))
            member_count+=1
    if member_count==0:without.append(oid)
if not rows:
    raise SystemExit("Source roof family members have no Imagepoint 1 frames; members="+str(family[1:]))
out=ROOT/"include/minidayz/generated/source_roof_member_points.hpp"
out.parent.mkdir(parents=True,exist_ok=True)
lines=["#pragma once","#include <cstdint>","#include <cstddef>",
"namespace minidayz::world_source {",
"// Each entry belongs to a real Sprite member of the original family 1049.",
"struct SourceRoofMemberPoint {",
" std::uint16_t object,animation; std::uint32_t frame;",
" float width,height,origin_x,origin_y,point_x,point_y; };",
"inline constexpr SourceRoofMemberPoint kSourceRoofMemberPoints[]={"]
for row in rows:
    lines.append(" {"+",".join([str(row[0]),str(row[1]),str(row[2])]
        +[f(x) for x in row[3:]])+"},")
lines += ["};",
"inline constexpr std::size_t kSourceRoofMemberPointCount=sizeof(kSourceRoofMemberPoints)/sizeof(kSourceRoofMemberPoints[0]);",
"inline const SourceRoofMemberPoint* source_roof_member_point(std::uint16_t object,std::uint16_t animation,std::uint32_t frame) noexcept {",
" for(const auto& point:kSourceRoofMemberPoints)if(point.object==object && point.animation==animation && point.frame==frame)return &point;",
" return nullptr;","}","} // namespace minidayz::world_source"]
out.write_text("\n".join(lines)+"\n",encoding="utf-8")
print("Original roof family frames:",len(rows),"members total",len(family)-1,"without point",without[:25])
