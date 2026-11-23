#!/usr/bin/env python3
"""Find original player/tree fade and z-order events using only root source."""
from pathlib import Path
from collections import Counter
import sys,json
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tools"/"stage1"))
from audit_original_world_exhaustive import decode
p=decode((ROOT/"data.deobfuscated.js").read_bytes())
trees={184,207,209,576,577,581}
families={}
for f in p[4]:
 if isinstance(f,list) and f and isinstance(f[0],int):
  members={x for x in f[1:] if isinstance(x,int)}
  if members.intersection(trees):
   families[f[0]]=sorted(members.intersection(trees))
print("TREE_FAMILIES",json.dumps(families))
targets=trees|set(families)
count=Counter()
events=[]
def walk(nodes,sheet,parents=()):
 for e in nodes if isinstance(nodes,list) else []:
  if not isinstance(e,list) or len(e)<7 or e[0]!=0:continue
  acts=[a for a in e[6] if isinstance(a,list) and len(a)>5 and a[0] in targets]
  if acts:
   for a in acts:count[(a[0],a[1])]+=1
   events.append((sheet,e[4],[p[4] for p in parents[-4:]],e[5],acts))
  if len(e)>7:walk(e[7],sheet,parents+(e,))
for sheet,ev in p[6]:
 walk(ev,sheet)
print("TREE_ACTION_OPCODE_COUNTS",json.dumps([(key[0],key[1],c) for key,c in count.most_common(90)]))
for sh,uid,anc,cond,acts in events[:140]:
 # All tree-related events with op codes aside from normal damage/angle
 print("TREE_ACTION_EVENT",json.dumps({"sheet":sh,"uid":uid,"parents":anc,
    "cond":cond,"acts":acts},ensure_ascii=False,separators=(",",":"))[:2600])

# Include the original tree fade/visibility action opcodes with enclosing
# event guards, including seven-field leaf Construct events. Treat no direct
# SetOpacity action as a source parity fact, not permission to invent one.
for op in (47,88,107,173,191,199):
 matching=[(obj,opcode,c) for (obj,opcode),c in count.items() if opcode==op]
 print("TREE_VISUAL_OPCODE",op,matching)
print("TREE_SOURCE_SEVEN_FIELD_EVENTS_INCLUDED",len(events))
