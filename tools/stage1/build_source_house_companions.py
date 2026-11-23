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
  if not isinstance(e,list) or len(e)<7 or e[0]!=0:continue
  yield e,ancestry
  if len(e)>7:yield from walk(e[7],ancestry+(e[4],))
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
unverified_primary_branches=[]
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
   if not isinstance(params,list) or len(params)<3 or params[0]!=[10,2]:continue
   last=params[-1]
   value=literal(last[1]) if isinstance(last,list) and len(last)>1 else None
   if isinstance(value,int) and 0<=value<=255:variants.append(value)
  # Original category 4 variants 46/47 have an unconditional nested Spawn
  # event with the variant condition on its immediate parent.
  nested_variant={502091414218817:46,656789528671005:47}.get(e[4])
  if nested_variant is not None and 1506402191387381 in anc and not e[5]:
   variants=[nested_variant]
  # Root variants 46/47 put their Spawn actions in an unconditional
  # child of the literal var2 guard. Inherit that guard only for these
  # independently inspected parent/child pairs; no other conditional
  # event path is implicitly promoted.
  inherited_guards={
      502091414218817:(504327785638027,46),
      656789528671005:(352684961907206,47),
  }
  inherited=inherited_guards.get(e[4])
  if inherited and not e[5] and anc[-1]==inherited[0]:
   variants=[inherited[1]]
  # Category 4 variant 40 selects seven structures by inherited var2=40
  # and local var6=0..6. The runtime already matches each exact primary
  # source event UID, so retaining variant 40 here is sufficient and
  # preserves all seven original companion Solid348 spawn chains.
  if not variants and anc[-1]==431776969482924 and 1506402191387381 in anc:
   if len(e[5])!=1 or e[5][0][:2]!=[508,71] or e[5][0][9][:2]!=[[10,6],[8,0]]:
    raise RuntimeError("Unexpected original variant40 subtype guard")
   subtype=literal(e[5][0][9][2][1])
   if not isinstance(subtype,int) or not 0<=subtype<=6:
    raise RuntimeError("Unexpected original variant40 subtype")
   variants=[40]
  if len(variants)!=1:continue
  variant=variants[0]
  # The outer 3673396466799513 event encloses all categories.
  # Only the actual category-7 branch is the primary house catalog.
  is_house_parent=4949969985883762 in anc
  original_conditions=e[5] if isinstance(e[5],list) else []
  direct_var2_condition=(is_house_parent and
      len(original_conditions)==1 and
      original_conditions[0][:2]==[508,71] and
      original_conditions[0][9][0]==[10,2] and
      original_conditions[0][9][1]==[8,0])
  if direct_var2_condition:
      primaries.append((e[4],base,variant,roots[0][2],roots[0][3]))
  elif is_house_parent:
      unverified_primary_conditions+=1
      unverified_primary_branches.append((e[4],base,variant,
                                          original_conditions,anc[-5:]))
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
    index=a[5][0][1] if (isinstance(a[5][0],list) and len(a[5][0])>1) else None
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
# Root category 7 contains 51 primary events, all single literal variant
# guards. Fail closed if the original changes: runtime does not consult the
# stale cached category-7 table anymore.
if len(primaries)!=51 or unverified_primary_conditions:
 raise RuntimeError(f"Unexpected category-7 primary coverage: {len(primaries)} direct, {unverified_primary_conditions} complex")
if len({r[2] for r in primaries})!=len(primaries):
 raise RuntimeError("Repeated category-7 variant needs explicit source selection")

# The original loot branch is separate from building creation: Mapgen
# 508.var1=7, var2=building variant, var3=0. Its last action marks var3=1.
# Emit every literal Spawn/SetVariable/Trigger_spawn in source order.
# Preserve markers without a Trigger_spawn call (two city-house points).
loot_markers=[]
def loot_walk(nodes,active=False,variant=None):
 for e in nodes:
  if not isinstance(e,list) or len(e)<7 or e[0]!=0:continue
  in_branch=active or e[4]==4949969985883762
  v=variant
  if in_branch:
   for c in e[5]:
    if c[:2]==[508,71] and c[9][0]==[10,2] and c[9][1]==[8,0]:
     v=literal(c[9][2][1])
   pending=None
   for a in e[6]:
    sp=spawn(a)
    if sp and sp[1]==218:
     if v is None:raise ValueError("Missing original house loot variant")
     pending=[e[4],a[3],v,sp[0],sp[2],sp[3],None,False,False]
     loot_markers.append(pending)
    elif pending is not None and a[:2]==[218,88]:
     if a[5][0]!=[10,0]:raise ValueError("Unexpected marker variable")
     pending[6]=literal(a[5][1][1])
    elif pending is not None and a[:2]==[218,55]:
     # Source variant 1: marker.Y = marker.Y + parent.Height/2 + 15.
     expected=[4,[4,[20,218,66,False,None],[7,[20,pending[3],61,False,None],[0,2]]],[0,15]]
     if a[5][0][1]!=expected:raise ValueError("Unsupported source marker Y expression")
     pending[8]=True
    elif pending is not None and a[:2]==[189,69]:
     if a[5][0][1]!=[2,"Trigger_spawn"]:raise ValueError("Unexpected marker function")
     pending[7]=True
  if len(e)>7 and isinstance(e[7],list):loot_walk(e[7],in_branch,v)
