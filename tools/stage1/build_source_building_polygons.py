#!/usr/bin/env python3
"""Build exact custom collision polygons for original Construct building sprites and player.

All coordinates are normalized to their ORIGINAL frame hotspot. This is a
collision source table, not a texture atlas or an inferred generic rectangle.
Missing custom polygons correctly fall back to Construct's frame bounds.
"""
from pathlib import Path
import math,sys
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tools/stage1"))
from audit_original_world_exhaustive import decode

project=decode((ROOT/"data.deobfuscated.js").read_bytes())
objects=project[3]
solid=set()
for oid,obj in enumerate(objects):
    if len(obj)>8 and isinstance(obj[8],list) and any(
        isinstance(b,list) and b and b[0]=="Solid" for b in obj[8]):
        solid.add(oid)
# Source-authored Map Buildings layers include many non-solid decorations.
# Collect original building Sprite IDs, including non-solid overlap triggers.
# Include original ghost object573 from Building_mode(12) Spawn action.
# It has a custom frame12 polygon; rectangular picking changes house
# placement eligibility and violates original sprite sheet coordinates.
# Original wreck Sprites213/212/487/214/etc SpawnAnotherObject(241) on
# Sunset_Dawn. Sprite241 is NOT a Solid or Map Buildings static instance,
# but it still needs original per-frame dimensions/hotspots: without them
# WorldSystem skips the 0x0 child and every car trunk becomes unsearchable.
building=set((241,302,306,311,312,313,321,322,323,324,333,348,349,573,787,951))
for layout in project[5]:
    if layout[0]!="Map":continue
    for layer in layout[6]:
        if not isinstance(layer,list) or layer[0] not in (
            "Buildings","Buildings_bg","Buildings_base",
            "Buildings_base_top","Buildings_shadows"):continue
        for instance in layer[14] if len(layer)>14 and isinstance(layer[14],list) else []:
            if isinstance(instance,list) and len(instance)>1 and isinstance(instance[1],int):
                building.add(instance[1])
for fam in project[4]:
    if isinstance(fam,list) and fam and fam[0] in (1041,1048,1049):
        building.update(x for x in fam[1:] if isinstance(x,int))
for oid,obj in enumerate(objects):
    anim=obj[7] if len(obj)>7 and isinstance(obj[7],list) else []
    for a in anim[:1]:
        if not isinstance(a,list) or len(a)<8 or not isinstance(a[7],list):continue
        for frame in a[7][:1]:
            if not isinstance(frame,list) or not frame:continue
            path=str(frame[0]).lower().replace("\\","/")
            if path.startswith("images/b_") or "door" in path or "building" in path:
                building.add(oid)
# Include every original Solid object: procedural Spawn actions may place a
# sprite on a Buildings layer even when no static Map instance exists.
# This preserves authored polygons instead of silently using rectangles.
building.update(solid)
rows=[];vertices=[]
def fmt(value):
    if not math.isfinite(value) or abs(value)>4:
        raise ValueError(f"Invalid original normalized collision vertex {value}")
    s=format(float(value),".9g")
    if "." not in s and "e" not in s:s+=".0"
    return s+"f"
