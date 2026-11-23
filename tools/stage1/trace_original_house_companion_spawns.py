#!/usr/bin/env python3
"""Root Construct audit of real house base/roof companion Spawn actions.

These are *not* guessed floor/roof pairings: this traces the exact original
event tree and nested actions that create/transform the secondary sprites.
"""
from pathlib import Path
import json,sys
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tools/stage1"))
from audit_original_world_exhaustive import decode
p=decode((ROOT/"data.deobfuscated.js").read_bytes())
BASE={302,306,311,312,313}
ROOFS={321,322,323,324,333}
all_roots=p[6]
events=[]
def scan(nodes,sheet,anc=(),depth=0):
 for e in nodes:
  if not isinstance(e,list) or len(e)<7 or e[0]!=0:continue
  uid=e[4]
  actions=e[6] if isinstance(e[6],list) else []
  hits=[]
  for a in actions:
   if not isinstance(a,list) or len(a)<2:continue
   params=a[9] if len(a)>9 and isinstance(a[9],list) else a[5] if len(a)>5 else []
   if a[0] in BASE|ROOFS or (a[1] in (73,94) and
       any(str(oid) in json.dumps(params,separators=(",",":")) for oid in ROOFS|BASE)):
    hits.append(a)
  if hits:
   rec={"sheet":sheet,"uid":uid,"parents":list(anc),
        "conditions":e[5],"actions":hits,"all_actions":actions}
   events.append(rec)
   print("HOUSE_COMPANION_SOURCE",json.dumps(rec,ensure_ascii=False,separators=(",",":"))[:8500])
  if len(e)>7:scan(e[7],sheet,anc+(uid,),depth+1)
for sheet,roots in all_roots:scan(roots,sheet)
out=ROOT/"audit-output/world";out.mkdir(parents=True,exist_ok=True)
(out/"original_house_companion_spawn_actions.json").write_text(
 json.dumps(events,ensure_ascii=False,indent=2)+"\n",encoding="utf-8")
print("ORIGINAL_HOUSE_COMPANION_EVENT_COUNT",len(events))
