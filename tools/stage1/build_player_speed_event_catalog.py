#!/usr/bin/env python3
"""Exact root-Construct PlayerBase193 AltMove modifier event catalog.

Indexes every action writing source instance variables 3..8 and every
AltMove MaxSpeed action, retaining original event path, inherited guards
and action order. This is source evidence, NOT native gameplay parity.
Never substitute event UID alone for original nested event paths.
"""
from collections import Counter
from hashlib import sha256
from pathlib import Path
import json
import sys

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tools/stage1"))
from audit_original_world_exhaustive import decode

raw=(ROOT/"data.deobfuscated.js").read_bytes()
project=decode(raw)
records=[]
counts=Counter()
def walk(nodes,sheet,ancestry=(),indices=(),groups=()):
    for index,event in enumerate(nodes if isinstance(nodes,list) else ()):
        if not isinstance(event,list) or len(event)<7 or event[0]!=0:
            continue
        path=indices+(index,)
        group=(event[1][1] if isinstance(event[1],list) and
               len(event[1])>1 and isinstance(event[1][1],str) else None)
        chain=groups+((group,) if group else ())
        acts=event[6] if isinstance(event[6],list) else ()
        for action_index,action in enumerate(acts):
            if not isinstance(action,list) or len(action)<6 or action[0]!=193:
                continue
            variable=None
            if action[1] in (88,89,90) and isinstance(action[5],list):
                for row in action[5]:
                    if isinstance(row,list) and row[:1]==[10] and len(row)>1:
                        variable=row[1]
                        break
            speed=(action[1]==253 and action[2]=="AltMove")
            if not speed and variable not in (3,4,5,6,7,8):
                continue
            key=("AltMove_MaxSpeed" if speed else f"instance_var{variable}")
            counts[key]+=1
            records.append({
                "sheet":sheet,"event_uid":event[4],
                "event_path":sheet+":"+".".join(map(str,path)),
                "action_index":action_index,"action_uid":action[3],
                "source_group_chain":list(chain),
                "ancestor_events":[{"uid":p[4],"conditions":p[5],
                                    "disabled":p[3] if len(p)>3 else None}
                                   for p in ancestry],
                "conditions":event[5],"action":action,
                "target":key,
            })
        if len(event)>7 and isinstance(event[7],list):
            yield_from=event[7]
            walk(yield_from,sheet,ancestry+(event,),path,chain)
for sheet,events in project[6]:
    walk(events,sheet)

required={
    "AltMove_MaxSpeed":{156884066447061},
    "instance_var4":{849089065891973,774129289252485},
    "instance_var5":{635135793841138},
    "instance_var6":{674044693781383,807379728796959},
    "instance_var7":{634838120087127},
}
for target,action_uids in required.items():
    seen={r["action_uid"] for r in records if r["target"]==target}
    if not action_uids.issubset(seen):
        raise SystemExit(f"Original source speed event missing for {target}: "
                         f"{sorted(action_uids-seen)}")
# Original var8 is in the MaxSpeed expression, but no original event
# writes it; accidental speed bonuses based on equipment are invented.
if counts["instance_var8"]:
    raise SystemExit("Original var8 writers changed; investigate before porting")
assert counts["AltMove_MaxSpeed"]>=1
assert counts["instance_var6"]>=2 and counts["instance_var7"]>=4
assert len(records)==sum(counts.values())
report={
    "provenance":"ROOT data.deobfuscated.js / all Construct event sheets",
    "source_sha256":sha256(raw).hexdigest(),
    "status":"root event inventory only; unimplemented gameplay modifiers remain open",
    "totals":{"modifier_action_occurrences":len(records),
              "by_target":dict(sorted(counts.items()))},
    "records":records,
}
dest=ROOT/"audit-output/player/original_player_speed_events.json"
dest.parent.mkdir(parents=True,exist_ok=True)
dest.write_text(json.dumps(report,ensure_ascii=False,indent=2)+"\n",encoding="utf-8")
print("ORIGINAL_PLAYER_SPEED_EVENT_CATALOG",json.dumps(report["totals"],sort_keys=True))
print("SOURCE_EVENT_CATALOG",dest)
