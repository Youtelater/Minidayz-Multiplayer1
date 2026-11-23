#!/usr/bin/env python3
"""Root-only check of every 'Trees' category and original town element multiplicity."""
from pathlib import Path
import json,sys
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tools/stage1"))
from audit_original_world_exhaustive import decode
p=decode((ROOT/"data.deobfuscated.js").read_bytes())
IDS=(184,207,209,210,576,577,581,705,345)
for i in IDS:
 obj=p[3][i]
 frames=[]
 for a in (obj[7] if len(obj)>7 and isinstance(obj[7],list) else []):
  if len(a)>7 and isinstance(a[7],list):
   frames.extend([(x[0],x[2:6],x[7:9],x[9][:4] if len(x)>9 and isinstance(x[9],list) else None) for x in a[7] if isinstance(x,list) and len(x)>9])
 print("ROOT_TREE_OBJECT",i,json.dumps({"original_name":obj[0],"object_header":obj[:7],"frames":frames[:6]},separators=(",",":"),ensure_ascii=False)[:4400])
def walk(events,parents=()):
 for e in events if isinstance(events,list) else []:
  if not isinstance(e,list) or len(e)<7 or e[0]!=0:continue
  yield e,parents
  yield from walk(e[7] if len(e)>7 else [],parents+(e[4],))
uids={6033785432522481,7238212881044891,9881460187187066,1618691315170714,618606757222811,
5221960099677299,1639594650416124,7109935635557623,2584406677664733,2788700463962024,5413722554242966}
for sheet,events in p[6]:
 for e,anc in walk(events):
  if e[4] in uids:
   print("ROOT_TREE_BRANCH",e[4],sheet,
    "conditions",json.dumps(e[5],separators=(",",":"),ensure_ascii=False)[:1200],
    "actions",json.dumps(e[6],separators=(",",":"),ensure_ascii=False)[:2300],
    "children",json.dumps(e[7] if len(e)>7 else [],separators=(",",":"),ensure_ascii=False)[:3300])
# Structural assertion: a settlement template may write multiple element
# columns to Array525. Count directly from root original array operands.
target=5017992314331459
for sheet,events in p[6]:
 for e,anc in walk(events):
  if e[4]!=target:continue
  for branch in e[7]:
   if len(branch)<7 or branch[0]!=0:continue
   cols=set()
   writes=0
   for node,_ in walk([branch]):
    for a in node[6] if isinstance(node[6],list) else []:
     if isinstance(a,list) and a[:2]==[525,199] and len(a)>5 and a[5]:
      writes+=1
      expr=a[5][0]
      if isinstance(expr,list) and len(expr)>1 and isinstance(expr[1],list) and expr[1][:1]==[0]:
       cols.add(expr[1][1])
   print("ROOT_LOCATION_MULTIPLICITY",branch[4],"writes",writes,"literal_columns",sorted(cols)[:45])
   if writes>120: print("ROOT_MULTI_BUILDING_TEMPLATE",branch[4],"writes",writes)
  break

# Spot other original per-object SetFrame and companion-spawn actions that
# a primary-only native emit_source_rule would silently omit.
print("ROOT_PRIMARY_FRAME_AND_COMPANION_AUDIT")
seen=[]
for sheet,events in p[6]:
 for e,_ in walk(events):
  actions=e[6] if isinstance(e[6],list) else []
  main=[a[5][0][1] for a in actions if isinstance(a,list) and len(a)>5
        and a[:2]==[508,73] and a[5] and isinstance(a[5][0],list)
        and a[5][0][:1]==[4] and isinstance(a[5][0][1],int)]
  for obj in main:
   updates=[a for a in actions if isinstance(a,list) and len(a)>5
            and a[0]==obj and a[1] in (72,73,88,103,169)]
   if updates and not (obj in (184,207,209,210,576,577,581)):
    seen.append({"uid":e[4],"object":obj,"sheet":sheet,
        "updates":[[a[1],a[5][:4]] for a in updates][:12]})
print("ROOT_OTHER_PRIMARY_ACTION_COUNTS",len(seen))
for entry in seen[:120]:
 print("ROOT_OTHER_PRIMARY_ACTION",json.dumps(entry,separators=(",",":"))[:2100])
