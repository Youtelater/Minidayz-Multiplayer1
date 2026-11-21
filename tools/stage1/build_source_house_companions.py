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
def imagepoint_delta(expr,axis):
 # Root building SetSize( ImagePointX("w2")-ImagePointX("w1"), ... )
 if not isinstance(expr,list) or len(expr)!=3 or expr[0]!=5:return None
 def endpoint(node):
  if not isinstance(node,list) or len(node)<6 or node[0]!=20 or node[2]!=axis:return None
  ps=node[5]
  if not isinstance(ps,list) or not ps:return None
  name=literal(ps[0])
  if not isinstance(name,str):return None
  return node[1],name
 hi=endpoint(expr[1]);lo=endpoint(expr[2])
 if not hi or not lo or hi[0]!=lo[0]:return None
 return hi[0],hi[1],lo[1]

def spawn(a):
 if not isinstance(a,list) or len(a)<6 or a[1]!=73:return None
 ps=a[5]
 if not isinstance(ps,list) or len(ps)<3:return None
 target=literal(ps[0]);layer=literal(ps[1][1]);point=literal(ps[2][1])
 if not isinstance(target,int) or not 0<=target<len(objects):return None
 if not isinstance(layer,str) or not isinstance(point,(int,str)):return None
 return a[0],target,layer,point
rules=[];ignored=0;events=0;primaries=[];unverified_primary_conditions=0
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
  # Original child event predicate: Mapgen508 instance variable index2
  # equals the current create_map_element variant. This is stronger than
  # trusting stale generated event UIDs in world_source_data.hpp.
  variants=[]
  for condition in e[5] if isinstance(e[5],list) else []:
   if not isinstance(condition,list) or len(condition)<10 or condition[:2]!=[508,71]:continue
   params=condition[9]
   if not isinstance(params,list) or len(params)<3:continue
   last=params[-1]
   value=literal(last[1]) if isinstance(last,list) and len(last)>1 else None
   if isinstance(value,int) and 0<=value<=255:variants.append(value)
  if len(variants)!=1:continue
  variant=variants[0]
  # Source parent UID 3673396466799513 is original var1==7 Mapgen
  # branch; source event condition 508.var2 chooses the variant.
  # A standalone conditional/complex secondary branch is never guessed.
  is_house_parent=3673396466799513 in anc
  original_conditions=e[5] if isinstance(e[5],list) else []
  direct_var2_condition=(
      is_house_parent and len(original_conditions)==1 and
      original_conditions[0][:2]==[508,71])
  if direct_var2_condition:
      primaries.append((e[4],base,variant,roots[0][2],roots[0][3]))
  else:
      unverified_primary_conditions+=1
  created=[]
  for a in acts:
   if not isinstance(a,list) or len(a)<2:continue
   sp=spawn(a)
   if sp and sp[0]!=508:
    # A companion belongs to the exact original source event and the
    # specific base Sprite. Repeated event UIDs can serve multiple bases.
    created.append({"parent":sp[0],"child":sp[1],"layer":sp[2],
                    "point":sp[3],"frame":0,"animation":0,
                    "width_delta":None,"height_delta":None,
                    "angle_degrees":99999.0,"opacity":1.0,
                    "variables":{}})
   elif a[1]==73 and a[0]!=508:ignored+=1
   elif a[1] in (103,169) and a[0]!=508 and len(a)>5 and a[5]:
    width=imagepoint_delta(a[5][0][1],59) if a[1]==103 else None
    height=imagepoint_delta(
        a[5][1][1] if a[1]==103 and len(a[5])>1 else a[5][0][1],
        60)
    for row in reversed(created):
     if row["child"]==a[0]:
      if width:row["width_delta"]=width
      if height:row["height_delta"]=height
      break
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
   elif a[1]==88 and a[0]!=508 and len(a)>5 and len(a[5])>=2:
    # Original SetInstanceVariable: literals and UID-of-source-object
    # expressions (command 151) are preserved, not guessed. The source
    # links wall 346, shadow 347 and door 349 to particular parents; it
    # also sets original loot-marker 218 category 9/19.
    index=literal(a[5][0][1])
    expr=a[5][1][1]
    if isinstance(index,int) and index in (0,1):
     kind=0;value=0;source_object=65535
     raw=literal(expr)
     if isinstance(raw,int):
      kind=1;value=raw
     elif isinstance(expr,list) and len(expr)>=3 and expr[0]==20 and expr[2]==151 and isinstance(expr[1],int):
      kind=2;source_object=expr[1]
     if kind:
      for row in reversed(created):
       if row["child"]==a[0]:
        row["variables"][index]=(kind,value,source_object)
        break
   elif a[1] in (84,137,163) and a[0]!=508 and len(a)>5 and a[5]:
    # Same-event authored post-Spawn transforms, literal values only.
    value=literal(a[5][0][1])
    if a[1]==84 and isinstance(value,(int,float)) and -3600<=value<=3600:
     for row in reversed(created):
      if row["child"]==a[0]:row["angle_degrees"]=float(value);break
    elif a[1]==163 and isinstance(value,(int,float)) and 0<=value<=100:
     for row in reversed(created):
      if row["child"]==a[0]:row["opacity"]=float(value)/100.0;break
    elif a[1]==137 and isinstance(value,str):
     for row in reversed(created):
      if row["child"]==a[0]:row["layer"]=value;break
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
   rules.append((e[4],base,variant,seq,item["parent"],item["child"],
                 item["layer"],point if isinstance(point,str) else "",
                 point if isinstance(point,int) else 0,
                 item["animation"],item["frame"],
                 item["width_delta"],item["height_delta"],
                 item["angle_degrees"],item["opacity"],item["variables"]))
