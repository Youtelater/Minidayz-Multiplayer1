#!/usr/bin/env python3
"""Source provenance audit for active category-15 LOOT_respawn marker placement.
Reads exclusively the root original data.deobfuscated.js extracted snapshot.
"""
import json,re
from pathlib import Path
p=Path(__file__).resolve().parents[2]/"data/stage1/construct_project_parsed.json"
project=json.loads(re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)","-",p.read_text(encoding="utf-8-sig")))["project"]
def walk(events, parents=()):
    for e in events if isinstance(events,list) else []:
        if not isinstance(e,list) or len(e)<7 or e[0]!=0:continue
        yield e, parents
        yield from walk(e[7] if len(e)>7 else [], (*parents,e))
for objid in (218,246,344,434,441,514):
    o=project[3][objid]
    print("OBJECT_DEF",objid,"prefix",json.dumps(o[:7],separators=(",",":"))[:450])
    anims=o[7] if len(o)>7 and isinstance(o[7],list) else []
    for anim in anims[:2]:
        print("OBJECT_ANIM",objid,anim[0],"prefix",json.dumps(anim[:7],separators=(",",":"))[:400])
        for i,fr in enumerate(anim[7] if len(anim)>7 and isinstance(anim[7],list) else []):
            if i>=2:break
            print("OBJECT_FRAME",objid,i,json.dumps(fr,separators=(",",":"))[:2000])
    print("OBJECT_TRAILING",objid,json.dumps(o[8:],separators=(",",":"))[:600])
sheets=[s for s in project[6] if isinstance(s,list) and len(s)>1]
roots=[e for s in sheets for e,_ in walk(s[1]) if e[4]==8627807469654645]
if len(roots)!=1:raise SystemExit(f"missing source loader {len(roots)}")
root=roots[0]
counts={344:0,434:0,246:0,218:0}
for e,anc in walk([root]):
    for i,a in enumerate(e[6]):
        if not isinstance(a,list) or len(a)<6 or not isinstance(a[5],list):continue
        obj = None
        if (a[:2]==[-1,94] or a[1]==73) and a[5] and a[5][0][0]==4 and isinstance(a[5][0][1],int):
            obj=a[5][0][1]
        if obj not in counts:continue
        counts[obj]+=1
        if obj!=344 and counts[obj]>10:continue
        print("ANCHOR_CREATE",obj,"event",e[4],"depth",len(anc),"action_index",i,
              "parents",[(n[4],json.dumps(n[5],separators=(",",":"))[:380]) for n in anc[-6:]],
              "conditions",json.dumps(e[5],separators=(",",":"))[:850],
              "action",json.dumps(a,separators=(",",":"))[:1500])
print("ANCHOR_CREATION_COUNTS",counts)


# Find exact 344/434 source creation sites across *all* source event sheets.
for sheet in sheets:
    for e,anc in walk(sheet[1]):
        for action in e[6]:
            if not isinstance(action,list) or len(action)<6 or not isinstance(action[5],list) or not action[5]:
                continue
            if not (action[:2]==[-1,94] or action[1]==73):continue
            created=action[5][0]
            if not isinstance(created,list) or created[0]!=4 or created[1] not in (344,434):continue
            print("GLOBAL_ANCHOR_CREATE",sheet[0],"obj",created[1],"event",e[4],
                "parent",[(p[4],json.dumps(p[5],separators=(",",":"))[:580]) for p in anc[-7:]],
                "cond",json.dumps(e[5],separators=(",",":"))[:900],
                "action",json.dumps(action,separators=(",",":"))[:1300])
