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
# Root Game_events authored actual 17x2 / 2x17 rectangles, not
# the old one-tile-width native visual substitute. Guard the exact
# original Tilemapspot +8 offset and both dimensions.
source_rects={
    358622687629146:[
        [0,[23,"TilemapspotX"]],
        [0,[4,[23,"TilemapspotY"],[0,8]]],
        [0,[0,17]],
        [0,[0,2]],
    ],
    532767105758898:[
        [0,[4,[23,"TilemapspotX"],[0,8]]],
        [0,[23,"TilemapspotY"]],
        [0,[0,2]],
        [0,[0,17]],
    ],
}
for r in records:
    assert r["paint_rect_ast"]==source_rects[r["action_uid"]], (
        "Original source road painting rectangle/offset changed")
native=(ROOT/"include/minidayz/source_mapgen_geometry.hpp").read_text(encoding="utf-8")
renderer=(ROOT/"source/engine.cpp").read_text(encoding="utf-8")
scene=(ROOT/"source/main.cpp").read_text(encoding="utf-8")
assert "kOriginalRoadLengthTiles=17" in native
assert "kOriginalRoadWidthTiles=2" in native
assert "source_road_painted_width(" in renderer
assert "source_road_painted_width(" in scene

# Resolve the original tile-space origin before changing the native
# corridor world coordinates. The source uses TilemapspotX/Y to anchor
# the authored 17x2 and 2x17 paint rectangles.
spot_writers=[]
def spotwalk(nodes,sheet,parents=()):
    for e in nodes if isinstance(nodes,list) else ():
        if not isinstance(e,list) or len(e)<7 or e[0]!=0:continue
        for a in e[6] if isinstance(e[6],list) else ():
            if not isinstance(a,list) or len(a)<6 or a[:2]!=[-1,41]:
                continue
            if not a[5] or a[5][0] not in ([11,"TilemapspotX"],[11,"TilemapspotY"]):
                continue
            spot_writers.append({"sheet":sheet,"event":e[4],
                                 "parents":[p[4] for p in parents],
                                 "action":a})
        if len(e)>7 and isinstance(e[7],list):
            spotwalk(e[7],sheet,parents+(e,))
for sh,events in project[6]:
    spotwalk(events,sh)
assert spot_writers,"Original TilemapspotX/Y formulas disappeared"
for row in spot_writers:
    print("SOURCE_ROAD_ORIGIN",json.dumps(row,separators=(",",":"))[:7000])
# Inspect every original create_map_location road-category branch,
# including code14 intersections; don't assume two overlapping native
# straight strips are all the original authored tile operations.
road_branch_report=[]
def find_road_branches(nodes,parents=()):
    for e in nodes if isinstance(nodes,list) else ():
        if not isinstance(e,list) or len(e)<7 or e[0]!=0:continue
        codes=[]
        for c in e[5] if isinstance(e[5],list) else ():
            if isinstance(c,list) and len(c)>9 and c[:2]==[514,71]:
                for p in c[9]:
                    if isinstance(p,list) and p[:1]==[7] and len(p)>1 and (
                        isinstance(p[1],list) and len(p[1])==2 and p[1][0]==0
                        and p[1][1] in (12,13,14)):
                        codes.append(p[1][1])
        if codes and any(p[4]==8627807469654645 for p in parents):
            paints=[]
            def visit(branch):
                for a in branch[6] if isinstance(branch[6],list) else ():
                    if isinstance(a,list) and len(a)>5 and a[0]==1066 and a[1] in (192,202):
                        paints.append({"event":branch[4],"uid":a[3],
                                       "op":a[1],"params":a[5]})
                if len(branch)>7 and isinstance(branch[7],list):
                    for child in branch[7]:
                        if isinstance(child,list) and len(child)>=7 and child[0]==0:
                            visit(child)
            visit(e)
            road_branch_report.append({"codes":codes,"event_uid":e[4],
                                       "paint_actions":paints})
        if len(e)>7 and isinstance(e[7],list):
            find_road_branches(e[7],parents+(e,))
