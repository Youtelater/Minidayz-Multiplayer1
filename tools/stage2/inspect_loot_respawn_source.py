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
# Compact raw original function trees and source marker payloads.
for sheet in sheets:
    if not isinstance(sheet,list) or len(sheet)<2: continue
    for e,anc in walk(sheet[1]):
        if e[4] not in (388188595587361,7952760861768009):continue
        entries=list(walk([e]))
        print("FUNCTION_TREE",e[4],"NODE_COUNT",len(entries))
        for node,parents in entries:
            cs=json.dumps(node[5],separators=(",",":"))
            ac=json.dumps(node[6],separators=(",",":"))
            depth=len(parents)
            if depth<=2 or '"Trigger_spawn"' in ac or '"Spawn_drop"' in ac or '"Global_respawn"' in ac or (node[5] and any(q[0]==218 for q in node[5] if isinstance(q,list))):
                print("FN_NODE",e[4],"depth",depth,"uid",node[4],"conditions",cs[:1400],"actions",ac[:1800])
for layout in j["project"][5]:
    if not isinstance(layout,list) or len(layout)<7:continue
    if layout[0]!="Map":continue
    count=0
    for layer in layout[6]:
        if not isinstance(layer,list) or len(layer)<15:continue
        for inst in layer[14] if isinstance(layer[14],list) else []:
            if isinstance(inst,list) and len(inst)>2 and inst[1]==218:
                if count<8:print("SOURCE_MARKER_INSTANCE","layer",layer[0],"record",json.dumps(inst,separators=(",",":"))[:1600])
                count+=1
    print("SOURCE_MARKER_INSTANCE_COUNT",count)

# Source tree audit for active marker creation and the drop entity constructor.
for sheet in sheets:
    if not isinstance(sheet,list) or len(sheet)<2:continue
    for e,anc in walk(sheet[1]):
        cs=e[5]
        if not any(q[0:2]==[189,40] and q[9][0]==[1,[2,"create_map_location"]]
                   for q in cs if isinstance(q,list) and len(q)>9):
            continue
        nodes=list(walk([e]))
        print("LOCATION_LOADER_ROOT",sheet[0],e[4],"descendants",len(nodes))
        from collections import Counter
        cc,ac=Counter(),Counter()
        for node,anc2 in nodes:
            for q in node[5]:cc[tuple(q[:2])]+=1
            for a in node[6]:ac[tuple(a[:2])]+=1
            if any(a[0]==218 or
                   (a[0]==-1 and a[1]==94 and any("218" in str(v) for v in a[5])) or
                   (a[0]==189 and "Trigger_spawn" in str(a[5]))
                   for a in node[6]):
                print("MARKER_SOURCE_EVENT",node[4],"parent",[x[4] for x in anc2[-5:]],"conds",json.dumps(node[5],separators=(",",":"))[:1200],"actions",json.dumps(node[6],separators=(",",":"))[:1700])
        print("LOCATION_LOADER_OPS",cc.most_common(30),ac.most_common(30))
for sheet in sheets:
    if not isinstance(sheet,list) or len(sheet)<2:continue
    for e,anc in walk(sheet[1]):
        if not any(q[0:2]==[189,40] and q[9][0]==[1,[2,"Spawn_drop"]]
                   for q in e[5] if isinstance(q,list) and len(q)>9):
            continue
        nodes=list(walk([e]))
        print("SPAWN_DROP_SOURCE",e[4],"n",len(nodes))
        for node,pa in nodes[:45]:
            print("DROP_NODE",node[4],"parents",[x[4] for x in pa[-4:]],"conds",json.dumps(node[5],separators=(",",":"))[:1400],"actions",json.dumps(node[6],separators=(",",":"))[:2100])
print("DIAGNOSTIC_END")
