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
    return format(float(v),".9g")+"f"
for ai,anim in enumerate(animations):
    name=anim[0]
    metal=name in ("firestation","ga_brown","ga_blue")
    for fi,fr in enumerate(anim[7]):
        points=fr[9] if len(fr)>9 and isinstance(fr[9],list) else []
        inside=next((point for point in points if point[0]=="in"),None)
        if inside is None: continue
        width,height=fr[4],fr[5]
        if width<=0 or height<=0:continue
        rows.append((ai,fi,width,height,fr[7],fr[8],inside[1],inside[2],metal,name))
if len(rows)<20 or not any(row[8] for row in rows):
    raise SystemExit("Original door point extraction failed: too few frames or no metal animations")
out=ROOT/"include/minidayz/generated/source_door_points.hpp"
out.parent.mkdir(parents=True,exist_ok=True)
lines=["#pragma once","#include <cstdint>","#include <cstddef>",
"namespace minidayz::world_source {",
"// Generated from root data.deobfuscated.js object 349 sprite frame imagepoint 'in'.",
"// Never guess this point: it changes with door animation and open/closed frame.",
"struct SourceDoorPoint {",
" std::uint16_t animation; std::uint32_t frame;",
" float width,height,origin_x,origin_y,in_x,in_y;",
" bool metal; };",
"inline constexpr SourceDoorPoint kSourceDoorPoints[]={"]
for ai,fi,w,h,ox,oy,ix,iy,metal,name in rows:
    lines.append(" {"+",".join([str(ai),str(fi),f(w),f(h),f(ox),f(oy),f(ix),f(iy),"true" if metal else "false"])+"}, // "+name)
lines += ["};",
"inline constexpr std::size_t kSourceDoorPointCount=sizeof(kSourceDoorPoints)/sizeof(kSourceDoorPoints[0]);",
"inline const SourceDoorPoint* source_door_point(std::uint16_t animation,std::uint32_t frame) noexcept {",
" for(const auto& item:kSourceDoorPoints)if(item.animation==animation&&item.frame==frame)return &item;",
" return nullptr;",
"}","} // namespace minidayz::world_source"]
out.write_text("\n".join(lines)+"\n",encoding="utf-8")
print("Original door image points generated:",len(rows),"frames",len(animations),"animations")