find_road_branches(dict(project[6])["Game_events"])
# The original code14 junction has four separately gated 9-cell
# half-strips rather than two invented unconditional full-length roads.
# Export their exact inherited guards for native source-condition parity.
source_leg_ids={
    1586657255052358,8618616698851008,
    4462790001866082,7247777744760893,
}
found_legs=set()
def trace_road_legs(nodes,parents=()):
    for e in nodes if isinstance(nodes,list) else ():
        if not isinstance(e,list) or len(e)<7 or e[0]!=0:continue
        for a in e[6] if isinstance(e[6],list) else ():
            if isinstance(a,list) and len(a)>5 and a[3] in source_leg_ids:
                assert a[:2]==[1066,202] and a[5][4]==[0,[0,15]]
                assert 9 in (a[5][2][1][1],a[5][3][1][1])
                source_codes={9,6,4,12,13,14,5,11,15,103}
                # Original Construct event is an OR-block of ten
                # Array518.At(X,Y)==literal predicates. No approximation:
                # retain all ten original codes and their Array indices.
                assert len(e[5])==10 and e[2], (
                    "Source code14 leg lost its ten-condition OR block")
                originals=set()
                first_coordinate_pair=None
                for cond in e[5]:
                    assert cond[:2]==[518,203] and len(cond)>9
                    assert len(cond[9])==4 and cond[9][2]==[8,0]
                    assert cond[9][3][0]==7 and cond[9][3][1][0]==0
                    originals.add(cond[9][3][1][1])
                    coordinates=cond[9][:2]
                    if first_coordinate_pair is None:
                        first_coordinate_pair=coordinates
                    else:
                        assert first_coordinate_pair==coordinates, (
                            "Inconsistent original junction neighbor picking")
                assert originals==source_codes
                expected_axis={
                    1586657255052358:(2,5),
                    8618616698851008:(2,4),
                    4462790001866082:(1,5),
                    7247777744760893:(1,4),
                }
                slot,operation=expected_axis[a[3]]
                shifted=first_coordinate_pair[0] if slot==2 else first_coordinate_pair[1]
                untouched=first_coordinate_pair[1] if slot==2 else first_coordinate_pair[0]
                assert shifted[0]==0 and shifted[1][0]==operation
                assert shifted[1][1]==[21,514,False,None,slot]
                assert shifted[1][2]==[0,1]
                assert untouched==[0,[21,514,False,None,1 if slot==2 else 2]]
                found_legs.add(a[3])
                print("SOURCE_ROAD_LEG",json.dumps({
                    "uid":a[3],"event":e[4],
                    "conditions":e[5],
                    "ancestors":[{"event":p[4],"guards":p[5]} for p in parents[-5:]],
                    "rect":a[5][:4]
                },separators=(",",":"))[:8000])
        if len(e)>7 and isinstance(e[7],list):
            trace_road_legs(e[7],parents+(e,))
trace_road_legs(dict(project[6])["Game_events"])
assert found_legs==source_leg_ids

