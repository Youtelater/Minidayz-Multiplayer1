#!/usr/bin/env python3
"""Trace root Construct road-array loop bounds, anchors and split-map semantics."""
from pathlib import Path
from collections import Counter
import json,sys
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tools/stage1"))
from audit_original_world_exhaustive import decode
p=decode((ROOT/"data.deobfuscated.js").read_bytes())
def walk(nodes,parents=()):
 for e in nodes if isinstance(nodes,list) else ():
  if not isinstance(e,list) or len(e)<7:continue
  if e[0]==0:yield e,parents
  if e[0]==0 and len(e)>7 and isinstance(e[7],list):
   yield from walk(e[7],parents+(e[4],))
uids={"horizontal":921061167763666,"vertical":484111970403648}
for name,uid in uids.items():
 e=next(e for _,events in p[6] for e,_ in walk(events) if e[4]==uid)
 guards=[]
 for node,anc in walk(e[7],(uid,)):
  for c in node[5] if isinstance(node[5],list) else []:
   if not isinstance(c,list) or len(c)<=9 or c[:2]!=[518,203]:continue
   guards.append((node[4],anc,c[9]))
 codes=sorted({g[2][3][1][1] for g in guards
   if len(g[2])>=4 and g[2][3][:1]==[7] and
   isinstance(g[2][3][1],list) and len(g[2][3][1])>1 and g[2][3][1][0]==0})
 print("ROAD_ROOT_ANCHORS",name,codes)
 assert codes==[6,9,11,15,103],(name,codes)
 top=e[7]
 repeats=[n for n in top if isinstance(n,list) and len(n)>5 and n[0]==0 and
  any(isinstance(c,list) and c[:2]==[-1,123] for c in n[5])]
 for n in repeats:
  cc=[c[9] for c in n[5] if isinstance(c,list) and c[:2]==[-1,123]]
  print("ROAD_ROOT_REPEAT",name,n[4],json.dumps(cc,separators=(',',':')))
print("ROAD_SOURCE_GRID_VARIABLE_DECLARATIONS")
for sheet,events in p[6]:
 def variables(nodes,anc=()):
  for n in nodes if isinstance(nodes,list) else ():
   if not isinstance(n,list) or len(n)<7:continue
   if n[0]==1 and n[1] in ("Map_size_X","Map_size_Y"):
    print("ROAD_GRID_DECL",sheet,n[:7],"ancestors",anc[-5:])
   elif n[0]==0:
    for a in n[6]:
     if (isinstance(a,list) and len(a)>5 and a[:2]==[-1,41] and a[5]
         and len(a[5][0])>1 and a[5][0] in ([11,"Map_size_X"],[11,"Map_size_Y"])):
      print("ROAD_GRID_SET_X",sheet,n[4],json.dumps(a[5],separators=(',',':')))
    if len(n)>7 and isinstance(n[7],list):
     variables(n[7],anc+(n[4],))
 variables(events)
# Root Map-size defaults are 16; challenge/test flags may explicitly
# set them to 3, then restore 16. The ordinary 16x33 map still uses the
# original 16-row horizontal pass and second-half vertical Y+17 pass.
world_source=(ROOT/"source/world.cpp").read_text(encoding="utf-8")
assert "for (int row = 0; row < grid.width() && row < grid.height(); ++row)" in world_source
assert "if (level >= 4) add_vertical_roads(17);" in world_source
print("PASS original road array anchor source extraction and split-map axis bounds")
