#!/usr/bin/env python3
"""Root-original PlayerBase193 variable8 movement source evidence.

The source AltMove MaxSpeed formula includes variable8, but a presence in
an expression does NOT imply an equipment boost. Do not invent that boost:
verify original initial value and enumerate every native Construct
event action that could assign the field.
"""
from pathlib import Path
import sys

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"tools/stage1"))
from audit_original_world_exhaustive import decode

p=decode((ROOT/"data.deobfuscated.js").read_bytes())
layout=next(l for l in p[5] if l[0]=="Map")
instances=[inst for layer in layout[6]
           for inst in (layer[14] if len(layer)>14 and
                        isinstance(layer[14],list) else [])
           if isinstance(inst,list) and len(inst)>3 and inst[1]==193]
assert len(instances)==1, "Original Map PlayerBase193 instance count changed"
original=instances[0]
assert isinstance(original[3],list) and len(original[3])>=9
assert original[3][8]==[0], "Original Map PlayerBase var8 not default zero"

def walk(nodes):
    for e in nodes if isinstance(nodes,list) else []:
        if not isinstance(e,list) or len(e)<7 or e[0]!=0:
            continue
        yield e
        if len(e)>7 and isinstance(e[7],list):
            yield from walk(e[7])

writers=[]
actions_examined=0
for sheet,events in p[6]:
    for event in walk(events):
        for action in event[6] if isinstance(event[6],list) else []:
            if (not isinstance(action,list) or len(action)<6 or
                    action[0]!=193 or not isinstance(action[5],list)):
                continue
            actions_examined+=1
            params=action[5]
            if params and params[0]==[10,8]:
                writers.append((sheet,event[4],action[3],action[1],params))
assert not writers, (
    "PlayerBase193 variable8 source actions changed. Do not assume an "
    f"equipment speed bonus; inspect exact conditions: {writers[:10]}")
assert actions_examined>0
native=(ROOT/"source/main.cpp").read_text(encoding="utf-8")
assert "SourcePlayerAltMoveModifiers speed_effects{}" in native
assert "speed_effects.equipment_bonus=" not in native
print("PASS original PlayerBase193 variable8 initial=0, "
      "zero source event writers, no invented 3DS equipment speed boost")