# Source road codes12 and13 also contain optional 9-tile
# perpendicular arms. Record each exact event/parent predicate before
# implementing them; a road code alone never authorizes those pixels.
additional_leg_ids={
    4140793207746288,1609283690277773,
    7388915329079069,9705891230602422,
    6009205636428013,2753364281434062,
    1939245565475465,4493044907863185,
}
seen_extra=set()
def trace_extra(nodes,anc=()):
    for e in nodes if isinstance(nodes,list) else ():
        if not isinstance(e,list) or len(e)<7 or e[0]!=0:continue
        for a in e[6] if isinstance(e[6],list) else ():
            if not isinstance(a,list) or len(a)<6 or a[3] not in additional_leg_ids:
                continue
            assert a[:2]==[1066,202] and a[5][4]==[0,[0,15]]
            conditions=e[5] if isinstance(e[5],list) else []
            assert e[2] and all(c[:2]==[518,203] for c in conditions)
            assert all(c[9][2]==[8,0] for c in conditions)
            source_base=12 if a[3] in {
                4140793207746288,1609283690277773,
                7388915329079069,9705891230602422
            } else 13
            source_arm={
                4140793207746288:0,1609283690277773:1,
                7388915329079069:2,9705891230602422:3,
                6009205636428013:0,2753364281434062:1,
                1939245565475465:2,4493044907863185:3,
            }[a[3]]
            allowed={9,6,4,12,13,14,5,11,15,103}
            if source_base==12 and source_arm in (0,1):
                allowed.remove(12)
            if source_base==13 and source_arm in (2,3):
                allowed.remove(13)
            found={c[9][3][1][1] for c in conditions}
            assert len(conditions)==len(allowed) and found==allowed, (
                f"Original source road {source_base} extension guard changed")
            expected_coord={
                0:[[0,[5,[21,514,False,None,2],[0,1]]],
                   [0,[21,514,False,None,1]]],
                1:[[0,[4,[21,514,False,None,2],[0,1]]],
                   [0,[21,514,False,None,1]]],
                2:[[0,[21,514,False,None,2]],
                   [0,[5,[21,514,False,None,1],[0,1]]]],
                3:[[0,[21,514,False,None,2]],
                   [0,[4,[21,514,False,None,1],[0,1]]]],
            }[source_arm]
            assert all(c[9][:2]==expected_coord for c in conditions)
            expected_rect={
                0:[[0,[4,[23,"TilemapspotX"],[0,8]]],
                   [0,[23,"TilemapspotY"]],[0,[0,2]],[0,[0,9]]],
                1:[[0,[4,[23,"TilemapspotX"],[0,8]]],
                   [0,[4,[23,"TilemapspotY"],[0,8]]],
                   [0,[0,2]],[0,[0,9]]],
                2:[[0,[23,"TilemapspotX"]],
                   [0,[4,[23,"TilemapspotY"],[0,8]]],
                   [0,[0,9]],[0,[0,2]]],
                3:[[0,[4,[23,"TilemapspotX"],[0,8]]],
                   [0,[4,[23,"TilemapspotY"],[0,8]]],
                   [0,[0,9]],[0,[0,2]]],
            }[source_arm]
            assert a[5][:4]==expected_rect
            seen_extra.add(a[3])
            print("SOURCE_ROAD_EXTRA_LEG",json.dumps({
                "uid":a[3],"event":e[4],
                "or_block":e[2],
                "conditions":conditions,
                "parents":[{"event":p[4],"guards":p[5]} for p in anc[-5:]],
                "rect":a[5][:4]
            },separators=(",",":"))[:8000])
        if len(e)>7 and isinstance(e[7],list):
            trace_extra(e[7],anc+(e,))
trace_extra(dict(project[6])["Game_events"])
assert seen_extra==additional_leg_ids

# Capture every ancestor of the original single-cell road repaint
# actions, including loop dimensions, coordinate setters and tile reads.
# These must be implemented from source, not guessed from image names.
edge_uids={a["uid"] for r in road_branch_report for a in r["paint_actions"] if a["op"]==192}
def trace_edge_actions(nodes,anc=(),path=()):
    for i,e in enumerate(nodes if isinstance(nodes,list) else ()):
        if not isinstance(e,list) or len(e)<7 or e[0]!=0:continue
        here=path+(i,)
        for a in e[6]:
            if isinstance(a,list) and len(a)>5 and a[3] in edge_uids:
                print("SOURCE_ROAD_EDGE",json.dumps({
                    "uid":a[3],"event":e[4],"path":here,
                    "guards":e[5],"or":e[2],"params":a[5],
                    "ancestors":[{"uid":p[4],"or":p[2],"conditions":p[5],
                                  "actions":p[6]} for p in anc
                                  if p[4] in {3785908327653467,5407406943689734,1239724375006415}
                                  or any(q[4] in {3785908327653467,5407406943689734,1239724375006415}
                                         for q in anc[:anc.index(p)])]
                },separators=(",",":")))
        if len(e)>7 and isinstance(e[7],list):
            trace_edge_actions(e[7],anc+(e,),here)
assert len(edge_uids)==36
trace_edge_actions(dict(project[6])["Game_events"])

for row in road_branch_report:
    print("SOURCE_ROAD_BRANCH",json.dumps(row,separators=(",",":"))[:6500])

for r in records:
    print("SOURCE_ROAD_RECT",json.dumps(r,ensure_ascii=False,separators=(",",":"))[:7000])
print("PASS original 2 straight-road family1066 SetTileRange tile15 actions "
      "with complete rectangle expression ancestry")
