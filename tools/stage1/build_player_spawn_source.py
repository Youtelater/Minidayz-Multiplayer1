#!/usr/bin/env python3
"""Decode exact original NEW-GAME player spawn actions from Game_events.

The original PlayerBase object is 193 (player_base-sheet0.png), not a
random generated settlement node. The first Start events' unsaved-game branch
uses Set Position with X=600 and Y=1500+random(LayoutHeight-5000) on Map;
CurrentLevel 4 overrides with X=random(1000,16500), Y=373.
Verify every expression and its parent conditions before generating C++.
"""
import json
import re
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
INPUT=ROOT/"data/stage1/construct_project_parsed.json"
OUTPUT=ROOT/"include/minidayz/generated/player_spawn_source.hpp"

def nodes(items, groups=()):
    for entry in items:
        if not isinstance(entry,list) or len(entry)<8 or entry[0]!=0:continue
        name=(entry[1][1] if isinstance(entry[1],list) and
              len(entry[1])>1 and isinstance(entry[1][1],str) else "")
        path=groups+(name,) if name else groups
        yield path,entry
        yield from nodes(entry[7],path)

def main():
    text=re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)","-",
                INPUT.read_text(encoding="utf-8-sig"))
    project=json.loads(text)["project"]
    objects=project[3]
    assert any("player_base-sheet0.png" in str(fr[0])
               for anim in objects[193][7] for fr in anim[7]
               if isinstance(fr,list) and fr), "Source PlayerBase 193 changed"
    source=dict(project[6])["Game_events"]
    found={}
    for group,e in nodes(source):
        if "Start events" not in group:continue
        for action in e[6]:
            if (not isinstance(action,list) or len(action)<6 or
                action[0]!=193 or action[1]!=47):continue
            if action[3] in (9846853259707544,641409757696071):
                found[action[3]]=(e[4],e[5],action[5])
    assert set(found)=={9846853259707544,641409757696071},found.keys()
    uid,conditions,params=found[9846853259707544]
    assert uid==9183183159438440
    assert all(any(p[0]==11 and p[1]==name for p in c[-1])
               for name in ("fromsav","sav_checked","sav_checked_tutor")
               for c in conditions if isinstance(c,list) and c[0]==-1 and
               len(c)>9 and isinstance(c[-1],list)), "Save guards changed"
    x,y=(p[1] for p in params)
    assert x==[0,600],x
    expected=[4,[0,1500],[18,[12,[19,52],[2,"Map"]],
              [19,95,[[5,[19,152],[0,5000]]]],[0,-500]]]
    assert y==expected,("Normal original spawn Y expression changed",y)
    uid,conditions,params=found[641409757696071]
    assert uid==980530989134876
    assert any(c[0]==-1 and c[1]==46 and
               [11,"CurrentLevel"] in c[-1] and [0,4] in c[-1]
               for c in conditions), "Level 4 source spawn override changed"
    assert params==[[0,[19,95,[[0,1000],[0,16500]]]],[0,[0,373]]],params
    OUTPUT.parent.mkdir(parents=True,exist_ok=True)
    OUTPUT.write_text("""#pragma once
// Exact original Construct Start events: 9183183159438440 and 980530989134876.
namespace minidayz::stage1 {
inline constexpr float kNormalPlayerStartX=600.0f;
inline constexpr float kNormalPlayerStartYBase=1500.0f;
inline constexpr float kNormalPlayerStartLayoutMargin=5000.0f;
inline constexpr float kOtherLayoutStartYOffset=-500.0f;
inline constexpr unsigned kSnowPlayerStartLevel=4u;
inline constexpr float kSnowPlayerStartXMin=1000.0f;
inline constexpr float kSnowPlayerStartXMax=16500.0f;
inline constexpr float kSnowPlayerStartY=373.0f;
}
""",encoding="utf-8")
    print("Verified original Start events SetPosition player spawn actions "
          "9846853259707544 and 641409757696071")

if __name__=="__main__":
    main()
