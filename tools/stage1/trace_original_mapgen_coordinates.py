#!/usr/bin/env python3
"""Pinpoint original Mapgen 514 event-based positions, array reads, and creation."""
from pathlib import Path
import sys,json
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tools/stage1"))
from audit_original_world_exhaustive import decode
p=decode((ROOT/"data.deobfuscated.js").read_bytes())
target_functions={"generate_array_tomap","generate_mapgen_toelements","create_map_location","generate_location"}
def walk(nodes,anc=(),groups=()):
 for e in nodes if isinstance(nodes,list) else ():
  if not isinstance(e,list) or len(e)<8:continue
  gn=groups+((e[1][1],) if isinstance(e[1],list) and len(e[1])>1 and isinstance(e[1][1],str) else ())
  if e[0]==0:yield e,anc,gn
  yield from walk(e[7],anc+((e[4],) if e[0]==0 else ()),gn)
hits=[]
for sheet,events in p[6]:
 for e,anc,groups in walk(events):
  for a in e[6] if isinstance(e[6],list) else ():
   if not isinstance(a,list) or len(a)<6:continue
   special=(a[0]==-1 and a[1]==94 and a[5] and a[5][0]==[4,514])
   if special or (a[0]==514 and a[1] in (65,66,73,88,95,114,132,137,163,189,200,208)):
    hits.append({"uid":e[4],"group":groups[-5:],"sheet":sheet,"anc":anc[-5:],
        "action":a,"conditions":e[5],"created_mapgen":special})
print("MAPGEN_SOURCE_ACTION_COUNT",len(hits))
for hit in hits[:140]:
 print("MAPGEN_SOURCE_ACTION",json.dumps(hit,ensure_ascii=False,separators=(',',':'))[:5400])
