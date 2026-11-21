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
# Root House_transparenting event 6159361858903351 sets picked
# foundation family 1048 animation to exactly "out" when PlayerBase
# does not overlap source object 346. This is a Sprite animation index,
# never a new object ID or PNG filename guess.
bases=next((row for row in p[4] if isinstance(row,list) and row and row[0]==1048),None)
if not bases:raise SystemExit("Missing original House foundation family 1048")
base_rows=[]
for oid in bases[1:]:
    obj=p[3][oid]
    animations=obj[7] if len(obj)>7 and isinstance(obj[7],list) else []
    named={a[0].lower():ai for ai,a in enumerate(animations)
           if isinstance(a,list) and len(a)>7 and isinstance(a[0],str)}
    if "out" in named:
        base_rows.append((oid,named["out"],named.get("in",65535)))
    elif oid in (302,306,311,312,313):
        print("Original house base 1048 object",oid,
              "lacks literal out animation; names",list(named))
# Original source may dispatch a family SetAnimation without a matching
# per-member animation. Preserve the authoritative absence rather than
# inventing an atlas animation or making unrelated game systems unbuildable.
baseout=ROOT/"include/minidayz/generated/source_roof_foundation_animations.hpp"
b=["#pragma once","#include <cstddef>","#include <cstdint>",
"namespace minidayz::world_source {",
"// Exact root Construct family 1048 Sprite.SetAnimation(out) mappings.",
"struct SourceRoofFoundationAnimations {std::uint16_t object,out_animation,in_animation;};",
"inline constexpr SourceRoofFoundationAnimations kSourceRoofFoundationAnimations[]={"]
b+=[" {"+",".join(map(str,row))+"}," for row in base_rows]
if not base_rows:b.append(" {65535,65535,65535}, // no matching root member animation")
b+=["};",
"inline constexpr std::size_t kSourceRoofFoundationAnimationCount="+str(len(base_rows))+";",
"inline const SourceRoofFoundationAnimations* source_roof_foundation_animations(std::uint16_t object) noexcept {",
" for(const auto& row:kSourceRoofFoundationAnimations)if(row.object==object)return &row;",
" return nullptr;","}", "} // namespace minidayz::world_source"]
baseout.write_text("\n".join(b)+"\n",encoding="utf-8")
print("Original family 1048 out-animation source Sprite count",len(base_rows))
print("Original roof family frames:",len(rows),"members total",len(family)-1,"without point",without[:25])
