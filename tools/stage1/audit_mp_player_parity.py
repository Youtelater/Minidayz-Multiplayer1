#!/usr/bin/env python3
"""Audit 3DS player equipment against root mp_players.js (static, never execute JS).

mp_players.js is a multiplayer overlay, NOT the authoritative singleplayer
movement engine. Its obfuscated Construct instance indexes must never be used
as native EquipmentSlot enum values.
"""
import json
import re
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
mp=(ROOT/"mp_players.js").read_text(encoding="utf-8-sig")
mp=re.sub(r"\\u([0-9a-fA-F]{4})",
          lambda m: chr(int(m.group(1),16)),mp)
header=(ROOT/"include/minidayz/equipment.hpp").read_text(encoding="utf-8")
cpp=(ROOT/"source/main.cpp").read_text(encoding="utf-8")
engine=(ROOT/"source/player.cpp").read_text(encoding="utf-8")
out=ROOT/"data/stage1/mp_player_parity.json"

def object_body(name):
    match=re.search(r"\\b"+re.escape(name)+r"=\\{([^;]+)\\};",mp)
    if not match:
        raise AssertionError("Missing original MP equipment table: "+name)
    return match.group(1)

raw=object_body("_xda7d")
slot_expr=dict(re.findall(r'"(firearm|melee|head|backpack|vest|body|legs|quick)":(\\d+\\^\\d+)',raw))
expected={"firearm","melee","head","backpack","vest","body","legs","quick"}
assert set(slot_expr)==expected,sorted(set(slot_expr)^expected)
slot_indices={name:(int(a)^int(b)) for name,expr in slot_expr.items()
              for a,b in [expr.split("^")]}
assert len(set(slot_indices.values()))==len(slot_indices),slot_indices
drops=dict(re.findall(r'"(firearm|melee|quick|backpack|vest|body|head|legs)":\\{[^{}]*?"drop":"(Drop_[^"]+)"',object_body("_x4c865e")))
# The MP inventory uses named Construct inventory instance variables and
# drop handlers. The native singleplayer port stores item IDs instead.
assert drops=={"firearm":"Drop_firearm","melee":"Drop_melee",
 "quick":"Drop_pistol","backpack":"Drop_backpack","vest":"Drop_vest",
 "body":"Drop_outerwear","head":"Drop_helmet","legs":"Drop_pants"},drops
native={"firearm":"weapon","melee":"hands","head":"head","backpack":"backpack",
        "vest":"vest","body":"outerwear","legs":"pants"}
for name in ("head","outerwear","vest","pants","hands","weapon"):
    assert re.search(r"\\b"+name+r"\\b",header),("Missing native slot",name)
assert "equipped_backpack()" in cpp
assert "gear_slot_for_card(" in cpp
assert "EquipmentSlot::weapon" in cpp and "EquipmentSlot::hands" in cpp
assert "const auto target = gear_slot_for_card(drag_target_equipment_slot_);" in cpp
assert "source_animation_index()" in engine
assert "draw_equipped_layer(held_item,true" in cpp
# MP quick/pistol is an extra multiplayer slot, not automatically the native
# weapon slot. Do not claim that it already works or silently collapse it.
unsupported=["quick/pistol separate MP inventory slot"]
result={
 "source":"root mp_players.js",
 "role":"multiplayer Construct player overlay (source comparison only)",
 "mp_construct_slot_variables":slot_indices,
 "mp_drop_handlers":drops,
 "native_slot_equivalents":native,
 "not_implemented_as_mp_features":unsupported,
 "checks":"MP categories and existing 3DS equipment separation verified; not emulator parity"
}
out.parent.mkdir(parents=True,exist_ok=True)
out.write_text(json.dumps(result,indent=2,sort_keys=True)+"\\n",encoding="utf-8")
print("PASS: MP player equipment references mapped to native 3DS categories")
print("UNPORTED:",", ".join(unsupported))
