#!/usr/bin/env python3
"""Extract ORIGINAL Mapgen category6 nested car trunk Sprite241 spawns.

Do not infer trunk existence from a generic car texture: original
create_map_element's car-wreck primary Spawn(508) is followed by a CHILD
event guarded by 508.var3==0 which calls Body.SpawnAnotherObject(241).
Both event ancestry and exact sprite frame/imagepoint are verified.
"""
from pathlib import Path
import json,sys
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tools/stage1"))
from audit_original_world_exhaustive import decode
p=decode((ROOT/"data.deobfuscated.js").read_bytes())
def walk(events,parents=()):
 for e in events if isinstance(events,list) else ():
  if isinstance(e,list) and len(e)>=7 and e[0]==0:
   yield e,parents
   yield from walk(e[7] if len(e)>7 else [],parents+(e,))
def literal(x):
 if isinstance(x,list) and len(x)==2 and x[0] in (0,2,4,5,7) and not isinstance(x[1],list):
  return x[1]
 return None
def check_var(e,slot,value):
 return any(isinstance(c,list) and len(c)>9 and c[:2]==[508,71]
    and c[9]==[[10,slot],[8,0],[7,[0,value]]] for c in e[5])
items=next(data for name,data in p[6] if name=="Game_events")
source_func=7736925111997576
source_category6=4174243855126541
found=[]
for e,anc in walk(items):
 if not e[6] or not any(x[4]==source_func for x in anc):
  continue
 if not any(x[4]==source_category6 and check_var(x,1,6) for x in anc):
  continue
 if not check_var(e,3,0):
  continue
 parent=anc[-1]
 variants=[]
 for c in parent[5]:
  if c[:2]==[508,71] and len(c)>9 and c[9][0]==[10,2] and c[9][1]==[8,0]:
   variants.append(literal(c[9][2][1]))
 if len(variants)!=1 or not isinstance(variants[0],int):
  continue
 v=variants[0]
 primary=[a for a in parent[6] if isinstance(a,list) and len(a)>5
          and a[:2]==[508,73] and isinstance(a[5],list)]
 if len(primary)!=1:raise SystemExit(f"Original car category6 parent changed {v}")
 body=literal(primary[0][5][0])
 spawns=[a for a in e[6] if isinstance(a,list) and len(a)>5
          and a[:2]==[body,73] and a[5][0]==[4,241]]
 if not spawns:continue
 if len(spawns)!=1:raise SystemExit(f"Multiple source car trunk spawns {v}")
 params=spawns[0][5]
 layer=literal(params[1][1])
 point=literal(params[2][1])
 if layer!="Sunset_Dawn" or point!=1:
  raise SystemExit(f"Original car overlay layer/imagepoint changed {v}")
 frames=[literal(a[5][0][1]) for a in e[6]
         if isinstance(a,list) and len(a)>5 and a[:2]==[241,72]]
 if len(frames)!=1 or not isinstance(frames[0],int):
  raise SystemExit(f"Original car Sprite241 SetFrame changed {v}: {frames}")
 frame=frames[0]
 authored=p[3][241][7][0][7][frame]
 if len(authored)<9 or authored[4]<=0 or authored[5]<=0:
  raise SystemExit(f"Original car frame dimensions invalid {v}: {frame}")
 # Preserve original source parent Sprite action UID as proof of origin.
 found.append((v,body,frame,point,layer,e[4],authored[4],authored[5],
               authored[7],authored[8]))
assert len(found)==15 and len({i[0] for i in found})==15,found
assert {i[0] for i in found}==set(range(1,13))|{15,16,17}
out=ROOT/"include/minidayz/generated/source_car_wreck_trunks.hpp"
out.parent.mkdir(parents=True,exist_ok=True)
lines=["#pragma once","#include <cstdint>",
 "namespace minidayz::world_source {",
 "// Exact root Game_events/create_map_element source category6; nested",
 "// Mapgen508.var3=0, Body.SpawnAnotherObject(241) at Imagepoint1.",
 "struct SourceCarWreckTrunk {",
 " std::uint8_t variant;std::uint16_t body_object;",
 " std::uint32_t frame;std::uint8_t image_point;const char* layer;",
 " std::uint64_t source_uid;float width,height,origin_x,origin_y;};",
 "inline constexpr SourceCarWreckTrunk kSourceCarWreckTrunks[]={"]
for v,body,frame,point,layer,uid,w,h,ox,oy in sorted(found):
 def fl(x):
  s=format(float(x),".9g")
  if "." not in s and "e" not in s:s+=".0"
  return s+"f"
 lines.append(" {"+",".join([str(v),str(body),str(frame),str(point),
       json.dumps(layer),str(uid)+"ULL",fl(w),fl(h),fl(ox),fl(oy)])+"},")
lines+=["};","} // namespace minidayz::world_source",""]
out.write_text("\n".join(lines),encoding="utf-8")
print("PASS ORIGINAL CAR-WRECK TRUNKS",[(v,body,frame) for v,body,frame,*_ in found])
