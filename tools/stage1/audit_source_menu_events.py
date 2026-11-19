#!/usr/bin/env python3
"""Audit original Construct Menu_Events against native Nintendo 3DS menu.

The source event sheet is authoritative. This audit verifies we did not
mistake mp_menu.js's multiplayer DOM controls for original game events.
UI parity and event/achievement parity are separate.
"""
import json
import re
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
src=ROOT/"data/stage1/construct_project_parsed.json"
project=json.loads(re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)","-",
    src.read_text(encoding="utf-8-sig")))["project"]
sheets=dict(project[6])
assert "Menu_Events" in sheets, "Missing source Construct menu sheet"
events=sheets["Menu_Events"]
groups={}
for event in events:
    if not isinstance(event,list) or len(event)<8 or event[0]!=0:
        continue
    for cond in event[5]:
        if not isinstance(cond,list) or len(cond)<10:
            continue
        # Most source menu sections are Construct comment groups (-1/38).
        # The Language section is a localStorage trigger (422/226), not
        # a comment group, but it is still the original language handler.
        if (cond[0],cond[1]) not in {(-1,38),(422,226)}:
            continue
        for parameter in cond[-1]:
            if isinstance(parameter,list) and len(parameter)>1 and isinstance(parameter[1],list):
                v=parameter[1]
                if len(v)>1 and isinstance(v[1],str):
                    groups[v[1]]={"uid":event[4],"nested_children":len(event[7]) if isinstance(event[7],list) else 0}
required={
    "new_menu","character_menu","Achievement_menu",
    "Item unlocks","Weapon_unlocks","Language","Animateds",
}
assert required<=groups.keys(),f"Missing original menu event groups: {sorted(required-groups.keys())}"
main=(ROOT/"source/main.cpp").read_text(encoding="utf-8")
menu=(ROOT/"include/minidayz/boot_menu.hpp").read_text(encoding="utf-8")
assert "BootPage" in menu and "menu_detail_open_" in main
assert "kSourceAchievementCount" in main and "kSourceUnlockCount" in main
assert "restore_continue_from_disk()" in main
assert "menu_achieves_bg-sheet0.png" in main
assert "options_menu-sheet0.png" in main
assert "boot_input_gate_.update" in main
print("PASS: original Menu_Events group provenance and source-based 3DS menu UI")
print("ORIGINAL MENU EVENT GROUPS:",json.dumps(
    {key:groups[key] for key in sorted(required)},sort_keys=True))
print("NOTE: Original achievement, item/weapon unlock and language event "
      "behavior remains a separate porting requirement; UI passing is not "
      "a certification of gameplay progression.")
