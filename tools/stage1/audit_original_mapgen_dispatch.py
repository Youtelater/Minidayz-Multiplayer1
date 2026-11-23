#!/usr/bin/env python3
"""Record original Construct map-tile to Mapgen Sprite514 dispatch, without guessing source semantics."""
from pathlib import Path
from collections import Counter,defaultdict
import sys,json
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tools/stage1"))
from audit_original_world_exhaustive import decode
project=decode((ROOT/"data.deobfuscated.js").read_bytes())
FUNC_UID=1694439930581699
MAPGEN_FUNC_UID=7785053978136909
def descendants(seq,anc=()):
 for n in seq if isinstance(seq,list) else ():
  if not isinstance(n,list) or not n:continue
  if n[0]==0 and len(n)>=7:
   yield n,anc
   if len(n)>7:yield from descendants(n[7],anc+(n[4],))
all_ev=[(sheet,n,anc) for sheet,evs in project[6] for n,anc in descendants(evs)]
creates=[]
for sheet,e,anc in all_ev:
 for a in e[6] if isinstance(e[6],list) else ():
  if not isinstance(a,list) or len(a)<6:continue
  if a[:2]==[-1,94]:
   item=a[5][0] if a[5] else None
   creates.append((sheet,e[4],anc[-4:],item,a[5]))
print("ROOT_CREATE_TARGETS",len(creates))
targets=Counter(json.dumps(i[3],separators=(",",":")) for i in creates)
print("ROOT_CREATE_TARGET_COUNTS",json.dumps(targets.most_common(100),separators=(",",":"))[:8500])
print("ROOT_CREATE_MAPGEN",json.dumps([c for c in creates if "514" in json.dumps(c[4][0])],separators=(",",":"))[:16000])
for fn_uid in (FUNC_UID,MAPGEN_FUNC_UID):
 sheet,e,anc=next((sh,n,a) for sh,n,a in all_ev if n[4]==fn_uid)
 print("MAP_DISPATCH_FUNCTION",fn_uid,"root",json.dumps(e[:7],separators=(",",":"))[:3500])
 count=Counter()
 commands=Counter()
 parent_nodes=[]
 def tree(nodes,depth=0):
  for child in nodes if isinstance(nodes,list) else ():
   if not isinstance(child,list) or not child:continue
   count[child[0]]+=1
   if child[0]==0 and len(child)>=7:
    for a in child[6] if isinstance(child[6],list) else ():
     if isinstance(a,list) and len(a)>1:commands[(a[0],a[1])]+=1
    if depth<=3:
     parent_nodes.append({"depth":depth,"uid":child[4],
        "cond":child[5], "acts":child[6]})
    if len(child)>7:tree(child[7],depth+1)
   elif child[0]==1:
    if depth<=2: parent_nodes.append({"depth":depth,"variable":child[:10]})
 tree(e[7])
 print("MAP_DISPATCH_TYPES",fn_uid,dict(count))
 print("MAP_DISPATCH_ACTION_COUNTS",fn_uid,str(commands.most_common(50)))
 for n in parent_nodes[:90]:
  print("MAP_DISPATCH_NODE",fn_uid,json.dumps(n,separators=(",",":"))[:4000])
print("MAPGEN_DIRECT_REFERENCES")
for sheet,e,anc in all_ev:
 for a in e[6] if isinstance(e[6],list) else ():
  if isinstance(a,list) and len(a)>5 and a[0] in (514,518) and a[1] in (53,73,88,192,199,200,210,211):
   if e[4] in (1131020651387518,156278692855564):continue
   print("MAPGEN_REFERENCE",sheet,e[4],anc[-4:],json.dumps(a,separators=(",",":"))[:1700])

print("ORIGINAL_SPRITE345_BEHAVIORS",json.dumps(project[3][345][8],ensure_ascii=False,separators=(",",":"))[:4000])
print("ORIGINAL_SPRITE345_NAME",project[3][345][0])
