#!/usr/bin/env python3
"""Root source Step11 audit: authored interior container objects and interactions.

No secondary community/data overrides; emit exact Construct object/event ids.
"""
from pathlib import Path
import json,sys,re,collections
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tools"/"stage1"))
from audit_original_world_exhaustive import decode
p=decode((ROOT/"data.deobfuscated.js").read_bytes())
objects=p[3]
keywords=re.compile(r"container|cabinet|dresser|chest|locker|fridge|frigi|cupboard|shelf|storage|stash|box|crate|barrel|bed|wardrobe|furniture|table|drawer|safe|trunk|tent|case|loot|fridge",re.I)
candidates={}
print('SAMPLE_OBJECT_NAMES',[(i,str(obj[0])[:80]) for i,obj in enumerate(objects[:55]) if isinstance(obj,list) and obj])
for i,obj in enumerate(objects):
 if not isinstance(obj,list) or not obj:continue
 sheets=set()
 if len(obj)>7 and isinstance(obj[7],list):
  for anim in obj[7]:
   if isinstance(anim,list) and len(anim)>7 and isinstance(anim[7],list):
    for fr in anim[7]:
     if isinstance(fr,list) and fr and isinstance(fr[0],str):sheets.add(fr[0])
 matched=sorted(x for x in sheets if keywords.search(x))
 if (isinstance(obj[0],str) and keywords.search(obj[0])) or matched:
  candidates[i]=str(obj[0])
  print("OBJECT",json.dumps({"id":i,"name":obj[0],"sheets":matched[:15],"type":obj[1] if len(obj)>1 else None,"animations":[a[0] for a in obj[7] if isinstance(a,list) and a] if len(obj)>7 and isinstance(obj[7],list) else []},separators=(",",":"),ensure_ascii=False))
print("OBJECT_CANDIDATE_COUNT",len(candidates))
def walk(events,parents=()):
 for e in events if isinstance(events,list) else []:
  if not isinstance(e,list) or len(e)<8 or e[0]!=0:continue
  yield e,parents
  yield from walk(e[7],parents+(e[4],))
# Full event references to candidate objects in conditions/action targets.
hits=collections.defaultdict(list)
for sheet,events in p[6]:
 for e,parents in walk(events):
  cond=e[5] if isinstance(e[5],list) else []
  acts=e[6] if isinstance(e[6],list) else []
  touched=set(a[0] for a in cond+acts if isinstance(a,list) and a and isinstance(a[0],int) and a[0] in candidates)
  if not touched:continue
  for oid in touched:
   hits[oid].append({"sheet":sheet,"uid":e[4],"parents":parents[-5:],
     "conditions":cond,"actions":acts})
for oid,name in candidates.items():
 rec=hits.get(oid,[])
 print("OBJECT_EVENT_COUNT",oid,name,len(rec))
 for e in rec[:45]:
  print("OBJECT_EVENT",oid,json.dumps(e,ensure_ascii=False,separators=(",",":"))[:3600])

# Source bunker storage and airdrop event descendants; original sheet only.
TARGET_U={389950912936355,425016699358996,7711324247664447,
  353286644684551,114378063020685,884665878059776}
def all_events(nodes,ancestors=()):
 for e in nodes if isinstance(nodes,list) else []:
  if isinstance(e,list) and len(e)>=8 and e[0]==0:
   yield e,ancestors
   yield from all_events(e[7],ancestors+(e[4],))
for sheet,evs in p[6]:
 if sheet!='Game_events':continue
 for e,anc in all_events(evs):
  if e[4] in TARGET_U or any(x in TARGET_U for x in anc):
   print('SOURCE_BOX_FAMILY',json.dumps({'uid':e[4],'anc':anc[-8:],
     'conditions':e[5],'actions':e[6]},separators=(',',':'),
     ensure_ascii=False)[:5000])
