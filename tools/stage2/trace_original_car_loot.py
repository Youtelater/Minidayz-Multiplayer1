#!/usr/bin/env python3
"""Inspect original car-loot probability/event chains, never invent chance or drops.

Evidence from repository-root data.deobfuscated.js Construct export only.
"""
from pathlib import Path
import sys,json,collections
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tools/stage1"))
from audit_original_world_exhaustive import decode
project=decode((ROOT/"data.deobfuscated.js").read_bytes())
for objid in (241,218,246,344,434,508):
 obj=project[3][objid]
 print("CAR_OBJECT",objid,json.dumps(obj[:7],ensure_ascii=False,separators=(",",":"))[:1500])
def walk(events,parents=()):
 for e in events if isinstance(events,list) else ():
  if not isinstance(e,list) or len(e)<7 or e[0]!=0:continue
  yield e,parents
  if len(e)>7:yield from walk(e[7],parents+(e,))
references=[]
spawns=[]
for sheet,events in project[6]:
 for e,parents in walk(events):
  conds=e[5] if isinstance(e[5],list) else []
  acts=e[6] if isinstance(e[6],list) else []
  c241=[c for c in conds if isinstance(c,list) and len(c)>2 and c[0]==241]
  a241=[a for a in acts if isinstance(a,list) and len(a)>2 and a[0]==241]
  car_create=[a for a in acts if isinstance(a,list) and len(a)>5 and
      a[:2] in ([-1,94],[508,73],[241,73]) and
      a[5] and isinstance(a[5][0],list) and a[5][0]==[4,241]]
  if c241 or a241 or car_create:
   row={"sheet":sheet,"uid":e[4],"parent_uids":[p[4] for p in parents[-5:]],
        "conds":conds,"acts":acts,"c241":len(c241),"a241":len(a241),
        "creates241":len(car_create)}
   references.append(row)
   if car_create:spawns.append(row)
print("CAR_EVENT_COUNT",len(references),"car_spawns",len(spawns))
for row in references[:115]:
 print("CAR_EVENT",json.dumps(row,ensure_ascii=False,separators=(",",":"))[:6200])
for row in spawns[:45]:
 print("CAR_CREATION",json.dumps(row,ensure_ascii=False,separators=(",",":"))[:6200])
# All event names/chains with car loot and random source expressions.
for sheet,events in project[6]:
 for e,parents in walk(events):
  txt=json.dumps(e[5:7],ensure_ascii=False,separators=(",",":"))
  if any(word in txt.lower() for word in ("car_loot","carloot","loot_car","wreck")):
   print("CAR_NAMED_EVENT",sheet,e[4],json.dumps({
     "parents":[p[4] for p in parents[-6:]],
     "conditions":e[5],"actions":e[6]},ensure_ascii=False,separators=(",",":"))[:7000])