for name,nodes in p[6]:
 if name=="Game_events":loot_walk(nodes)
if len(loot_markers)!=153 or any(r[6] is None for r in loot_markers):
 raise RuntimeError("Original house loot markers missing: "+str(len(loot_markers)))
# Construct layer names are case-insensitive. Resolve the exact Map layer.
map_layers={l[0].lower():l[0] for l in next(l for l in p[5] if l[0]=="Map")[6]}
for r in loot_markers:r[4]=map_layers[r[4].lower()]
# Car Sprite241 is spawned as a child of authored wreck scenery, not a
# SpawnAnotherObject parent. Root trunk Spawn_drop explicitly targets
# 241.ImagePointX/Y(3); it must be in the exact image-point catalog.
parents={row[4] for row in rules} | {row[3] for row in loot_markers} | {241}

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
"#include <strings.h>",
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
"  if(name&&name[0]){if(::strcasecmp(p.name,name)==0)return &p;}",
"  else if(p.index==index)return &p;",
" }return nullptr;}",
"} // namespace minidayz::world_source"]
# Append before the namespace closes, using the same original sprite imagepoint table.
lines.pop()
lines += [
 "struct SourceHouseLootMarker {std::uint64_t event_uid,action_uid;",
 " std::uint8_t variant;std::uint16_t parent;const char* layer;const char* point_name;",
 " std::uint8_t point_index;std::uint16_t category;bool trigger_spawn,offset_y;};",
 "inline constexpr SourceHouseLootMarker kSourceHouseLootMarkers[]={"]
for uid,aid,variant,parent,layer,pt,category,trigger,offset in loot_markers:
 lines.append(" {"+",".join([str(uid)+"ULL",str(aid)+"ULL",str(variant),str(parent),
    quote(layer),quote(pt if isinstance(pt,str) else ""),str(pt if isinstance(pt,int) else 0),
    str(category),str(trigger).lower(),str(offset).lower()])+"},")
lines += ["};","inline constexpr std::size_t kSourceHouseLootMarkerCount=sizeof(kSourceHouseLootMarkers)/sizeof(kSourceHouseLootMarkers[0]);",
"} // namespace minidayz::world_source"]
out.write_text("\n".join(lines)+"\n",encoding="utf-8")
for oid in (302,306,311,312,313,321,322,323,324,333,346,347,348,349,1323):
 obj=objects[oid]
 anims=obj[7] if len(obj)>7 and isinstance(obj[7],list) else []
 fr=anims[0][7][0] if anims and len(anims[0])>7 and anims[0][7] else []
 print("SOURCE_HOUSE_GEOMETRY",oid,
       "frameWH",fr[4:6] if len(fr)>5 else [],
       "point_names",[p[0] for p in fr[9]] if len(fr)>9 and isinstance(fr[9],list) else [])
for uid,base,variant,conditions,anc in unverified_primary_branches:
 print("SOURCE_COMPLEX_PRIMARY_HOUSE",uid,base,variant,
       "conditions",json.dumps(conditions,separators=(",",":"),
                               ensure_ascii=False)[:2200],
       "ancestry",list(anc))
for obj_id,ai,fi in ((1323,0,0),(348,0,30)):
 obj=objects[obj_id]
 animations=obj[7] if len(obj)>7 else []
 frame=(animations[ai][7][fi] if ai<len(animations) and
        len(animations[ai])>7 and fi<len(animations[ai][7]) else None)
 print("SOURCE_MISSING_HOUSE_FRAME",obj_id,ai,fi,
       "frame_fields",json.dumps(frame[:11] if frame else None,
                                  ensure_ascii=False,separators=(",",":"))[:3400])
print("SOURCE_VARIANT36_RULES",
 [(x[0],x[4],x[5],x[6],x[7],x[8],x[9],x[10])
  for x in rules if x[2]==36])
print("Root source house companion Spawn actions:",len(rules),
      "events",events,"parents",len(parents),"imagepoints",len(points),
      "source instance-variable actions",len(variable_rows),
      "unsupported Spawn actions",ignored,
      "literal primary branches",len(primaries),
      "complex-source primary branch guards deferred",unverified_primary_conditions)
