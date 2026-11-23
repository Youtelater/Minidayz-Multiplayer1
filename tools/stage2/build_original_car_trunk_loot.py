#!/usr/bin/env python3
"""Generate original non-Tutorial searchable car-trunk rewards.

All probabilities, frame-class branches and duplicate item weights are
validated against ROOT data.deobfuscated.js Game_events. Never use a
guessed percentage or items from the unrelated LOOT Respawn subsystem.
"""
from pathlib import Path
import sys,json
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tools/stage1"))
from audit_original_world_exhaustive import decode
project=decode((ROOT/"data.deobfuscated.js").read_bytes())
OUTPUT=ROOT/"include/minidayz/generated/original_car_trunk_loot.hpp"
# Events are the root Construct's three mutually exclusive non-Tutorial
# loot branches for frame groups 0/3/5, 1 and 2/4 (Sprite241).
SOURCE_BRANCHES=(
 (226004896143721,(0,3,5)),
 (880252604545313,(1,)),
 (961432369386549,(2,4)),
)
def walk(events,anc=()):
 for e in events if isinstance(events,list) else []:
  if not isinstance(e,list) or len(e)<7 or e[0]!=0:continue
  yield e,anc
  if len(e)>7:
   yield from walk(e[7],anc+(e,))
events=[(sheet,e,parents) for sheet,items in project[6]
        for e,parents in walk(items)]
def literals(expr):
 if not(isinstance(expr,list) and len(expr)==3 and expr[:2]==[19,82]):
  raise SystemExit("Original car drop is not a literal Construct choose()")
 result=[]
 for item in expr[2]:
  if not (isinstance(item,list) and len(item)==2 and item[0]==0 and
          isinstance(item[1],int) and 1<=item[1]<=65535):
   raise SystemExit("Nonliteral or invalid original car drop item")
  result.append(item[1])
 if not result:raise SystemExit("Empty original car loot pool")
 return result
def spawned_items(event):
 found=[]
 for action in event[6] if isinstance(event[6],list) else []:
  if not(isinstance(action,list) and len(action)>5 and
         action[:2]==[189,69] and isinstance(action[5],list) and
         len(action[5])>1 and action[5][0]==[1,[2,"Spawn_drop"]]):
   continue
  args=action[5][1]
  if not(isinstance(args,list) and len(args)>=4 and args[0]==13 and
         isinstance(args[1],list) and args[1][0]==7):
   raise SystemExit("Changed original Spawn_drop arguments")
  # Original Sprite241.ImagePointX/Y(3), not the car's visual hotspot.
  if args[2] != [7,[20,241,59,False,None,[[0,3]]]] or \
     args[3] != [7,[20,241,60,False,None,[[0,3]]]]:
   raise SystemExit("Original car drop source Imagepoint3 changed")
  found.append(literals(args[1][1]))
 return found
def gate_is_base(event):
 cond=event[5] if isinstance(event[5],list) else []
 return len(cond)==1 and cond[0][:2]==[-1,51] and len(cond[0])>9 and \
   cond[0][9]==[[7,[23,"trunk_rng"]],[8,2],
                [7,[4,[0,50],[23,"trunk_bonus"]]]]
def frame_groups(anc):
 for node in reversed(anc):
  relevant=[c for c in node[5] if isinstance(c,list) and len(c)>9
            and c[:2]==[241,64]]
  if not relevant:continue
  result=[]
  for guard in relevant:
   x=guard[9]
   if not(isinstance(x,list) and len(x)==2 and x[0]==[8,0]
           and isinstance(x[1],list) and len(x[1])==2
           and x[1][0]==0 and isinstance(x[1][1],list)
           and x[1][1][0]==0):
    raise SystemExit("Original car frame condition changed")
   result.append(x[1][1][1])
  return tuple(result)
 return None
records=[]
for uid,frames in SOURCE_BRANCHES:
 candidates=[(e,parents) for sheet,e,parents in events if
   sheet=="Game_events" and e[4]==uid and gate_is_base(e) and
   any(a[4] in (7711324247664447,1792234758292318)
       for a in parents)]
 if len(candidates)!=1:
  raise SystemExit(f"Original map-car source branch {uid}: {len(candidates)}")
 event,parents=candidates[0]
 ancestry_frames=frame_groups(parents)
 if ancestry_frames!=frames:
  raise SystemExit(f"Original car frame group changed {uid}: {ancestry_frames}")
 primary=spawned_items(event)
 if len(primary)!=1:raise SystemExit(f"Original car drop count changed {uid}")
 # Source Trunkster level-2: nested event has random(100)<10 and drops
 # the identical weighted pool a second time.
 extras=[]
 for child,ca in walk(event[7] if len(event)>7 else []):
  drops=spawned_items(child)
  if not drops:continue
  conds=[c for ancestor in (event,*ca,child) for c in ancestor[5]]
  if not any(c[:2]==[-1,46] and len(c)>9 and
     c[9]==[[11,"perk_trunkster"],[8,0],[7,[0,2]]] for c in conds):
   raise SystemExit(f"Extra car drop missing Trunkster level2 {uid}")
  if not any(c[:2]==[-1,51] and len(c)>9 and
     c[9]==[[7,[19,95,[[0,100]]]],[8,2],[7,[0,10]]] for c in conds):
   raise SystemExit(f"Extra car drop original 10% guard changed {uid}")
  extras.extend(drops)
 if len(extras)!=1 or extras[0]!=primary[0]:
  raise SystemExit(f"Original extra Trunkster car pool changed {uid}")
 records.append((uid,frames,primary[0]))
