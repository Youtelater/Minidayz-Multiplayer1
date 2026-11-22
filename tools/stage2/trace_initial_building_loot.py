#!/usr/bin/env python3
"""Trace initial Trigger_spawn / Spawn_drop direct original event ancestry."""
from pathlib import Path
from collections import Counter,defaultdict
import json,sys
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tools"/"stage1"))
from audit_original_world_exhaustive import decode
p=decode((ROOT/"data.deobfuscated.js").read_bytes())
target=7952760861768009
def walk(events,anc=()):
 for e in events if isinstance(events,list) else ():
  if isinstance(e,list) and len(e)>7 and e[0]==0:
   yield e,anc
   yield from walk(e[7],anc+(e,))
def isdrop(a):
 return isinstance(a,list) and len(a)>5 and a[:2]==[189,69] and a[5] and a[5][0]==[1,[2,"Spawn_drop"]]
def extract(cond):
 try:
  if cond[:2]==[218,71] and cond[9][:2]==[[10,0],[8,0]]:
   return str(cond[9][2])
  if cond[:2]==[-1,46] and cond[9][0][0]==11:
   return cond[9][0][1]+": "+str(cond[9][1:])
 except Exception:pass
 return None
for sh,events in p[6]:
 for e,anc in walk(events):
  if e[4]!=target:continue
  print("SOURCE_TRIGGER_ROOT",sh,json.dumps({"uid":e[4],"conds":e[5],"actions":e[6],"parent_uids":[x[4] for x in anc]},separators=(',',':'))[:6000])
  print("SOURCE_TRIGGER_CHILDREN",len(e[7]),[c[4] for c in e[7][:30] if isinstance(c,list) and len(c)>4])
  descendants=list(walk([e]))
  print("SOURCE_TRIGGER_DESCENDANTS",len(descendants),[(n[4],len(n[6]),[a[:2] for a in n[6] if isinstance(a,list)]) for n,_ in descendants[:18]])
  counts=Counter()
  patterns=defaultdict(list)
  for node,parents in walk([e]):
   calls=[a for a in node[6] if isdrop(a)]
   if not calls:continue
   chain=parents+(node,)
   groups=[]
   for n in chain:
    for c in n[5]:
     if isinstance(c,list) and c:
      groups.append((n[4],c[:2],extract(c),c[9] if len(c)>9 else []))
   marker=[g[2] for g in groups if g[1]==[218,71]]
   branch=marker[-1] if marker else '(none)'
   counts[branch]+=len(calls)
   for call in calls:
    patterns[branch].append({"event":node[4],"action":call[3],
     "params":call[5][1],"guard_types":[g[1] for g in groups],
     "guards":groups})
  print("SOURCE_TRIGGER_COUNTS",json.dumps(counts,ensure_ascii=False))
  for cat,records in sorted(patterns.items(),key=lambda x:x[0]):
   print("SOURCE_CATEGORY",cat,"count",len(records))
   for rec in records[:6]:
    print("SOURCE_CATEGORY_RECORD",cat,json.dumps(rec,separators=(',',':'),ensure_ascii=False)[:8000])
  break
