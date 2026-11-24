#!/usr/bin/env python3
"""Guard crash-site world generation against the original Construct events."""
from pathlib import Path
import math
import sys

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"tools/stage1"))
from audit_original_world_exhaustive import decode

p=decode((ROOT/"data.deobfuscated.js").read_bytes())
headers=(ROOT/"include/minidayz/source_crash_site_geometry.hpp").read_text()
native=(ROOT/"source/world.cpp").read_text()

def walk(nodes):
    for e in nodes if isinstance(nodes,list) else ():
        if not isinstance(e,list) or len(e)<7 or e[0]!=0:
            continue
        yield e
        if len(e)>7:
            yield from walk(e[7])

game=next(sheet for name,sheet in p[6] if name=="Game_events")
events=list(walk(game))

def find(uid):
    return [e for e in events if e[4]==uid]

def actions(uid):
    return [a for e in find(uid) for a in e[6] if isinstance(a,list)]

def has_action(acts,obj,command,param_object=None):
    return any(a[:2]==[obj,command] and
               (param_object is None or a[5] and a[5][0]==[4,param_object])
               for a in acts)

# Original Game_events function generate_helicrashes. Source first/second
# halves use the same crash quota, but separate Mapgen.var1 predicates.
fn=find(924737797383123)
assert len(fn)==1
root=fn[0]
assert has_action(root[6],344,53)
assert has_action(root[6],434,53)
per_level={}
for e in root[7]:
    if not isinstance(e,list) or e[0]!=0:
        continue
    cond=next((c for c in e[5] if c[:2]==[-1,46] and
               c[9][0]==[11,"CurrentLevel"]),None)
    if cond is None:
        continue
    values={}
    for a in e[6]:
        if a[:2]==[-1,41] and len(a[5])>=2:
            var=a[5][0][1]
            if var in ("helispawned","hammerspawned"):
                values[var]=int(a[5][1][1][1])
    if values and cond[9][1]==[8,0]:
        per_level[int(cond[9][2][1][1])]=values
assert all(per_level[level]=={
    "helispawned":(0,2,3,4,5)[level],
    "hammerspawned":(1,3,5,6,8)[level]}
    for level in range(5)),per_level
assert any(e[5] and any(c[:2]==[-1,46] and
           c[9][0]==[11,"CurrentLevel"] and
           c[9][1]==[8,5] and c[9][2]==[7,[0,4]]
           for c in e[5]) for e in root[7])

heli_spawners=actions(116073859892425)
assert len(heli_spawners)==10
assert has_action(heli_spawners,514,73,344)
assert has_action(heli_spawners,344,47)
assert has_action(heli_spawners,344,88)

hammer_spawners=actions(311546240563344)
# The two child events select original 434 for source marker types
# 13 or 5/15 and place it at Mapgen +500, +600.
assert len(find(311546240563344))==2
assert has_action(actions(468895175988554),514,73,434)
assert has_action(actions(935196605060123),514,73,434)
for event,oid,uid,dy in ((116073859892425,344,629217245088326,500),
                         (468895175988554,434,525441183319823,600),
                         (935196605060123,434,207285229110160,600)):
    expected=[[0,[4,[20,oid,65,False,None],[0,500]]],
              [0,[4,[20,oid,66,False,None],[0,dy]]]]
    assert any(a[:2]==[oid,47] and a[3]==uid and a[5]==expected
               for a in actions(event))

for uid in (427158362727888,147519896698226,897613305483386):
    acts=actions(uid)
    assert acts and has_action(acts,344,73,246)
    assert has_action(acts,246,73,348)
    assert has_action(acts,348,72)

for uid in (820958392602955,888354578468182,897613305483386):
    acts=actions(uid)
    categories=[a for a in acts if a[:2]==[218,88] and
                a[5][0]==[10,0] and a[5][1]==[7,[0,15]]]
    assert len(categories)==3,(uid,len(categories))
    assert sum(a[:2]==[189,69] and
        a[5][0]==[1,[2,"Trigger_spawn"]] for a in acts)>=3

for uid in (904312442718003,246837865038640,
            335691402311173,201052983500428,
            755731948052177,707526540934340,274223755207461):
    acts=actions(uid)
    assert has_action(acts,434,73,441)
    assert sum(a[:2]==[218,88] and a[5][0]==[10,0] and
        a[5][1]==[7,[0,18]] for a in acts)>=3

# The source frame atlas positions, dimensions and named/numeric
# imagepoints must remain matched to assets/gfx spritesheet frames.
frames={oid:p[3][oid][7][0][7]
        for oid in (246,441,348)}
assert frames[246][0][4:6]==[240,186]
assert frames[441][0][4:6]==[103,56]
assert frames[441][1][4:6]==[71,87]
assert frames[348][19][4:6]==[240,186]
assert frames[348][26][4:6]==[1,1]
for oid,frame,required in ((246,0,("Imagepoint 3","Imagepoint 4","Imagepoint 5")),
                           (441,0,("Imagepoint 1","Imagepoint 2","Imagepoint 3","w1","w2")),
                           (441,1,("Imagepoint 1","Imagepoint 2","Imagepoint 3","w1","w2"))):
    names={item[0] for item in frames[oid][frame][9]}
    assert set(required)<=names,(oid,frame,names)

for literal in ("kHeliLootPoints[3]","kHammerStandingLootPoints[3]",
                "kHammerFallenLootPoints[3]",
                "kHammerStandingWallPoints[2]",
                "kHammerFallenWallPoints[2]"):
    assert literal in headers,literal
assert "generate_source_crash_sites()" in native
assert "marker.source_loot_category=category;" in native
assert "source_initial_loot_trigger=true;" in native
print("PASS: root generated crash sites, source sprite geometry, "
      "category-15 heli and category-18 hammer loot, and collision objects")