# Confirm the independent level-1+ original source sets trunk_bonus to 10.
if not any("trunk_bonus" in str(a[5]) and "[0, 10]" in str(a[5])
    for _,e,_ in events for a in e[6]
    if isinstance(a,list) and len(a)>5 and a[:2]==[-1,41]):
 raise SystemExit("Original Trunkster bonus +10 source global changed")
# Root mobile pick-and-interact UID711142196261088:
# distance(Player193, Sprite241) <= 100. Keep the original comparison
# opcode and literal range; do not use an invented 50px interaction radius.
range_events=[e for sheet,e,_ in events if sheet=="Game_events"
              and e[4]==711142196261088]
if len(range_events)!=1 or not any(
    isinstance(c,list) and len(c)>9 and c[:2]==[-1,51] and
    c[9][1]==[8,3] and c[9][2]==[7,[0,100]] and
    isinstance(c[9][0],list) and c[9][0][0]==7 and
    isinstance(c[9][0][1],list) and c[9][0][1][:2]==[19,79]
    for c in range_events[0][5]):
 raise SystemExit("Original car mobile interact <=100 source guard changed")
# Verify the original Sprite241 frame3 drop imagepoint exists for each frame.
sprite=project[3][241]
if not sprite or len(sprite)<=7:raise SystemExit("Original car loot Sprite241 missing")
animations=sprite[7]
if not animations or len(animations[0])<=7 or len(animations[0][7])<6:
 raise SystemExit("Car source frames 0..5 missing")
for frame in animations[0][7][:6]:
 if len(frame)<10 or not isinstance(frame[9],list) or len(frame[9])<3:
  raise SystemExit("Car trunk drop Imagepoint3 missing in source")
lines=[
 "#pragma once","#include <cstddef>","#include <cstdint>",
 "namespace minidayz::items {",
 "// Generated from root Game_events, car Sprite241, not invented loot.",
 "struct OriginalCarTrunkPool {std::uint64_t event_uid;",
 " std::uint32_t first;std::uint16_t count;",
 " std::uint8_t frame_first;std::uint8_t frame_mask;};",
 "inline constexpr std::uint8_t kOriginalCarTrunkBaseChance=50;",
 "inline constexpr std::uint8_t kOriginalCarTrunkBonusOnTrunkster=10;",
 "inline constexpr std::uint8_t kOriginalCarTrunkExtraChance=10;",
 "inline constexpr std::uint8_t kOriginalCarTrunkRollMax=100;",
 "inline constexpr float kOriginalCarTrunkInteractionRange=100.0f;",
 "inline constexpr std::uint16_t kOriginalCarTrunkDropImagepoint=3;",
 "inline constexpr std::uint16_t kOriginalCarTrunkItems[]={"]
flat=[]
rows=[]
for uid,frames,values in records:
 rows.append((uid,len(flat),len(values),min(frames),
              sum(1<<frame for frame in frames)))
 flat.extend(values)
for i in range(0,len(flat),20):
 lines.append(" "+",".join(str(x) for x in flat[i:i+20])+",")
lines.extend(["};","inline constexpr OriginalCarTrunkPool kOriginalCarTrunkPools[]={"])
for uid,start,count,first,mask in rows:
 lines.append(f" {{{uid}ULL,{start},{count},{first},{mask}}},")
lines.extend(["};","} // namespace minidayz::items",""])
OUTPUT.parent.mkdir(parents=True,exist_ok=True)
OUTPUT.write_text("\n".join(lines),encoding="utf-8")
print("SOURCE_CAR_TRUNK_LOOT",json.dumps({"car_frame_pools":[{"source_uid":u,
 "frames":f,"choices":len(ids),"duplicates":len(ids)-len(set(ids))}
 for u,f,ids in records],"base_chance":50,
 "trunkster_bonus":10,"extra_trunkster_chance":10,
 "original_drop_imagepoint":3},sort_keys=True))