if not rules:raise SystemExit("No original source house Spawn events recovered")
parents={row[4] for row in rules}
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
"// Literal root source primary Mapgen508 Spawn command in var1=7 branch.",
"struct SourceHousePrimarySpawn {std::uint64_t event_uid;",
" std::uint16_t object;std::uint8_t variant;const char* layer;",
" std::uint8_t point_index;};",
"inline constexpr SourceHousePrimarySpawn kSourceHousePrimarySpawns[]={"]
for uid,base,variant,layer,point in dict.fromkeys(primaries):
    if not isinstance(point,int) or not 0<=point<255:continue
    lines.append(" {"+",".join([str(uid)+"ULL",str(base),str(variant),
                                quote(layer),str(point)])+"},")
lines+=["};",
"inline constexpr std::size_t kSourceHousePrimarySpawnCount=sizeof(kSourceHousePrimarySpawns)/sizeof(kSourceHousePrimarySpawns[0]);",
"// Root Construct create_map_element nested Sprite command 73, in action order.",
"struct SourceHouseCompanionSpawn {",
" std::uint64_t event_uid;std::uint16_t base_object;",
" std::uint8_t variant;",
" std::uint16_t sequence,parent_object,object;",
" const char* layer;const char* point_name;std::uint8_t point_index;",
" std::uint16_t animation;std::uint32_t frame;",
" std::uint16_t width_parent,height_parent;",
" const char* width_hi;const char* width_lo;",
" const char* height_hi;const char* height_lo;",
" float angle_degrees,opacity;};",
"inline constexpr SourceHouseCompanionSpawn kSourceHouseCompanionSpawns[]={"]
for uid,base,variant,seq,parent,obj,layer,name,index,anim,frame,wd,hd,ang,opacity,vars in rules:
 lines.append(" {"+",".join([str(uid)+"ULL",str(base),str(variant),str(seq),
                              str(parent),str(obj),quote(layer),
                              quote(name),str(index),str(anim),str(frame),
                              str(wd[0]) if wd else "65535",
                              str(hd[0]) if hd else "65535",
                              quote(wd[1]) if wd else quote(""),
                              quote(wd[2]) if wd else quote(""),
                              quote(hd[1]) if hd else quote(""),
                              quote(hd[2]) if hd else quote(""),
                              fl(ang),fl(opacity)])+"},")
lines += ["};",
"inline constexpr std::size_t kSourceHouseCompanionSpawnCount=sizeof(kSourceHouseCompanionSpawns)/sizeof(kSourceHouseCompanionSpawns[0]);",
"// Source Sprite SetInstanceVariable 88 immediately after Spawn 73.",
"struct SourceHouseCompanionVariable {",
" std::uint64_t event_uid;std::uint16_t base_object;",
" std::uint8_t variant;std::uint16_t sequence,child,slot;",
" std::uint8_t kind;std::int32_t literal_value;",
" std::uint16_t uid_object;};",
"inline constexpr SourceHouseCompanionVariable kSourceHouseCompanionVariables[]={",
]
variable_rows=[]
for uid,base,variant,seq,parent,obj,layer,name,index,anim,frame,wd,hd,ang,opacity,vars in rules:
 for slot,(kind,value,uid_object) in sorted(vars.items()):
  variable_rows.append((uid,base,variant,seq,obj,slot,kind,value,uid_object))
for uid,base,variant,seq,obj,slot,kind,value,uid_object in variable_rows:
 lines.append(" {"+",".join([str(uid)+"ULL",str(base),str(variant),
     str(seq),str(obj),str(slot),str(kind),str(value),str(uid_object)])+"},")
lines += [
"};",
"inline constexpr std::size_t kSourceHouseCompanionVariableCount=sizeof(kSourceHouseCompanionVariables)/sizeof(kSourceHouseCompanionVariables[0]);",
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
for oid in (302,306,311,312,313,321,322,323,324,333,346,347,349):
 obj=objects[oid]
 anims=obj[7] if len(obj)>7 and isinstance(obj[7],list) else []
 fr=anims[0][7][0] if anims and len(anims[0])>7 and anims[0][7] else []
 print("SOURCE_HOUSE_GEOMETRY",oid,
       "frameWH",fr[4:6] if len(fr)>5 else [],
       "point_names",[p[0] for p in fr[9]] if len(fr)>9 and isinstance(fr[9],list) else [])
print("Root source house companion Spawn actions:",len(rules),
      "events",events,"parents",len(parents),"imagepoints",len(points),
      "source instance-variable actions",len(variable_rows),
      "unsupported Spawn actions",ignored,
      "literal primary branches",len(primaries),
      "complex-source primary branch guards deferred",unverified_primary_conditions)