for oid in sorted(building | {181,346,349}):
    obj=objects[oid]
    animations=obj[7] if len(obj)>7 and isinstance(obj[7],list) else []
    for ai,a in enumerate(animations):
        if not isinstance(a,list) or len(a)<8 or not isinstance(a[7],list):continue
        for fi,fr in enumerate(a[7]):
            poly=fr[10] if len(fr)>10 else None
            if not isinstance(poly,list) or len(poly)<6:continue
            if len(poly)%2:raise RuntimeError(f"Odd collision polygon at {oid}:{ai}:{fi}")
            if len(poly)>1024:raise RuntimeError(f"Excessive collision polygon at {oid}:{ai}:{fi}")
            offset=len(vertices)//2
            vertices.extend(fmt(v) for v in poly)
            rows.append((oid,ai,fi,offset,len(poly)//2))
if not rows or not vertices:raise SystemExit("Root building custom collision polygons missing")
out=ROOT/"include/minidayz/generated/source_building_collision_polygons.hpp"
out.parent.mkdir(parents=True,exist_ok=True)
lines=[
"#pragma once","#include <cstddef>","#include <cstdint>",
"namespace minidayz::stage1 {",
"// Generated from original root Construct frame[10] collision vertices.",
"// Values are relative to each Sprite frame's hotspot, not PNG atlas pixels.",
"struct SourceBuildingCollisionPolygon {",
" std::uint16_t object,animation;std::uint32_t frame,first;std::uint16_t points;};",
"inline constexpr float kSourceBuildingCollisionXY[]={"]
lines += [" "+v+"," for v in vertices]
lines += ["};","inline constexpr SourceBuildingCollisionPolygon kSourceBuildingCollisionPolygons[]={"]
lines += [" {"+",".join(map(str,r))+"}," for r in rows]
lines += [
"};",
"inline constexpr std::size_t kSourceBuildingCollisionPolygonCount=sizeof(kSourceBuildingCollisionPolygons)/sizeof(kSourceBuildingCollisionPolygons[0]);",
"inline const SourceBuildingCollisionPolygon* source_building_collision_polygon(",
" std::uint16_t object,std::uint16_t animation,std::uint32_t frame) noexcept {",
" std::size_t first=0,last=kSourceBuildingCollisionPolygonCount;",
" while(first<last){const auto mid=first+(last-first)/2;",
"  const auto& row=kSourceBuildingCollisionPolygons[mid];",
"  if(row.object<object || (row.object==object && (row.animation<animation ||",
"     (row.animation==animation && row.frame<frame))))first=mid+1;",
"  else last=mid;}",
" if(first>=kSourceBuildingCollisionPolygonCount)return nullptr;",
" const auto& row=kSourceBuildingCollisionPolygons[first];",
" return row.object==object&&row.animation==animation&&row.frame==frame?&row:nullptr;",
"}","} // namespace minidayz::stage1",
]
out.write_text("\n".join(lines)+"\n",encoding="utf-8")
# Preserve original frame-specific sizes and hotspots for EVERY building
# family member, even sprites without Solid behavior or custom vertices.
# Source SetFrame actions may change the active original image rectangle.
geometry=[]
for oid in sorted(building | {181,346,349}):
    obj=objects[oid]
    animations=obj[7] if len(obj)>7 and isinstance(obj[7],list) else []
    for ai,a in enumerate(animations):
        if not isinstance(a,list) or len(a)<8 or not isinstance(a[7],list):continue
        for fi,fr in enumerate(a[7]):
            if len(fr)<=8:continue
            w,h,ox,oy=fr[4],fr[5],fr[7],fr[8]
            if not all(isinstance(v,(int,float)) and math.isfinite(v)
                       for v in (w,h,ox,oy)):continue
            if not (0<w<16384 and 0<h<16384):continue
            geometry.append((oid,ai,fi,w,h,ox,oy))
geom=ROOT/"include/minidayz/generated/source_building_frame_geometry.hpp"
g=[
 "#pragma once","#include <cstddef>","#include <cstdint>",
 "namespace minidayz::stage1 {",
 "struct SourceBuildingFrameGeometry {std::uint16_t object,animation;",
 " std::uint32_t frame;float width,height,origin_x,origin_y;};",
 "inline constexpr SourceBuildingFrameGeometry kSourceBuildingFrameGeometry[]={"
]
def gf(n):
    result=format(float(n),".9g")
    if "." not in result and "e" not in result and "E" not in result:result+=".0"
    return result+"f"
g+=[" {"+",".join([str(oid),str(ai),str(fi),gf(w),gf(h),gf(ox),gf(oy)])+"},"
    for oid,ai,fi,w,h,ox,oy in geometry]
g+=["};",
"inline constexpr std::size_t kSourceBuildingFrameGeometryCount=sizeof(kSourceBuildingFrameGeometry)/sizeof(kSourceBuildingFrameGeometry[0]);",
"inline const SourceBuildingFrameGeometry* source_building_frame_geometry(",
" std::uint16_t object,std::uint16_t animation,std::uint32_t frame) noexcept {",
" std::size_t first=0,last=kSourceBuildingFrameGeometryCount;",
" while(first<last){const auto mid=first+(last-first)/2;",
"  const auto& row=kSourceBuildingFrameGeometry[mid];",
"  if(row.object<object || (row.object==object && (row.animation<animation ||",
"     (row.animation==animation && row.frame<frame))))first=mid+1;",
"  else last=mid;}",
" if(first>=kSourceBuildingFrameGeometryCount)return nullptr;",
" const auto& row=kSourceBuildingFrameGeometry[first];",
" return row.object==object&&row.animation==animation&&row.frame==frame?&row:nullptr;",
"}","} // namespace minidayz::stage1"]
geom.write_text("\n".join(g)+"\n",encoding="utf-8")
print("Original building frame geometry",len(geometry),"frames")
print(f"Original building custom polygons: {len(rows)} frames, {len(vertices)//2} vertices, {len(building & solid)} source-Solid building kinds (also includes non-solid interaction masks)")
