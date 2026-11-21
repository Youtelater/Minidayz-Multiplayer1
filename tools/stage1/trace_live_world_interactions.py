#!/usr/bin/env python3
"""Print source evidence needed to port live 3DS tree and automatic-door interactions.

No guessed controls, random numbers, initial HP or geometry here. All
records come directly from the root original data.deobfuscated.js.
"""
from pathlib import Path
import sys, json
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tools"/"stage1"))
from audit_original_world_exhaustive import decode
p=decode((ROOT/"data.deobfuscated.js").read_bytes())
targets={6061166023614368:"DOOR_OPEN",243854400993751:"DOOR_CLOSE",
7227858680569601:"TREE_184_HIT",8754965976446553:"TREE_207_HIT",
5386206472407495:"TREE_209_HIT",9193333425135665:"TREE_576_HIT",
4761103342332918:"TREE_577_HIT",3531921662641626:"TREE_581_HIT"}
def short(obj,length=14000):
    return json.dumps(obj,ensure_ascii=False,separators=(",",":"))[:length]
for oid in (181,184,193,207,209,348,349,576,577,581):
    print("ORIGINAL_OBJECT",oid,short(p[3][oid],3500))
def visit(nodes,sheet,ancestors=()):
    for e in nodes:
        if not isinstance(e,list) or len(e)<8 or e[0]!=0:continue
        if e[4] in targets:
            print("FOUND",targets[e[4]],"SHEET",sheet,"UID",e[4])
            for i,anc in enumerate(ancestors[-5:]):
                print("PARENT",i,"UID",anc[4],"CONDITIONS",short(anc[5],2500),"ACTIONS",short(anc[6],2500))
            print("EVENT_CONDITIONS",short(e[5],7500))
            print("EVENT_ACTIONS",short(e[6],7500))
            print("CHILDREN",short(e[7],4500))
        visit(e[7],sheet,ancestors+(e,))
for sh in p[6]:visit(sh[1],sh[0])

tree_types={184,207,209,576,577,581}
found=[]
def scan(nodes,sheet,chain=()):
    for e in nodes:
        if not isinstance(e,list) or len(e)<8 or e[0]!=0:continue
        for action in e[6]:
            if isinstance(action,list) and len(action)>5 and action[0] in tree_types and action[1] in (87,88):
                found.append((sheet,chain[-3:],e[4],action))
        scan(e[7],sheet,chain+(e[4],))
for sheet in p[6]:scan(sheet[1],sheet[0])
print("TREE_VAR_SET_OR_SUBTRACT_COUNT",len(found))
for sheet,chain,uid,a in found[:65]:
    print("TREE_VAR_ACTION",sheet,uid,chain,short(a,1400))

for layout in p[5] if isinstance(p[5],list) else []:
    raw=json.dumps(layout,ensure_ascii=False,separators=(",",":"))
    if '"t207"' in raw or '"t349"' in raw:
        print("INSTANCE_LAYOUT_STRUCTURE",short(layout,1500))
        break
