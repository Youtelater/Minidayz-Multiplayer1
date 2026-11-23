#!/usr/bin/env python3
"""Diagnostic: original category-6 car-wreck parent/child SpawnAnotherObject chain."""
from pathlib import Path
import json,sys
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tools/stage1"))
from audit_original_world_exhaustive import decode
p=decode((ROOT/"data.deobfuscated.js").read_bytes())
def walk(nodes,anc=()):
 for e in nodes if isinstance(nodes,list) else ():
  if not isinstance(e,list) or len(e)<7 or e[0]!=0:continue
  yield e,anc
  yield from walk(e[7] if len(e)>7 else [],anc+(e,))
for sheet,events in p[6]:
 for e,anc in walk(events):
  a=e[6]
  creates=[x for x in a if isinstance(x,list) and len(x)>5
      and x[1]==73 and x[5] and x[5][0]==[4,241]]
  if not creates:continue
  print("CAR_NESTED_SPAWN",e[4],"parents",[(p[4],p[5],p[6]) for p in anc[-6:]],
        "conditions",json.dumps(e[5],separators=(",",":"))[:1200],
        "actions",json.dumps(a,separators=(",",":"))[:3800])
