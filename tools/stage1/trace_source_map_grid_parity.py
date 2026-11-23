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
  if not isinstance(e,list) or len(e)<7:continue
  if e[0]==0:yield e,anc
  if len(e)>7:yield from walk(e[7],anc+(e[4],))
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
    if not isinstance(c,list) or len(c)<7:continue
    if depth<2:
     print("ROOT_CHILD",label,depth,"uid",c[4],"kind",c[0],
       "conditions",json.dumps(c[5],ensure_ascii=False,separators=(',',':'))[:3100],
       "actions",json.dumps(c[6],ensure_ascii=False,separators=(',',':'))[:3100])
    if len(c)>7:children(c[7],depth+1)
  children(e[7])

print("ROOT_ORIGINAL_MAPGEN_LAYOUT")
for layout in p[5]:
 if layout[0]!="Map":continue
 for layer in layout[6]:
  for inst in layer[14] if len(layer)>14 and isinstance(layer[14],list) else []:
   if len(inst)>2 and inst[1] in (514,518,525,230):
    print("MAPGEN_TEMPLATE",inst[1],layer[0],json.dumps(inst,separators=(',',':'))[:2200])
targets_set=set(targets.values())
for sheet,events in p[6]:
 for e,anc in walk(events):
  if e[4] not in targets_set:continue
  name=next(k for k,v in targets.items() if v==e[4])
  if name in ("generate_mapgen_toelements","generate_array_tomap"):
   print("ROOT_CHILD_RAW",name,json.dumps(e[7][0],ensure_ascii=False,separators=(',',':'))[:16000])
  relevant=[]
  for node,nanc in walk(e[7],(e[4],)):
   for action in node[6] if isinstance(node[6],list) else []:
    if not isinstance(action,list) or len(action)<6:continue
    if action[0] in (514,518,525) or (action[0]==-1 and action[1] in (94,41,122)) or (action[0]==189 and action[1]==69):
     relevant.append((node[4],tuple(nanc[-4:]),action[0],action[1],action[5]))
  print("ROOT_ACTION_SUMMARY",name,len(relevant))
  for uid,ancestor,obj,cmd,params in relevant[:85]:
   print("ROOT_ACTION",name,uid,"parent",ancestor,"obj",obj,"cmd",cmd,"params",json.dumps(params,ensure_ascii=False,separators=(',',':'))[:3200])

for sheet,events in p[6]:
 for e,_ in walk(events):
  if e[4] not in (921061167763666,484111970403648):continue
  name="roads_1" if e[4]==921061167763666 else "roads_2"
  def all_conditions(nodes,depth=0):
   for node in nodes if isinstance(nodes,list) else ():
    if not isinstance(node,list) or len(node)<7 or node[0]!=0:continue
    conds=[c for c in node[5] if isinstance(c,list) and len(c)>9 and
      (c[0]==518 or (c[0]==-1 and ("518" in repr(c) or "anotherX" in repr(c))))]
    acts=[a for a in node[6] if isinstance(a,list) and len(a)>5 and
      (a[0]==518 or a[0]==514)]
    if conds or acts:
     print("ROOT_ROAD_DETAIL",name,depth,node[4],
      "conds",json.dumps(conds,separators=(",",":"))[:4200],
      "actions",json.dumps(acts,separators=(",",":"))[:4200])
    if len(node)>7:all_conditions(node[7],depth+1)
  all_conditions(e[7])
