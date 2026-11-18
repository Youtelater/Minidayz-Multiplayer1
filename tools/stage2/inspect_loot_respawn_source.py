#!/usr/bin/env python3
"""Temporary source-only diagnostic for wiring faithful loot respawn."""
import json,re,collections
from pathlib import Path
p=Path(__file__).resolve().parents[2]/"data/stage1/construct_project_parsed.json"
j=json.loads(re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)","-",p.read_text(encoding="utf-8-sig")))
def walk(xs,anc=()):
    for e in xs if isinstance(xs,list) else []:
        if not isinstance(e,list) or len(e)<7 or e[0]!=0: continue
        yield e,anc
        yield from walk(e[7] if len(e)>7 else [],(*anc,e))
sheets=j["project"][6]
for sheet in sheets:
    if not isinstance(sheet,list) or len(sheet)<2:continue
    for e,anc in walk(sheet[1]):
        if e[4] in (2026232979865106,3772264929481614,673982235644371):
            print("ROOT",sheet[0],"uid",e[4],"anc",[(a[4],str(a[5])[:230]) for a in anc[-8:]])
            print("  CONDS",json.dumps(e[5],separators=(",",":"))[:1800])
            print("  ACTS",json.dumps(e[6],separators=(",",":"))[:2200])
            for child,_ in list(walk(e[7] if len(e)>7 else []))[:14]:
                print("  CHILD",child[4],"COND",json.dumps(child[5],separators=(",",":"))[:550],"ACTS",json.dumps(child[6],separators=(",",":"))[:650])
        actions=e[6]
        if not isinstance(actions,list):continue
        for a in actions:
            if not isinstance(a,list) or len(a)<6 or a[:2]!=[189,69]:continue
            name=json.dumps(a[5],separators=(",",":"))[:120]
            if any(w in name for w in ("Global_respawn","Trigger_spawn")):
                print("CALL",sheet[0],"uid",e[4],"name",name,"anc",[(n[4],str(n[5])[:220]) for n in anc[-6:]],"conditions",json.dumps(e[5])[:1400])
print("DIAGNOSTIC_END")
