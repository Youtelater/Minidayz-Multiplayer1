#!/usr/bin/env python3
"""Trace exact root Construct map-array function chains vs native generation."""
from pathlib import Path
import json,sys
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tools/stage1"))
from audit_original_world_exhaustive import decode,strings
p=decode((ROOT/"data.deobfuscated.js").read_bytes())
targets={
 "generate_array_locations":6938246402369311,
 "generate_array_roads_1":921061167763666,
 "generate_array_roads_2":484111970403648,
 "generate_array_tomap":1694439930581699,
 "generate_mapgen_toelements":7785053978136909,
 "generate_location":5017992314331459,
 "create_map_location":8627807469654645,
 "create_map_element":7736925111997576
}
def walk(nodes,anc=()):
 for e in nodes if isinstance(nodes,list) else ():
  if not isinstance(e,list) or len(e)<8:continue
  if e[0]==0:yield e,anc
  yield from walk(e[7],anc+(e[4],))
for sheet,events in p[6]:
 for e,anc in walk(events):
  if e[4] not in targets.values():continue
  label=next(k for k,v in targets.items() if v==e[4])
  print("ROOT_FUNCTION",label,"uid",e[4],"sheet",sheet,"parent",anc[-6:])
  print("ROOT_ENTRY_CONDS",json.dumps(e[5],ensure_ascii=False,separators=(',',':'))[:4200])
  print("ROOT_ENTRY_ACTIONS",json.dumps(e[6],ensure_ascii=False,separators=(',',':'))[:4200])
  print("ROOT_CHILD_COUNT",label,len(e[7]))
  def children(nodes,depth=0):
   for c in nodes if isinstance(nodes,list) else ():
    if not isinstance(c,list) or len(c)<8:continue
    if depth<2:
     print("ROOT_CHILD",label,depth,"uid",c[4],"kind",c[0],
       "conditions",json.dumps(c[5],ensure_ascii=False,separators=(',',':'))[:3100],
       "actions",json.dumps(c[6],ensure_ascii=False,separators=(',',':'))[:3100])
    children(c[7],depth+1)
  children(e[7])
