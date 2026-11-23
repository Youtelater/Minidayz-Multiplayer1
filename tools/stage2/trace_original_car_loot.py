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

# Interpret the ORIGINAL interact / search event ancestry, including
# probability comparisons, never infer chance from loot item weights.
TRACE_UIDS={
 926793085373299,5295137988838652,5907389574047888,
 7711324247664447,1074032691412307,400880056066513,
 191794159726343,226004896143721,147540547237867,
 626175794442741,701148703897151,483051176616500,
 1182523301410486,8880091309135125,6075180485305278,
 1923316192354192,711142196261088,1792234758292318,
 3302144065259174,838215728858889,4701221851322259,
 388182302027069,7634579179280124,6684143115687861,
 1430881814617229,8106312900809434,6755804858417062,
}
print("CAR_SOURCE_ANCESTOR_EVENTS")
for sheet,events in project[6]:
 for e,parents in walk(events):
  if e[4] not in TRACE_UIDS:continue
  print("CAR_PARENT",sheet,e[4],"anc",tuple(x[4] for x in parents[-7:]),
    "conds",json.dumps(e[5],ensure_ascii=False,separators=(",",":"))[:4700],
    "acts",json.dumps(e[6],ensure_ascii=False,separators=(",",":"))[:4700])
# Enumerate all original Spawn_drop calls under Car open event UID and
# preserve full source comparison/choice probabilities via ancestor chain.
for sheet,events in project[6]:
 for e,parents in walk(events):
  actions=e[6] if isinstance(e[6],list) else ()
  drops=[a for a in actions if isinstance(a,list) and len(a)>5 and
    a[:2]==[189,69] and a[5] and a[5][0]==[1,[2,"Spawn_drop"]]]
  if not drops or not any(x[4] in (7711324247664447,1923316192354192) for x in parents):
   continue
  print("CAR_DROP_CHAIN",e[4],sheet,
    "ancestors",json.dumps([(a[4],a[5]) for a in parents[-10:]],ensure_ascii=False,separators=(",",":"))[:9000],
    "conds",json.dumps(e[5],ensure_ascii=False,separators=(",",":"))[:2200],
    "drops",json.dumps(drops,ensure_ascii=False,separators=(",",":"))[:2500])

# Source 1041 is a Construct family: report its ORIGINAL member sprite
# filenames before deciding how to pick the car whose trunk241 is searched.
for family in project[4]:
 if not(isinstance(family,list) and family and family[0]==1041):continue
 for id in family[1:]:
  obj=project[3][id]
  sprites=obj[7] if len(obj)>7 and isinstance(obj[7],list) else []
  frames=sprites[0][7] if sprites and len(sprites[0])>7 else []
  pic=frames[0][0] if frames and isinstance(frames[0],list) else ""
  print("CAR_COLLISION_FAMILY_1041",id,obj[0],pic)
# Catch the source action that actually creates the separate 241
# interaction marker. Sprite art existing is NOT evidence of a world spawn.
create_calls=[]
for sheet,events in project[6]:
 for e,parents in walk(events):
  for action in e[6] if isinstance(e[6],list) else ():
   if not(isinstance(action,list) and len(action)>5):continue
   if action[1] not in (73,94,161):continue
   if "[4, 241]" in str(action[5]) or "[4,241]" in str(action[5]):
    create_calls.append((sheet,e[4],[a[4] for a in parents],action))
for sheet,uid,anc,action in create_calls:
 print("SOURCE_CAR_241_CREATION",sheet,uid,"anc",anc[-8:],
       "action",json.dumps(action,ensure_ascii=False,separators=(",",":"))[:2800])
print("SOURCE_CAR_241_CREATION_COUNT",len(create_calls))
