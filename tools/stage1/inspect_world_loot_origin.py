#!/usr/bin/env python3
"""Trace original create_map_location -> LOOT_respawn (218) runtime actions.

Source evidence only; printed records preserve original Construct commands
rather than assigning semantics to unknown ACE IDs.
"""
from pathlib import Path
import json
import sys
sys.path.insert(0,str(Path(__file__).resolve().parent))
from audit_original_world_exhaustive import decode, strings
root=Path(__file__).resolve().parents[2]
project=decode((root/"data.deobfuscated.js").read_bytes())
target=8627807469654645
matches=[]
chains=[]
def visit(nodes,sheet,parents=(),path=()):
    for i,e in enumerate(nodes):
        if not isinstance(e,list) or len(e)<8 or e[0]!=0:continue
        p=path+(i,)
        ancestry=parents+(e[4],)
        if e[4]==target:
            matches.append((sheet,p,e))
        for ix,a in enumerate(e[6] if isinstance(e[6],list) else []):
            if not isinstance(a,list) or len(a)<6:continue
            if a[0]==218 or (a[0]==-1 and a[1]==94 and
                 (a[5][0] == 218 if isinstance(a[5],list) and a[5] else False)):
                chains.append({"sheet":sheet,"path":p,"uid":e[4],
                    "ancestors":parents[-8:],"conditions":e[5][:6],
                    "action_index":ix,"action":a,"other_actions":e[6][:8]})
        visit(e[7],sheet,ancestry,p)
for sheet in project[6]:visit(sheet[1],sheet[0])
print("CREATE_MAP_LOCATION_BRANCHES",len(matches))
for name,path,event in matches[:5]:
    print("CREATE_MAP_LOCATION",name,".".join(map(str,path)),json.dumps({
        "uid":event[4],"conditions":event[5],"actions":event[6],
        "child_count":len(event[7]),"child_uids":[c[4] for c in event[7][:30]
            if isinstance(c,list) and len(c)>4]
    },ensure_ascii=False)[:4500])
print("LOOT_MARKER_218_ACTIONS",len(chains))
for row in chains[:35]:
    print("LOOT_MARKER_ACTION",json.dumps(row,ensure_ascii=False)[:2300])
