#!/usr/bin/env python3
"""Locate every source tree hit/damage/fall branch and its parent guards.

Source: root data.deobfuscated.js (never the archived stage1 snapshot).
Print compact records for later native implementation, without inferring
numeric Construct condition/action semantics.
"""
import json
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
from audit_original_world_exhaustive import decode

ROOT = Path(__file__).resolve().parents[2]
TREE = {184, 207, 209, 576, 577, 581}
FALL_UIDS = {8821937621821993,2738244684146745,7838407830495979,4972733237327446,9345628732648648,4043087112077636}
p = decode((ROOT/"data.deobfuscated.js").read_bytes())
records = []
def visit(events, sheet, parents=(), path=()):
    for i, e in enumerate(events):
        if not isinstance(e,list) or len(e)<8 or e[0]!=0: continue
        now=path+(i,)
        conditions=e[5] if isinstance(e[5],list) else []
        actions=e[6] if isinstance(e[6],list) else []
        referenced=sorted({x[0] for x in conditions+actions
                           if isinstance(x,list) and x and isinstance(x[0],int) and x[0] in TREE})
        if referenced and (any("Hitting_obstacles" in repr(v) for v in parents)
                          or e[4] in FALL_UIDS):
            records.append({
                "sheet":sheet,"uid":e[4],"path":".".join(map(str,now)),
                "trees":referenced,
                "parents":[{"uid":x[4],"conditions":x[5][:2],
                            "actions":x[6][:2]} for x in parents[-4:]],
                "conditions":conditions, "actions":actions,
                "children":[x[4] for x in e[7] if isinstance(x,list) and len(x)>4 and x[0]==0]
            })
        visit(e[7],sheet,parents+(e,),now)
for sh in p[6]:visit(sh[1],sh[0])
out=ROOT/"audit-output/world"
out.mkdir(parents=True,exist_ok=True)
(out/"source_tree_hit_full_chains.json").write_text(json.dumps(records,indent=2,ensure_ascii=False)+"\n",encoding="utf-8")
# Fail source CI if any native hit UID/action pairing drifts from the root.
# A 345 parent collision guard is necessary but not sufficient to grant a hit.
expected={
    184:(7227858680569601,8821937621821993),
    207:(8754965976446553,2738244684146745),
    209:(5386206472407495,7838407830495979),
    576:(9193333425135665,4972733237327446),
    577:(4761103342332918,9345628732648648),
    581:(3531921662641626,4043087112077636),
}
for obj,(hit_uid,fall_uid) in expected.items():
    hit=[x for x in records if x["uid"]==hit_uid and obj in x["trees"]]
    fall=[x for x in records if x["uid"]==fall_uid and obj in x["trees"]]
    assert len(hit)==1 and len(fall)==1, (obj,hit_uid,fall_uid)
    assert any(isinstance(a,list) and a[:2]==[obj,87]
               for a in hit[0]["actions"]), (obj,"missing subtract action")
    assert any(isinstance(a,list) and a[:2]==[obj,72]
               for a in fall[0]["actions"]), (obj,"missing set-frame action")
    assert any(isinstance(c,list) and c[:2]==[obj,71]
               for c in fall[0]["conditions"]), (obj,"missing <=0 HP guard")
print("TREE_HIT_SOURCE_PARITY",len(expected),"species with hit/decrement/fall guards verified")
print("TREE_HIT_SOURCE_BRANCHES",len(records))
for row in records:
    if row["uid"] in FALL_UIDS or any(
        a[0] in TREE and a[1] in (72,137,163,88,78,64) for a in row["actions"]
        if isinstance(a,list) and len(a)>1
    ):
        print("TREE_BRANCH",json.dumps(row,separators=(",",":"),ensure_ascii=False)[:3500])
print("TREE_OBJECT_INSTANCE_DEFAULTS")
for ident in sorted(TREE):
    obj=p[3][ident]
    print("TREE_OBJ",ident,json.dumps(obj[:7],ensure_ascii=False,separators=(",",":"))[:550])
