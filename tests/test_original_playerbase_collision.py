#!/usr/bin/env python3
"""Check source PlayerBase193 collision dimensions/hotspot, not skin art."""
from pathlib import Path
import re,json
R=Path(__file__).resolve().parents[1]
s=(R/"data/stage1/construct_project_parsed.json").read_text(encoding="utf-8-sig")
s=re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)","-",s)
p=json.loads(s)["project"]
frame=p[3][193][7][0][7][0]
assert frame[4:6]==[2,2] and frame[7:9]==[1,1],frame[:11]
assert "player_base-sheet0.png" in frame[0]
layout=next(l for l in p[5] if l[0]=="Map")
instances=[i for layer in layout[6] for i in layer[14] if isinstance(i,list) and len(i)>2 and i[1]==193]
assert len(instances)==1
assert instances[0][0][3:5]==[2,2],instances[0][0]
h=(R/"include/minidayz/source_player_base_geometry.hpp").read_text()
for name,value in (("HalfWidth","1.0f"),("HalfHeight","1.0f"),
                   ("CollisionOffsetX","-1.0f"),("CollisionOffsetY","-1.0f")):
    assert f"kSourcePlayerBase{name}={value};" in h,(name,value)
scene=(R/"source/main.cpp").read_text()
assert "preview_rules_.collision_half_width = 5.0f" not in scene
assert "preview_rules_.collision_half_width=5.0f" not in scene
assert "kSourcePlayerBaseCollisionOffsetX" in scene
print("PASS: original 2x2 PlayerBase193 collider and bottom-right hotspot used")
