#!/usr/bin/env python3
"""Trace exact root Construct straight-road SetTileRange rectangles.

Two original family-1066 actions paint ground tile 15. The event ancestry
and full original x/y/width/height parameter AST are retained; a repeated
60px native road sheet is not proof that these source rectangle actions
or their road-junction branches have been executed.
"""
from pathlib import Path
import json
import sys

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"tools/stage1"))
from audit_original_world_exhaustive import decode
project=decode((ROOT/"data.deobfuscated.js").read_bytes())
uids={358622687629146,532767105758898}
records=[]

def walk(nodes,sheet,parents=(),path=()):
    for i,e in enumerate(nodes if isinstance(nodes,list) else ()):
        if not isinstance(e,list) or len(e)<7 or e[0]!=0:
            continue
        here=path+(i,)
        for ai,a in enumerate(e[6] if isinstance(e[6],list) else ()):
            if not isinstance(a,list) or len(a)<6 or a[3] not in uids:
                continue
            assert a[:2]==[1066,202] and len(a[5])==6
            assert a[5][4]==[0,[0,15]], (
                f"Original road texture choice changed in action {a[3]}")
            records.append({
                "sheet":sheet,"action_uid":a[3],"event_uid":e[4],
                "path":sheet+":"+".".join(map(str,here)),
                "action_index":ai,
                "parent_conditions":[{"uid":p[4],"guards":p[5]} for p in parents],
                "event_conditions":e[5],
                "paint_rect_ast":a[5][:4],
                "source_flags":a[5][5],
            })
        if len(e)>7 and isinstance(e[7],list):
            walk(e[7],sheet,parents+(e,),here)

for sheet,events in project[6]:
    walk(events,sheet)
assert len(records)==2 and {r["action_uid"] for r in records}==uids
for r in records:
    print("SOURCE_ROAD_RECT",json.dumps(r,ensure_ascii=False,separators=(",",":"))[:7000])
print("PASS original 2 straight-road family1066 SetTileRange tile15 actions "
      "with complete rectangle expression ancestry")
