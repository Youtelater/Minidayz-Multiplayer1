#!/usr/bin/env python3
"""Extract exact tree Imagepoint 1 from the original Construct sprite frames.

Each source tree creates obst_base object 345 at that imagepoint. Construct
sets the base to frame 10 and links its UID to the tree's instance variable.
"""
from pathlib import Path
import sys
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tools"/"stage1"))
from audit_original_world_exhaustive import decode
p=decode((ROOT/"data.deobfuscated.js").read_bytes())
TREE_IDS=(184,207,209,576,577,581)
def f(v):
    s=format(float(v),".9g")
    if "." not in s and "e" not in s and "E" not in s:s+=".0"
    return s+"f"
rows=[]
for oid in TREE_IDS:
    frame=p[3][oid][7][0][7][0]
    points=frame[9]
    hit=next((point for point in points if point[0]=="Imagepoint 1"),None)
    if not hit or frame[4]<=0 or frame[5]<=0:
        raise SystemExit(f"Original tree {oid} missing frame-0 imagepoint 1")
    rows.append((oid,frame[4],frame[5],frame[7],frame[8],hit[1],hit[2]))
# Original obst_base 345 frame 10 was assigned by all six creation branches.
obstacle=p[3][345][7][0][7][10]
ow,oh=obstacle[4],obstacle[5]
if ow<=0 or oh<=0:raise SystemExit("Source obst_base frame 10 missing")
out=ROOT/"include/minidayz/generated/source_tree_impact_points.hpp"
out.parent.mkdir(parents=True,exist_ok=True)
lines=["#pragma once","#include <cstdint>","#include <cstddef>",
"namespace minidayz::world_source {",
"// Generated from root Game_events/create_map_element tree branches.",
"// Sprite 345 is created at the tree's Imagepoint 1, then set to frame 10.",
"struct SourceTreeImpactPoint {",
" std::uint16_t object; float width,height,origin_x,origin_y,point_x,point_y; };",
"inline constexpr SourceTreeImpactPoint kSourceTreeImpactPoints[]={"]
for row in rows:
    lines.append(" {"+",".join([str(row[0])]+[f(v) for v in row[1:]])+"},")
lines.extend(["};",
"inline constexpr std::size_t kSourceTreeImpactPointCount=sizeof(kSourceTreeImpactPoints)/sizeof(kSourceTreeImpactPoints[0]);",
"inline const SourceTreeImpactPoint* source_tree_impact_point(std::uint16_t obj) noexcept {",
" for(const auto& point:kSourceTreeImpactPoints)if(point.object==obj)return &point;",
" return nullptr;","}",
"inline constexpr std::uint16_t kOriginalTreeHitboxObject=345;",
"inline constexpr std::uint32_t kOriginalTreeStandingHitboxFrame=10;",
"inline constexpr std::uint32_t kOriginalTreeFallenHitboxFrame=8;",
"inline constexpr float kOriginalTreeHitboxWidth="+f(ow)+";",
"inline constexpr float kOriginalTreeHitboxHeight="+f(oh)+";",
"inline constexpr float kOriginalTreeHitboxOriginX="+f(obstacle[7])+";",
"inline constexpr float kOriginalTreeHitboxOriginY="+f(obstacle[8])+";",
"} // namespace minidayz::world_source"])
out.write_text("\n".join(lines)+"\n",encoding="utf-8")
print("Original tree hitbox imagepoints:",len(rows),"hitbox-frame10",ow,oh)
