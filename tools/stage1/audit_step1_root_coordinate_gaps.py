#!/usr/bin/env python3
"""Read ROOT MiniDAYZ data.deobfuscated.js, never the stale stage1 snapshot.

For Step 1 parity: identify original map Tilemap coordinate semantics,
dynamic road painting, and exact source action references. This is a
source EVIDENCE report, not a claim that C++ matches every action.
"""
import json
import sys
from collections import Counter
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tools/stage1"))
from audit_original_world_exhaustive import decode
p=decode((ROOT/"data.deobfuscated.js").read_bytes())
obj=p[3]
world=next(l for l in p[5] if l[0]=="Map")
inspect_objects={1066,518,514,525,482,687,688,776,980,1138,1139,230,208,211,418}
report={"authority":"root data.deobfuscated.js","object_definitions":[],
        "map_layout":[],"coordinate_expression_uses":[],"road_actions":[],
        "tilemap_actions":[]}
for oid in sorted(inspect_objects):
    o=obj[oid]
    report["object_definitions"].append({"id":oid,"name":o[0],
        "type":o[1],"image":str(o[6][0]) if len(o)>6 and o[6] else None})
for index,layer in enumerate(world[6]):
    item={"index":index,"name":layer[0],"objects":[]}
    for ins in layer[14] if len(layer)>14 and isinstance(layer[14],list) else []:
        if not isinstance(ins,list) or len(ins)<6: continue
        oid=ins[1]
        if oid not in inspect_objects: continue
        transform=ins[0]
        item["objects"].append({"object":oid,"uid":ins[2],
             "x":transform[0] if len(transform)>0 else None,
             "y":transform[1] if len(transform)>1 else None,
             "width":transform[3] if len(transform)>3 else None,
             "height":transform[4] if len(transform)>4 else None,
             "data":str(ins[5])[:140],
             "tilemap_header":transform[13][:2] if len(transform)>13 and isinstance(transform[13],list) else None})
    if item["objects"]:report["map_layout"].append(item)
def visit(events,anc=()):
    for e in events if isinstance(events,list) else ():
        if not isinstance(e,list) or len(e)<7 or e[0]!=0:continue
        yield e,anc
        if len(e)>7:
            yield from visit(e[7],anc+(e[4],))
def find_expr_id(expr,oid):
    if isinstance(expr,list):
        if len(expr)>=5 and expr[:2]==[20,oid] and expr[2] in (220,221):
            yield expr
        for child in expr:yield from find_expr_id(child,oid)
for sheet_record in p[6]:
    sheet,events=sheet_record[:2]
    for e,anc in visit(events):
        for c in e[5] if isinstance(e[5],list) else []:
            if not isinstance(c,list):continue
            for oid in [1066]:
                if list(find_expr_id(c,oid)):
                    report["coordinate_expression_uses"].append({"sheet":sheet,"event_uid":e[4],
                    "parent_uid":list(anc[-5:]),"kind":"condition",
                    "raw":c})
        for a in e[6] if isinstance(e[6],list) else []:
            if not isinstance(a,list) or len(a)<6:continue
            if a[0] in (208,211,418,230,482,687,688,776,980,1138,1139,1066) and a[1] in (189,192,202):
                report["tilemap_actions"].append((sheet,e[4],a[0],a[1],a[3]))
            if a[0] in (514,518,525) and a[1] in (199,200,203,210,211):
                report["road_actions"].append({"sheet":sheet,"event_uid":e[4],
                  "parent_uid":list(anc[-5:]),"object":a[0],"command":a[1],
                  "params":a[5]})
            for oid in [1066]:
                if list(find_expr_id(a,oid)):
                    report["coordinate_expression_uses"].append({"sheet":sheet,"event_uid":e[4],
                        "parent_uid":list(anc[-5:]),"kind":"action",
                        "raw":a})
# The seven ground objects receive paint actions through their Construct family.
members=next(f[1:] for f in p[4] if f[0]==1066)
report["ground_family_members"]=members
paint_counts=Counter(a[3] for a in report["tilemap_actions"] if a[2]==1066)
assert set(members)=={482,687,688,776,980,1138,1139}
assert paint_counts=={192:398,202:163}, paint_counts
assert report["coordinate_expression_uses"], "No family1066 position-to-tile calls found"
out=ROOT/"audit-output/world/step1_coordinate_and_road_source.json"
out.parent.mkdir(parents=True,exist_ok=True)
out.write_text(json.dumps(report,indent=2)+"\n")
print("STEP1_ROOT_MAP_TILEMAP_OBJECTS",report["object_definitions"])
print("STEP1_ROOT_MAP_LAYOUT",report["map_layout"])
print("STEP1_ROOT_TILEMAP_ACTION_COUNTS",sorted(Counter((a[2],a[3]) for a in report["tilemap_actions"]).items()))
print("STEP1_ROOT_TILEMAP_LOCATION_ACTIONS",len(report["tilemap_actions"]))
print("STEP1_ROOT_1066_EXPRESSIONS",report["coordinate_expression_uses"][:12])
print("STEP1_ROOT_ROAD_ACTION_SAMPLES",report["road_actions"][:7])
