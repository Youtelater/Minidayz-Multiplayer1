#!/usr/bin/env python3
"""Extract exact Construct door 'in' image points for native automatic doors.

The origin and in-point vary per animation/frame inside door-sheet0.png.
Never use the whole PNG or a hardcoded threshold/point for all doors.
"""
from pathlib import Path
import sys
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tools"/"stage1"))
from audit_original_world_exhaustive import decode
p=decode((ROOT/"data.deobfuscated.js").read_bytes())
animations=p[3][349][7]
rows=[]
def f(v):
    s=format(float(v),".9g")
    if "." not in s and "e" not in s and "E" not in s:s+=".0"
    return s+"f"
for ai,anim in enumerate(animations):
    name=anim[0]
    metal=name in ("firestation","ga_brown","ga_blue")
    for fi,fr in enumerate(anim[7]):
        points=fr[9] if len(fr)>9 and isinstance(fr[9],list) else []
        inside=next((point for point in points if point[0]=="in"),None)
        anchor=next((point for point in points if point[0]=="Imagepoint 1"),None)
        if inside is None or anchor is None: continue
        width,height=fr[4],fr[5]
        if width<=0 or height<=0:continue
        rows.append((ai,fi,width,height,fr[7],fr[8],inside[1],inside[2],
                     anchor[1],anchor[2],metal,name))
if len(rows)<20 or not any(row[10] for row in rows):
    raise SystemExit("Original door point extraction failed: too few frames or no metal animations")
# Original Doors_auto close UID 243854400993751:
# object 349 spawns object 348 at its authored Imagepoint 1, then
# object 348 SetFrame(15) and SetAngle(Door.Angle+6). Object 348 is
# the original Solid sprite with frame-specific collision vertices.
solid=p[3][348][7][0][7][15]
if len(solid)<=10 or solid[4]<=0 or solid[5]<=0:
    raise SystemExit("Original door Solid 348 frame 15 invalid")
poly=solid[10]
if not isinstance(poly,list) or len(poly)<6 or len(poly)%2:
    raise SystemExit("Original door Solid frame 15 polygon missing/odd")
for coordinate in poly:
    if not isinstance(coordinate,(int,float)) or not (-4.0<coordinate<4.0):
        raise SystemExit("Invalid original Solid polygon point")

out=ROOT/"include/minidayz/generated/source_door_points.hpp"
out.parent.mkdir(parents=True,exist_ok=True)
lines=["#pragma once","#include <cstdint>","#include <cstddef>",
"namespace minidayz::world_source {",
"// Generated from root data.deobfuscated.js object 349 sprite frame imagepoint 'in'.",
"// Never guess this point: it changes with door animation and open/closed frame.",
"struct SourceDoorPoint {",
" std::uint16_t animation; std::uint32_t frame;",
" float width,height,origin_x,origin_y,in_x,in_y;",
" float solid_anchor_x{},solid_anchor_y{};",
" bool metal; };",
"inline constexpr SourceDoorPoint kSourceDoorPoints[]={"]
for ai,fi,w,h,ox,oy,ix,iy,ax,ay,metal,name in rows:
    lines.append(" {"+",".join([str(ai),str(fi),f(w),f(h),f(ox),f(oy),f(ix),f(iy),f(ax),f(ay),"true" if metal else "false"])+"}, // "+name)
lines += ["};",
"inline constexpr std::size_t kSourceDoorPointCount=sizeof(kSourceDoorPoints)/sizeof(kSourceDoorPoints[0]);",
"inline const SourceDoorPoint* source_door_point(std::uint16_t animation,std::uint32_t frame) noexcept {",
" for(const auto& item:kSourceDoorPoints)if(item.animation==animation&&item.frame==frame)return &item;",
" return nullptr;",
"}","} // namespace minidayz::world_source"]
# Normalized polygon vertices are relative to frame-15 hotspot.
# They are not the PNG atlas crop coordinates. Both are extracted from
# the original root Construct frame definition.
lines += [
"namespace door_solid348 {",
"inline constexpr std::uint16_t kObject=348;",
"inline constexpr std::uint32_t kClosedFrame=15;",
"inline constexpr float kWidth="+f(solid[4])+";",
"inline constexpr float kHeight="+f(solid[5])+";",
"inline constexpr float kOriginX="+f(solid[7])+";",
"inline constexpr float kOriginY="+f(solid[8])+";",
"inline constexpr float kDoorAngleOffsetDegrees=6.0f;",
"inline constexpr float kPolygonXY[]={",
]
lines += [" "+f(poly[i])+","+f(poly[i+1])+"," for i in range(0,len(poly),2)]
lines += [
"};",
"inline constexpr std::size_t kPolygonPointCount=sizeof(kPolygonXY)/sizeof(float)/2;",
"} // namespace door_solid348",
]
out.write_text("\n".join(lines)+"\n",encoding="utf-8")
print("Original door image points generated:",len(rows),"frames",len(animations),"animations")
