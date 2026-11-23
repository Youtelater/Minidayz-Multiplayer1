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

# Root Game_events > create_map_element, Sprite210 tree_block variants:
# variants 7..10 each spawn their own Sprite348 on zed_resp at Imagepoint1.
# Derive BOTH animation frames and collision geometry from original events.
BLOCK_BRANCHES=((7,6033785432522481),(8,7238212881044891),
                (9,9881460187187066),(10,1618691315170714))
def walk_events(events):
 for e in events if isinstance(events,list) else []:
  if isinstance(e,list) and len(e)>=7 and e[0]==0:
   yield e
   yield from walk_events(e[7] if len(e)>7 else [])
by_uid={e[4]:e for sheet,events in p[6] for e in walk_events(events)}
block_rows=[]
for variant,uid in BLOCK_BRANCHES:
 e=by_uid.get(uid)
 if not e or not any(c[:2]==[508,71] and len(c)>9 and
     c[9][-1]==[7,[0,variant]] for c in e[5]):
  raise SystemExit(f"Original tree_block branch/condition changed {variant}:{uid}")
 actions=e[6]
 if not any(a[:2]==[508,73] and a[5][0]==[4,210] for a in actions):
  raise SystemExit(f"Original Sprite210 spawn missing in branch {uid}")
 if not any(a[:2]==[210,73] and a[5][0]==[4,348]
      and a[5][2]==[7,[0,1]] for a in actions):
  raise SystemExit(f"Original Sprite348 Imagepoint1 spawn missing in {uid}")
 tree_set=[a for a in actions if a[:2]==[210,72]]
 solid_set=[a for a in actions if a[:2]==[348,72]]
 if len(tree_set)>1 or len(solid_set)!=1:
  raise SystemExit(f"Original tree_block frame actions changed {uid}")
 tree_frame=tree_set[0][5][0][1][1] if tree_set else 0
 solid_frame=solid_set[0][5][0][1][1]
 if tree_frame!=variant-7 or solid_frame!=(17 if variant==7 else variant+13):
  raise SystemExit(f"Unexpected original tree_block frames {variant}:{tree_frame}/{solid_frame}")
 tree=p[3][210][7][0][7][tree_frame]
 solid=p[3][348][7][0][7][solid_frame]
 point=next((v for v in tree[9] if v[0]=="Imagepoint 1"),None)
 if not point or any(x[4]<=0 or x[5]<=0 for x in (tree,solid)):
  raise SystemExit(f"Missing source Sprite210/348 frame or imagepoint {variant}")
 block_rows.append((variant,tree_frame,solid_frame,tree,solid,point))
block_lines=[
 "// Root tree_block Sprite210 variants 7..10 with linked Sprite348 solids.",
 "struct SourceTreeBlockVariant {",
 " std::uint8_t variant;std::uint32_t tree_frame,solid_frame;",
 " float width,height,origin_x,origin_y,point_x,point_y;",
 " float solid_width,solid_height,solid_origin_x,solid_origin_y;};",
 "inline constexpr SourceTreeBlockVariant kSourceTreeBlockVariants[]={"]
for variant,tf,sf,tree,solid,point in block_rows:
 values=[variant,tf,sf,tree[4],tree[5],tree[7],tree[8],
         point[1],point[2],solid[4],solid[5],solid[7],solid[8]]
 block_lines.append(" {"+",".join(str(v) if j<3 else f(v)
                                    for j,v in enumerate(values))+"},")
block_lines.extend([
 "};",
 "inline const SourceTreeBlockVariant* source_tree_block_variant(",
 " std::uint8_t variant) noexcept {",
 " for(const auto& v:kSourceTreeBlockVariants)if(v.variant==variant)return &v;",
 " return nullptr;}",
 "inline const SourceTreeBlockVariant* source_tree_block_frame(",
 " std::uint32_t frame) noexcept {",
 " for(const auto& v:kSourceTreeBlockVariants)if(v.tree_frame==frame)return &v;",
 " return nullptr;}",
])
lines[-1:-1]=block_lines
print("Original source tree_block frame maps",[(v,tf,sf) for v,tf,sf,*_ in block_rows])

out.write_text("\n".join(lines)+"\n",encoding="utf-8")
print("Original tree hitbox imagepoints:",len(rows),"hitbox-frame10",ow,oh)
