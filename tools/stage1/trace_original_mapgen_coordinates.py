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
  if not isinstance(e,list) or len(e)<7:continue
  gn=groups+((e[1][1],) if isinstance(e[1],list) and len(e[1])>1 and isinstance(e[1][1],str) else ())
  if e[0]==0:yield e,anc,gn
  if len(e)>7:yield from walk(e[7],anc+((e[4],) if e[0]==0 else ()),gn)
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

from collections import Counter
print("MAPGEN_DIRECT_ACTIONS")
all_actions=[]
for sheet,events in p[6]:
 for e,anc,groups in walk(events):
  for a in e[6] if isinstance(e[6],list) else ():
   if not isinstance(a,list) or len(a)<6:continue
   if a[0] not in (514,508) and not (a[:2]==[-1,94]):continue
   all_actions.append((sheet,e[4],anc,groups,a))
print("MAPGEN_ACTION_COMMAND_HISTOGRAM",
 json.dumps({str(k):v for k,v in sorted(Counter((a[0],a[1]) for _,_,_,_,a in all_actions).items())},
  ensure_ascii=False))
for sheet,uid,anc,groups,a in all_actions:
 interesting=((a[0]==514 and a[1]!=88) or
   (a[0]==508 and a[1] in (73,88,132,137,163)) or
   (a[:2]==[-1,94] and len(a[5])>0 and a[5][0] in ([4,514],[4,508])))
 if not interesting:continue
 print("MAPGEN_POSITION_ACTION",json.dumps({"uid":uid,"sheet":sheet,
    "anc":anc[-5:],"groups":groups[-4:],"action":a},separators=(",",":"))[:4600])

print("ORIGINAL_MAP_FUNCTION_CALLS")
wants={"generate_array_locations","generate_array_roads_1","generate_array_roads_2","generate_array_tomap","generate_mapgen_toelements","generate_location","create_map_location","create_map_element"}
for sheet,events in p[6]:
 for e,anc,groups in walk(events):
  for a in e[6] if isinstance(e[6],list) else ():
   if not isinstance(a,list) or len(a)<6 or a[:2]!=[189,69]:continue
   vals=a[5]
   if not vals or not isinstance(vals[0],list) or len(vals[0])<2:continue
   fn=vals[0][1]
   if not isinstance(fn,list) or len(fn)<2 or fn[0]!=2 or fn[1] not in wants:continue
   print("MAP_FN_CALL",fn[1],"sheet",sheet,"uid",e[4],"ancestor",anc[-6:],
     "conditions",json.dumps(e[5],separators=(',',':'))[:2600],
     "actions",json.dumps(e[6],separators=(',',':'))[:3400])
print("ORIGINAL_MAPGEN_POSITION_ANCESTRY")
positions={1131020651387518,156278692855564}
for sheet,events in p[6]:
 for e,anc,groups in walk(events):
  if e[4] not in positions:continue
  print("MAPGEN_POS_UID",e[4],"anc",anc)
  for wanted in anc:
   for x,xc,xg in walk(events):
    if x[4]==wanted:
     print("MAPGEN_POS_PARENT",wanted,"conditions",json.dumps(x[5],separators=(',',':'))[:3000],
       "actions",json.dumps(x[6],separators=(',',':'))[:3000])
     break
  print("MAPGEN_POS_EVENT",json.dumps(e,separators=(',',':'))[:10000])
print("MAPGEN_SPRITE_INSTANCES")
for lay in p[5]:
 if lay[0]!="Map":continue
 for layer in lay[6]:
  for i in layer[14] if len(layer)>14 and isinstance(layer[14],list) else ():
   if len(i)>2 and i[1] in (514,508,518,525):
    print("MAPGEN_SPRITE",i[1],layer[0],json.dumps(i,separators=(',',':'))[:4200])

print("MAPGEN_ORIGINAL_TILE_COORD_TRANSFORMS")
for layout in p[5]:
 if layout[0] not in ("Map","TD"):continue
 for layer in layout[6]:
  for inst in layer[14] if len(layer)>14 and isinstance(layer[14],list) else ():
   if len(inst)<6 or inst[1] not in (1066,514,508):continue
   print("TILE_COORD_SOURCE_INSTANCE",layout[0],layer[0],inst[1],
     "transform",json.dumps(inst[0],separators=(",",":"))[:800],
     "attributes",json.dumps(inst[5],separators=(",",":"))[:800])
print("TILE_COORD_OBJECT_TYPE",1066,p[3][1066][0],p[3][1066][1])
for sheet,events in p[6]:
 for e,anc,groups in walk(events):
  for a in e[6] if isinstance(e[6],list) else ():
   if isinstance(a,list) and len(a)>5 and a[0]==1066 and a[1] in (47,103,189,206,210,212):
    print("TILE_COORD_MUTATION",e[4],json.dumps(a,separators=(",",":"))[:1500])
