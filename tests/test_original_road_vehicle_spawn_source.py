#!/usr/bin/env python3
"""Check all four root car respawn functions against native road-car placement."""
from pathlib import Path
import sys

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"tools"/"stage1"))
from audit_original_world_exhaustive import decode
p=decode((ROOT/"data.deobfuscated.js").read_bytes())

def walk(nodes):
    for e in nodes if isinstance(nodes,list) else ():
        if not isinstance(e,list) or len(e)<7 or e[0]!=0:
            continue
        yield e
        if len(e)>7:
            yield from walk(e[7])

game=next(events for sheet,events in p[6] if sheet=="Game_events")
all_events=list(walk(game))

def function(name):
    roots=[e for e in all_events if any(
        c[:2]==[189,40] and
        c[9][0]==[1,[2,name]]
        for c in e[5])]
    assert len(roots)==1,(name,len(roots))
    return roots[0]

cars=(
    ("Car_respawn_uaz",809,1175,12,13,2),
    ("Car_respawn_volga",815,1177,12,13,2),
    ("Car_respawn_hammer",920,0,70,70,1),
    ("Car_respawn_ka",1343,0,104,104,1),
)
for name,body,overlay,type_a,type_b,spawns in cars:
    event=function(name)
    nodes=list(walk([event]))
    actions=[a for node in nodes for a in node[6]
             if isinstance(a,list) and len(a)>5]
    creates=[a for a in actions if a[:2]==[-1,94] and
             a[5] and a[5][0]==[4,body]]
    assert len(creates)==spawns,(name,len(creates))
    observed={
        int(c[9][2][1][1])
        for node in nodes for c in node[5]
        if c[:2]==[514,71] and c[9][0]==[10,0] and
        c[9][2][:1]==[7] and c[9][2][1][:1]==[0]
    }
    assert {type_a,type_b}<=observed,(name,observed)
    if overlay:
        companions=[a for a in actions if a[1]==73 and a[5] and
                    a[5][0]==[4,overlay]]
        assert len(companions)==2,(name,len(companions))
    source_frame=p[3][body][7][0][7][0]
    assert source_frame[4]>0 and source_frame[5]>0

native=(ROOT/"source/world.cpp").read_text()
header=(ROOT/"include/minidayz/source_road_vehicle_geometry.hpp").read_text()
assert "generate_source_road_vehicles()" in native
for obj in (809,815,920,1343):
    assert str(obj) in header
# Root vehicle System.Create offsets contain random(800)/random(200)
# without floor(). Native integer-only car positions lost valid original
# fractional positions and consumed a different expression path.
for name,body,*_ in cars[:2]:
    event=function(name)
    creates=[a for e in walk([event]) for a in e[6]
             if isinstance(a,list) and len(a)>5 and
             a[:2]==[-1,94] and a[5] and a[5][0]==[4,body]]
    assert all("95" in repr(a[5][2:4]) for a in creates),name
assert "source_random_unit(rng_)*" in native
assert "(vertical?800.0:200.0)" in native
assert "(vertical?200.0:800.0)" in native
assert "source_random_bounded(rng_,vertical?800:200)" not in native
assert "p.x+874.0f,p.y+855.0f,180.0f" in native
assert "p.x+150.0f,p.y+500.0f" in native
print("PASS original road vehicle spawn callbacks: UAZ Volga Hammer KA, "
      "original position offsets, orientations and source sprite frames")
