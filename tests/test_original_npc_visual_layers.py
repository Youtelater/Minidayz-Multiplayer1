#!/usr/bin/env python3
"""Check original NPC controller/body Map layers; visual art != game logic.

The ROOT MiniDAYZ Spawn_NPC event creates an invisible Sprite256 controller
on zed_resp and six skin bodies on Zeds. The 3DS must render only bodies at
source Zeds depth, not paint the hidden controller's eye texture over them.
"""
from pathlib import Path
import json
import re

ROOT=Path(__file__).resolve().parents[1]
raw=(ROOT/"data.deobfuscated.js").read_text(encoding="utf-8-sig")
game=json.loads(re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)","-",raw))["project"]
layout=next(row for row in game[5] if row[0]=="Map")
layers={row[0]:(index,row) for index,row in enumerate(layout[6])}
assert layers["Zeds"][0]==29 and layers["Zeds"][1][3] is True
assert layers["zed_resp"][0]==49 and layers["zed_resp"][1][3] is False

def walk(events):
    for event in events if isinstance(events,list) else ():
        if not isinstance(event,list) or len(event)<7 or event[0]!=0:
            continue
        yield event
        if len(event)>7 and isinstance(event[7],list):
            yield from walk(event[7])

source=next(entries for name,entries in game[6] if name=="Game_events")
function=[event for event in walk(source) if event[4]==180375540292947]
assert len(function)==1, "Root Spawn_NPC function was not found exactly once"
events=list(walk([function[0]]))
created=[event for event in events if event[4]==943134110152762]
assert len(created)==1
assert any(action[:2]==[216,73] and action[5][:2]==[
        [4,256],[5,[2,"zed_resp"]]]
    for action in created[0][6]), "Source eyes/controller no longer on hidden zed_resp"

skin_to_body={1:600,2:206,3:582,4:583,5:586,6:587}
for skin,body in skin_to_body.items():
    branches=[event for event in events if
        any(action[:2]==[256,73] and action[5][:2]==[
            [4,body],[5,[2,"Zeds"]]]
            for action in event[6] if isinstance(action,list) and len(action)>5)]
    # Source Spawn_NPC has two guarded creation paths for each body.
    # Checking only the first branch silently loses a whole action family.
    assert len(branches)==2, f"Original skin {skin} body Spawn branch count changed"
    for branch in branches:
        assert any(action[:3]==[body,57,"Pin2"] and action[5]==[
            [4,256],[3,1]] for action in branch[6]), (
            f"Source skin {skin} body no longer pinned to hidden controller")

# Do not silently collapse the SPECIAL original Spawn_NPC body branches
# into the six ordinary walkers. They have independent inherited source
# guards (function Param2=5/7/17/19, and riot-global state for body932).
# Native support for these event chains remains an explicit OPEN parity
# item, not permission to spawn them with guessed probabilities.
from collections import Counter
special_bodies=Counter(
    action[5][0][1]
    for event in events
    for action in event[6] if isinstance(action,list) and len(action)>5
    and action[:2]==[256,73]
    and action[5][1]==[5,[2,"Zeds"]]
    and action[5][0][1] not in skin_to_body.values()
)
assert special_bodies==Counter({932:1,718:2,956:1,982:2}), (
    f"Original guarded special NPC bodies changed: {special_bodies}")

# Every source reference to zed_resp is a spawn/position action, never a
# visible-world layer-toggle. The controller layer remains hidden.
for sheet,all_events in game[6]:
    for event in walk(all_events):
        for action in event[6] if isinstance(event[6],list) else ():
            if not isinstance(action,list) or len(action)<6:
                continue
            if "zed_resp" in json.dumps(action[5]).lower():
                assert action[1] in (47,73,94), (
                    sheet,event[4],action[3],"unexpected zed_resp action")

renderer=(ROOT/"source/main.cpp").read_text(encoding="utf-8")
assert "source_world_layer_depth(29,0,0)" in renderer
assert "npc->x, npc->y, 1.0f, 1.0f,npc_body_depth" in renderer
assert "renderer.draw_source_sprite(npc->eyes_object" not in renderer
print("PASS: original NPC body on Zeds29, hidden controller on zed_resp49")
