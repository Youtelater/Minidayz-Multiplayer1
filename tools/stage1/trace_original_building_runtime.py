#!/usr/bin/env python3
"""Root-original building interactivity beyond house Spawn and roof groups."""
from pathlib import Path
from collections import Counter
import json,sys
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tools/stage1"))
from audit_original_world_exhaustive import decode
p=decode((ROOT/"data.deobfuscated.js").read_bytes())
TARGET={302,306,311,312,313,321,322,323,324,333,346,347,348,349,
        1041,1048,1049}
SOURCE_CREATE=7736925111997576
def walk(nodes,ancestors=()):
 for e in nodes:
  if not isinstance(e,list) or len(e)<8 or e[0]!=0:continue
  yield e,ancestors
  yield from walk(e[7],ancestors+(e[4],))
records=[]
counts=Counter()
for sheet,events in p[6]:
 for event,parents in walk(events):
  if SOURCE_CREATE in parents or event[4]==SOURCE_CREATE:continue
  cond=event[5] if isinstance(event[5],list) else []
  acts=event[6] if isinstance(event[6],list) else []
  refs=set()
  for x in cond+acts:
   if isinstance(x,list) and x and isinstance(x[0],int) and x[0] in TARGET:
    refs.add(x[0])
  if not refs:continue
  groups=(sheet,event[4],tuple(sorted(refs)))
  counts.update(refs)
  records.append({
      "sheet":sheet,"uid":event[4],"parents":parents[-6:],
      "objects":sorted(refs),"conditions":cond,"actions":acts})
print("ROOT_OTHER_BUILDING_EVENT_COUNT",len(records),
      "DIRECT_OBJECT_REFERENCES",json.dumps(counts,sort_keys=True))
for item in records:
 if any(o in item["objects"] for o in (346,347,348,349)):
  print("BUILDING_RUNTIME_EVENT",json.dumps(item,separators=(",",":"),
                                              ensure_ascii=False)[:3800])
out=ROOT/"audit-output/world/original_building_runtime"
out.mkdir(parents=True,exist_ok=True)
(out/"original_building_runtime_event_sites.json").write_text(
 json.dumps(records,indent=2,ensure_ascii=False)+"\n",encoding="utf-8")
