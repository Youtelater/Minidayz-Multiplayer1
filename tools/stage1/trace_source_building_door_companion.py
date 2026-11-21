#!/usr/bin/env python3
"""Trace root-original door companion 348 and roof-family event chains.

Output is a *source evidence transcript*, not inferred gameplay semantics.
Only the root original's Construct records and parent event context count.
"""
import json
from pathlib import Path
from audit_original_world_exhaustive import ROOT,decode,strings

p=decode((ROOT/"data.deobfuscated.js").read_bytes())
objects=p[3]
TARGET_U={5688415260272274,6061166023614368,243854400993751,
          5249390718058362,3352196201835613,
          7755220865965526,9848716569903957}
print("SOURCE_OBJECT_348",json.dumps({
 "object_name":objects[348][0],
 "object_type":objects[348][1],
 "record_fields":[{"index":n,"value":item if n!=7 else
    [{"animation":a[0],"frames":len(a[7]) if len(a)>7 and isinstance(a[7],list) else 0}
      for a in item if isinstance(a,list)]}
     for n,item in enumerate(objects[348])],
},ensure_ascii=False,separators=(",",":"))[:15000])
rows=[]
def visit(entries,sheet,ancestors=(),groups=()):
 for entry in entries:
  if not isinstance(entry,list) or len(entry)<8 or entry[0]!=0:continue
  uid=entry[4]
  own_groups=groups
  if isinstance(entry[1],list) and len(entry[1])>1 and isinstance(entry[1][1],str):
   own_groups=groups+(entry[1][1],)
  cond=entry[5] if isinstance(entry[5],list) else []
  acts=entry[6] if isinstance(entry[6],list) else []
  door=uid in TARGET_U or 5688415260272274 in ancestors
  direct348=[a for a in acts if isinstance(a,list) and len(a)>1 and a[0]==348]
  creates348=[a for a in acts if isinstance(a,list) and len(a)>5 and a[:2]==[-1,94]
              and "348" in json.dumps(a[5],separators=(",",":"))]
  if door or direct348 or creates348:
   rows.append({"sheet":sheet,"uid":uid,"ancestors":ancestors,
    "group_path":own_groups,"door_chain":door,
    "conditions":cond if door or direct348 else [],
    "actions":acts if door or direct348 else [],
    "creates348":creates348})
  visit(entry[7],sheet,ancestors+(uid,),own_groups)
for name,entries in p[6]:visit(entries,name)
out=ROOT/"audit-output/world/building_source_348"
out.mkdir(parents=True,exist_ok=True)
(out/"door_companion_event_traces.json").write_text(
 json.dumps(rows,indent=2,ensure_ascii=False)+"\n",encoding="utf-8")
for row in rows:
 if not (row["door_chain"] or any(isinstance(a,list) and len(a)>1 and a[0]==348 for a in row["actions"])):
  continue
 print("SOURCE_DOOR_TRACE",json.dumps({
  "event_uid":row["uid"],"ancestor_uids":row["ancestors"][-5:],
  "group_path":row["group_path"],"condition_ids":[c[:2] for c in row["conditions"] if isinstance(c,list)],
  "actions":[{"object":a[0],"command":a[1],"params":a[5] if len(a)>5 else []}
             for a in row["actions"] if isinstance(a,list) and len(a)>1],
 },ensure_ascii=False,separators=(",",":"))[:5500])
print("SOURCE_348_TRACE_ROWS",len(rows))
print("SOURCE_348_DIRECT_ACTION_ROWS",sum(any(a[0]==348 for a in r["actions"] if isinstance(a,list) and a) for r in rows))
