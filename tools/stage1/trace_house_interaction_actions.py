#!/usr/bin/env python3
"""Identify exact native house post-Spawn gameplay requirements in root source."""
from pathlib import Path
from collections import Counter,defaultdict
import json,sys
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tools/stage1"))
from audit_original_world_exhaustive import decode
p=decode((ROOT/"data.deobfuscated.js").read_bytes())
SOURCE=7736925111997576
def walk(nodes,parents=()):
 for e in nodes:
  if not isinstance(e,list) or len(e)<7 or e[0]!=0:continue
  yield e,parents
  if len(e)>7:yield from walk(e[7],parents+(e[4],))
counts=Counter()
traces=defaultdict(list)
for sheet,events in p[6]:
 if sheet!="Game_events":continue
 for event,ancestors in walk(events):
  if SOURCE not in ancestors:continue
  acts=event[6] if isinstance(event[6],list) else []
  spawned=set()
  for action in acts:
   if not isinstance(action,list) or len(action)<6:continue
   if action[1]==73 and action[0]!=508:
    target=action[5][0][1] if action[5] and len(action[5][0])>1 else None
    # Construct literal expression nodes are [type, value].
    if isinstance(target,list) and len(target)>1 and isinstance(target[1],int):
     spawned.add(target[1])
    elif isinstance(target,int):
     spawned.add(target)
   elif action[0] in spawned:
    key=(action[0],action[1])
    counts[key]+=1
    if len(traces[key])<2:
     traces[key].append({"event_uid":event[4],"action":action[5]})
print("ORIGINAL_HOUSE_POST_SPAWN_ACTIONS")
for (object_id,command),count in counts.most_common():
 print("HOUSE_ACTION",json.dumps({"object":object_id,
  "name":p[3][object_id][0],"command":command,"count":count,
  "examples":traces[(object_id,command)]},ensure_ascii=False,separators=(",",":"))[:1600])
print("TOTAL_AUTHORED_HOUSE_POST_SPAWN_ACTIONS",sum(counts.values()))
