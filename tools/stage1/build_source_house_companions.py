#!/usr/bin/env python3
"""Emit literal original Construct Spawn-another-object house companions.

Source: root data.deobfuscated.js, Game_events create_map_element children.
Only source action 73 with literal object, layer and imagepoint is accepted.
This does not guess which roof, door, trigger or shadow belongs to a house.
"""
from pathlib import Path
import json,sys
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tools/stage1"))
from audit_original_world_exhaustive import decode
p=decode((ROOT/"data.deobfuscated.js").read_bytes())
objects=p[3]
source_func=7736925111997576
def walk(nodes,ancestry=()):
 for e in nodes:
  if not isinstance(e,list) or len(e)<8 or e[0]!=0:continue
  yield e,ancestry
  yield from walk(e[7],ancestry+(e[4],))
def literal(expr):
 if not isinstance(expr,list) or len(expr)<2:return None
 if expr[0] in (0,2,4,5,7) and not isinstance(expr[1],list):return expr[1]
 return None
def spawn(a):
 if not isinstance(a,list) or len(a)<6 or a[1]!=73:return None
 ps=a[5]
 if not isinstance(ps,list) or len(ps)<3:return None
 target=literal(ps[0]);layer=literal(ps[1][1]);point=literal(ps[2][1])
 if not isinstance(target,int) or not 0<=target<len(objects):return None
 if not isinstance(layer,str) or not isinstance(point,(int,str)):return None
 return a[0],target,layer,point
rules=[];ignored=0;events=0
for name,nodes in p[6]:
 if name!="Game_events":continue
 for e,anc in walk(nodes):
  if source_func not in anc:continue
  acts=e[6] if isinstance(e[6],list) else []
  roots=[spawn(a) for a in acts if isinstance(a,list) and len(a)>1 and a[0]==508 and a[1]==73]
  roots=[x for x in roots if x]
  if not roots:continue
  # The separate tree branch already creates object 345 hitboxes. Only
  # building-base original Spawn parents belong to this building pass.
  if roots[0][2] not in ("Buildings_base","Buildings"):continue
  base=roots[0][1]
  created=[]
  for a in acts:
   if not isinstance(a,list) or len(a)<2:continue
   sp=spawn(a)
   if sp and sp[0]!=508:
    # A companion belongs to the exact original source event and the
    # specific base Sprite. Repeated event UIDs can serve multiple bases.
    created.append({"parent":sp[0],"child":sp[1],"layer":sp[2],
                    "point":sp[3],"frame":0,"animation":0})
   elif a[1]==73 and a[0]!=508:ignored+=1
   elif a[1]==160 and a[0]!=508 and len(a)>5 and a[5]:
    # Original Sprite SetAnimation("city3",from-beginning=1)
    # must select the exact matching animation in the original atlas,
    # especially for visible house doors and metal/wood sound families.
    value=literal(a[5][0][1])
    if isinstance(value,str):
     anims=objects[a[0]][7] if 0<=a[0]<len(objects) and len(objects[a[0]])>7 else []
     matched=next((ai for ai,anim in enumerate(anims)
                   if isinstance(anim,list) and anim and anim[0]==value),None)
     if matched is not None:
      for row in reversed(created):
       if row["child"]==a[0]:
        row["animation"]=matched
        break
   elif a[1]==72 and a[0]!=508 and len(a)>5 and a[5]:
    # Construct SetFrame acts on the most recently picked spawned Sprite.
    value=literal(a[5][0][1])
    if isinstance(value,int) and 0<=value<100000:
     for created_row in reversed(created):
      if created_row["child"]==a[0]:
       created_row["frame"]=value
       break
  if not created:continue
  events+=1
  for seq,item in enumerate(created):
   point=item["point"]
   rules.append((e[4],base,seq,item["parent"],item["child"],
                 item["layer"],point if isinstance(point,str) else "",
                 point if isinstance(point,int) else 0,
                 item["animation"],item["frame"]))
