#!/usr/bin/env python3
"""Require original Construct AltMove speed expression and native modifier order."""
from pathlib import Path
import json,re
R=Path(__file__).resolve().parents[1]
s=(R/"data/stage1/construct_project_parsed.json").read_text(encoding="utf-8-sig")
s=re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)","-",s)
p=json.loads(s)["project"]
def nodes(seq):
    for e in seq if isinstance(seq,list) else ():
        if not isinstance(e,list) or not e:continue
        if e[0]==0 and len(e)>=7:
            yield e
            if len(e)>7:yield from nodes(e[7])
events=dict(p[6])["Game_events"]
hit=[(e,a) for e in nodes(events) for a in e[6]
     if isinstance(a,list) and len(a)>5 and a[3]==156884066447061]
assert len(hit)==1
e,a=hit[0]
assert e[4]==6013847730477848
assert a[:2]==[193,253] and a[2]=="AltMove"
v=lambda i:[21,193,False,None,i]
expected=[4,[4,[4,[4,[5,[5,[0,100],v(6)],v(7)],v(4)],v(3)],v(5)],v(8)]
assert a[5]==[[0,expected]],(a[5],expected)
header=(R/"include/minidayz/source_player_altmove.hpp").read_text()
assert "100.0f-m.damage_slowdown-m.terrain_slowdown+" in header
assert "m.energy_boost+m.runner_bonus+m.regen_bonus+" in header
game=(R/"source/main.cpp").read_text()
assert "speed_effects.runner_bonus" in game
assert "speed_effects.regen_bonus" in game
assert "source_player_altmove_max_speed(speed_effects)" in game
print("PASS: exact original AltMove SetMaxSpeed formula and verified runner/regen bonuses")
