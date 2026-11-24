#!/usr/bin/env python3
"""Verify 3DS radiation-zone world placements use root original function wuranqu."""
from pathlib import Path
import sys
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"tools/stage1"))
from audit_original_world_exhaustive import decode
p=decode((ROOT/"data.deobfuscated.js").read_bytes())

def walk(nodes):
    for e in nodes if isinstance(nodes,list) else ():
        if not isinstance(e,list) or len(e)<7 or e[0]!=0:continue
        yield e
        if len(e)>7:
            yield from walk(e[7])

nodes=[e for _,sheet in p[6] for e in walk(sheet)]
fn=[e for e in nodes if e[4]==316200290612230]
assert len(fn)==1
root=fn[0]
assert any(c[:2]==[189,40] and
    c[9][0]==[1,[2,"wuranqu"]] for c in root[5])
military=[e for e in walk([root]) if e[4]==849848542281325]
assert len(military)==1
assert any(a[:2]==[514,73] and a[3]==513805253688873 and
           a[5][0]==[4,1126] and a[5][1]==[5,[2,"Rain"]]
           for a in military[0][6])
picked=[e for e in walk([root]) if e[4]==475451033019087]
assert len(picked)==5
assert all(any(a[:2]==[514,73] and a[3]==680790671133170
                   and a[5][0]==[4,1126] for a in e[6])
           for e in picked)
# Check the actual positive-offset expressions, not just the spawn IDs.
def assert_offset(events, uid):
    expr=[[0,[4,[20,1126,65,False,None],[0,500]]],
          [0,[4,[20,1126,66,False,None],[0,500]]]]
    assert all(any(a[:2]==[1126,47] and a[3]==uid and a[5]==expr
                   for a in e[6]) for e in events)
assert_offset(military,704202770294192)
assert_offset(picked,481838527458372)
filters=[e for e in walk([root]) if e[4]==174342964709830]
assert len(filters)==5
expected={5,22,102,15,103,11,6,9,104,106,32}
for e in filters:
    assert e[2] is True
    types={int(c[9][2][1][1]) for c in e[5]
           if c[:2]==[514,71]}
    assert types==expected,types

# CurrentLevel exactly 3 -> Repeat(2) first-half only. CurrentLevel
# >=4 -> perk_dao !=7 Repeat(3) each half, or perk_dao==7 Repeat(6).
by_branch={e[4]:e for e in root[7] if isinstance(e,list) and e[0]==0}
low=by_branch[510231927288336]
assert low[5][0][9]==[[11,"CurrentLevel"],[8,0],[7,[0,3]]]
assert low[5][1][9]==[[0,[0,2]]]
upper=[e for e in root[7] if isinstance(e,list) and
       e[0]==0 and e[4]==389956509163909]
assert len(upper)==4
assert sorted(int(c[9][0][1][1]) for e in upper
           for c in e[5] if c[:2]==[-1,123])==[3,3,6,6]
assert all(e[7][0][4]==174342964709830 for e in upper)
# Source object1126 frame0 dimensions; preserve the original Rain
# layer and centered hotspot.
frame=p[3][1126][7][0][7][0]
assert frame[4:6]==[2000,2000]
assert frame[7:9]==[0.5,0.5]
h=(ROOT/"include/minidayz/source_radiation_zone.hpp").read_text()
src=(ROOT/"source/world.cpp").read_text()
vis=(ROOT/"include/minidayz/terrain_visible_index.hpp").read_text()
for s in ("kOriginalRadioactiveSprite=1126",
          "kOriginalRadioactiveWidth=2000.0f",
          "kOriginalRadioactiveOffsetX=500.0f",
          "kOriginalRadioactivePickedSpawnAction"):
    assert s in h,s
assert "generate_source_radiation_sites()" in src
# Original Sprite1126 is visible on source Rain44; the renderer now
# admits only root-verified high-layer gameplay art, including the
# Sprite241 car trunk on Sunset_Dawn45. Do not require the obsolete
# blanket high-layer check that hid every car trunk.
assert "source_world_high_layer_gameplay_art(" in vis
source_roles=(ROOT/"include/minidayz/source_world_sprite_visibility.hpp").read_text()
assert 'object==1126 && std::strcmp(source_layer_name,"Rain")==0' in source_roles
assert 'object==241 && std::strcmp(source_layer_name,"Sunset_Dawn")==0' in source_roles
print("PASS root wuranqu source placement: 70 military overlays; "
      "2 level-three and 3/6-per-half level-four/five radiation zones")