if not rules:raise SystemExit("No original source house Spawn events recovered")
parents={row[3] for row in rules}
# Object frame image points are relative to each source animation frame's
# own hotspot; multiple image points may live in ONE PNG atlas rectangle.
points=[]
for oid in sorted(parents):
 obj=objects[oid]
 anims=obj[7] if len(obj)>7 and isinstance(obj[7],list) else []
 for ai,a in enumerate(anims):
  frames=a[7] if len(a)>7 and isinstance(a[7],list) else []
  for fi,fr in enumerate(frames):
   if len(fr)<10:continue
   origin_x,origin_y=fr[7],fr[8]
   if not isinstance(fr[9],list):continue
   for j,pt in enumerate(fr[9],1):
    if not isinstance(pt,list) or len(pt)<3:continue
    if not isinstance(pt[0],str):continue
    points.append((oid,ai,fi,j,pt[0],pt[1],pt[2],
                   origin_x,origin_y))
out=ROOT/"include/minidayz/generated/source_house_companions.hpp"
out.parent.mkdir(parents=True,exist_ok=True)
def quote(v):return json.dumps(v,ensure_ascii=True)
def fl(v):
 s=format(float(v),".9g")
 if "." not in s and "e" not in s and "E" not in s:s+=".0"
 return s+"f"
lines=["#pragma once","#include <cstddef>","#include <cstdint>",
"#include <cstring>",
"namespace minidayz::world_source {",
"// Root Construct create_map_element nested Sprite command 73, in action order.",
"struct SourceHouseCompanionSpawn {",
" std::uint64_t event_uid;std::uint16_t base_object;",
" std::uint16_t sequence,parent_object,object;",
" const char* layer;const char* point_name;std::uint8_t point_index;",
" std::uint16_t animation;std::uint32_t frame;};",
"inline constexpr SourceHouseCompanionSpawn kSourceHouseCompanionSpawns[]={"]
for uid,base,seq,parent,obj,layer,name,index,anim,frame in rules:
 lines.append(" {"+",".join([str(uid)+"ULL",str(base),str(seq),
                              str(parent),str(obj),quote(layer),
                              quote(name),str(index),str(anim),str(frame)])+"},")
lines += ["};",
"inline constexpr std::size_t kSourceHouseCompanionSpawnCount=sizeof(kSourceHouseCompanionSpawns)/sizeof(kSourceHouseCompanionSpawns[0]);",
"struct SourceHouseImagePoint {",
" std::uint16_t object,animation;std::uint32_t frame;std::uint8_t index;",
" const char* name;float x,y,origin_x,origin_y;};",
"inline constexpr SourceHouseImagePoint kSourceHouseImagePoints[]={"]
for oid,ai,fi,j,name,x,y,ox,oy in points:
 lines.append(" {"+",".join([str(oid),str(ai),str(fi),str(j),
                              quote(name),fl(x),fl(y),fl(ox),fl(oy)])+"},")
lines += ["};",
"inline constexpr std::size_t kSourceHouseImagePointCount=sizeof(kSourceHouseImagePoints)/sizeof(kSourceHouseImagePoints[0]);",
"inline const SourceHouseImagePoint* source_house_image_point(",
" std::uint16_t object,std::uint16_t animation,std::uint32_t frame,",
" const char* name,std::uint8_t index) noexcept {",
" if((!name||!name[0])&&index==0)return nullptr;",
" for(const auto& p:kSourceHouseImagePoints){",
"  if(p.object!=object||p.animation!=animation||p.frame!=frame)continue;",
"  if(name&&name[0]){if(std::strcmp(p.name,name)==0)return &p;}",
"  else if(p.index==index)return &p;",
" }return nullptr;}",
"} // namespace minidayz::world_source"]
out.write_text("\n".join(lines)+"\n",encoding="utf-8")
print("Root source house companion Spawn actions:",len(rules),
      "events",events,"parents",len(parents),"imagepoints",len(points),
      "unsupported Spawn actions",ignored)
