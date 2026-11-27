#!/usr/bin/env python3
"""Verify road-service SetInstanceVariable branches against root MiniDAYZ.

Root generate_array_locations creates ONLY the six original POI types.
Root generate_array_tomap converts roads to 514 markers before selecting
gas stations, cafes, and lite shops by Repeat + OR + PickRandom. Reverting
these to made-up Array518 3x3 footprints deletes source roads and breaks
town/location generation even when C++ still compiles.
"""
from pathlib import Path
import sys

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"tools"/"stage1"))
from audit_original_world_exhaustive import decode

project=decode((ROOT/"data.deobfuscated.js").read_bytes())

def walk(nodes):
    for node in nodes if isinstance(nodes,list) else ():
        if not isinstance(node,list) or len(node)<7 or node[0]!=0:
            continue
        yield node
        if len(node)>7:
            yield from walk(node[7])

def get(uid):
    matches=[node for _,sheet in project[6] for node in walk(sheet)
             if node[4]==uid]
    assert len(matches)==1,(uid,len(matches))
    return matches[0]

def literal(param):
    assert param[:1]==[7] and param[1][:1]==[0],param
    return int(param[1][1])

def event_actions(root):
    for e in walk([root]):
        for a in e[6]:
            if isinstance(a,list) and len(a)>5:
                yield a

raw=get(6938246402369311)
raw_values={
    literal(a[5][2])
    for a in event_actions(raw)
    if a[:2]==[518,199]
}
assert raw_values=={5,6,9,11,15,103},raw_values

mapping=[]
dispatch=get(1694439930581699)
for entry in dispatch[7]:
    if not isinstance(entry,list) or len(entry)<8 or entry[0]!=0:
        continue
    conditions=entry[5]
    repeat=next((c for c in conditions if c[:2]==[-1,123]),None)
    if not repeat or repeat[9][0][:1]!=[0] or repeat[9][0][1][:1]!=[23]:
        continue
    name=repeat[9][0][1][1]
    if name not in ("gas_stations","Roadside_cafes","lite_shop"):
        continue
    bound=next((c for c in conditions if c[:2]==[514,71] and
                c[9][0]==[10,1]),None)
    assert bound,(name,conditions)
    op=bound[9][1][1]
    limit=literal(bound[9][2])
    assert (op,limit) in ((3,15),(5,17)),(name,op,limit)
    assert len(entry[7])==1,name
    or_branch=entry[7][0]
    assert or_branch[2] is True and len(or_branch[5])==2,name
    source_types={
        literal(c[9][2]) for c in or_branch[5] if c[:2]==[514,71]
    }
    assert source_types=={12,13},(name,source_types)
    picker=or_branch[7][0]
    assert any(c[:2]==[-1,124] for c in picker[5]),name
    changes={}
    for child in picker[7]:
        if not isinstance(child,list) or child[0]!=0:
            continue
        input_code=literal(next(c for c in child[5]
            if c[:2]==[514,71])[9][2])
        action=next(a for a in child[6] if a[:2]==[514,88])
        assert action[5][0]==[10,0],name
        changes[input_code]=literal(action[5][1])
    mapping.append((name,op,limit,changes.get(12),changes.get(13)))

expected={
    ("gas_stations",3,15,30,31),
    ("Roadside_cafes",3,15,50,51),
    ("lite_shop",3,15,108,109),
    ("gas_stations",5,17,30,31),
    ("Roadside_cafes",5,17,50,51),
    ("lite_shop",5,17,108,109),
}
assert set(mapping)==expected,(mapping,expected)
assert len(mapping)==6,len(mapping)

# Independent source proof of the campaign service counters. 41 is SET,
# not ADD; the split-map assignments do not accumulate.
def writes(node,name):
    return [literal(a[5][1]) for a in node[6]
            if a[:2]==[-1,41] and a[5][0]==[11,name]]
for uid,expected_count in ((159730919035798,1),(2939943328018906,2),
                           (6778445369859927,3),(599862058562986,4),
                           (298986601539246,4)):
    node=get(uid)
    assert writes(node,"gas_stations")==[expected_count]
    assert writes(node,"Roadside_cafes")==[expected_count]
split=[n for n in walk([raw]) if n[4]==996290260027148]
assert len(split)==2
assert all(writes(n,"gas_stations")==[4] and
           writes(n,"Roadside_cafes")==[4] for n in split)
assert writes(get(159730919035798),"lite_shop")==[0]
assert writes(get(2939943328018906),"lite_shop")==[1]
assert writes(get(298986601539246),"lite_shop")==[2]
# The root sheet-global lite_shop declaration has default 1; levels
# 2/3 and the >4 branches do not assign it, so campaign travel retains it.
assert not writes(get(6778445369859927),"lite_shop")
assert not writes(get(599862058562986),"lite_shop")
assert all(not writes(n,"lite_shop") for n in split)
native=(ROOT/"source/world.cpp").read_text(encoding="utf-8")
assert "place_gas_station_count(" not in native
assert "replace_source_road_markers(map_markers_,rng_" in native
for dispatch_code in ("profile.gas_stations,second_half,30,31",
                      "profile.roadside_cafes,second_half,50,51",
                      "profile.lite_shop,second_half,108,109"):
    assert dispatch_code in native,dispatch_code
overlay=(ROOT/"include/minidayz/source_map_overlay.hpp").read_text()
assert "marker_cell_kind(" in overlay

print("PASS root generate_array_tomap roadside picks: 6 source branches; "
      "gas 30/31, cafe 50/51, lite shop 108/109; original Array518 roads intact")
