#!/usr/bin/env python3
"""Audit original Construct player movement and animation events by source UID."""
from pathlib import Path
import sys,json,collections
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tools/stage1"))
from audit_original_world_exhaustive import decode
p=decode((ROOT/"data.deobfuscated.js").read_bytes())
def walk(nodes,parents=(),groups=()):
 for node in nodes if isinstance(nodes,list) else ():
  if not isinstance(node,list) or len(node)<7:continue
  if node[0]!=0:
   if len(node)>7 and isinstance(node[7],list):yield from walk(node[7],parents,groups)
   continue
  h=node[1]
  name=h[1] if isinstance(h,list) and len(h)>1 and isinstance(h[1],str) else None
  path=groups+((name,) if name else ())
  yield node,parents,path
  if len(node)>7:yield from walk(node[7],parents+(node[4],),path)
sheets=dict(p[6])
match=0
for e,parents,groups in walk(sheets["Game_events"]):
 actions=e[6] if isinstance(e[6],list) else []
 conds=e[5] if isinstance(e[5],list) else []
 for a in actions:
  if not isinstance(a,list) or len(a)<6:continue
  data=json.dumps(a[5],ensure_ascii=False)
  likely=(a[3]==156884066447061 or
    (a[0] in (193,679,-1) and any(word in data.lower()
      for word in ("altmove","maxspeed","speed","run","sprint","moving","movement"))) or
    ("AltMove" in data and a[0] in (193,679,-1)))
  if not likely:continue
  print("SOURCE_PLAYER_ACTION",json.dumps({"action_uid":a[3],"event_uid":e[4],
    "sheet":"Game_events","groups":groups[-5:],"parents":parents[-5:],
    "object":a[0],"cmd":a[1],"params":a[5],"guards":conds},separators=(",",":"))[:4800])
  match+=1
print("PLAYER_SOURCE_MATCHED_ACTIONS",match)
print("PLAYER_SOURCE_ANIMATION_INFO")
for oid in (193,679):
 obj=p[3][oid]
 print("PLAYER_OBJECT",oid,"name",obj[0],"behaviors",str(obj[4])[:1200],
   "effects",str(obj[5])[:500])
 for i,a in enumerate(obj[7] if len(obj)>7 and isinstance(obj[7],list) else []):
  if not isinstance(a,list) or len(a)<8:continue
  print("PLAYER_ORIGINAL_ANIMATION",oid,i,a[0],"speed",a[2],"loop",a[4],
     "frames",len(a[7]))
