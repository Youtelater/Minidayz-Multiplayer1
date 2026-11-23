#!/usr/bin/env python3
"""Validate original Construct player sprite layer order and native 3DS depths."""
from pathlib import Path
import json,re
R=Path(__file__).resolve().parents[1]
s=(R/"data/stage1/construct_project_parsed.json").read_text(encoding="utf-8-sig")
s=re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)","-",s)
p=json.loads(s)["project"]
map_layout=next(layout for layout in p[5] if layout[0]=="Map")
expected=["player","player_pants","player_hands","player_clothing",
          "player_vests","player_helmets","player_backpacks","player_weapons"]
assert [map_layout[6][i][0] for i in range(32,40)]==expected
h=(R/"include/minidayz/source_player_layers.hpp").read_text()
for i,name in enumerate(("Player","PlayerPants","PlayerHands","PlayerClothing",
                         "PlayerVest","PlayerHelmet","PlayerBackpack","PlayerWeapon"),start=32):
    assert f"kSource{name}Layer={i};" in h,(i,name)
scene=(R/"source/main.cpp").read_text()
assert "source_player_layer_depth(" in scene
assert "0.57f,0.0f,outlined" not in scene
assert "kSourcePlayerWeaponLayer" in scene
print("PASS source Construct player layers 32–39 match 3DS renderer and equipment")
