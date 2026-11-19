#!/usr/bin/env python3
"""Map original inventory GUI artwork to the exact Construct sprite objects."""
import json,re
from pathlib import Path
root=Path(__file__).resolve().parents[2]
j=json.loads(re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)","-",(root/"data/stage1/construct_project_parsed.json").read_text(encoding="utf-8-sig")))
objects=j["project"][3]
names={
    "InventoryBackground":"gui_inventory-sheet0.png",
    "InventoryCard":"gui_inventory_card-sheet0.png",
    "EquipmentVest":"gui_inv_card_vest-sheet0.png",
    "EquipmentCoat":"gui_inv_card_outerwear-sheet0.png",
    "EquipmentBackpack":"gui_inv_card_backpack-sheet0.png",
    "EquipmentSatchel":"gui_inv_card_kuabao-sheet0.png",
    "EquipmentFirearm":"gui_firearm-sheet0.png",
    "Slot":"gui_slot-sheet0.png",
    "SlotBackground":"gui_slot_bg-sheet0.png",
    # Original source empty equipment-slot pictograms, NOT invented labels.
    "HelmetPlaceholder":"gui_item_helmet-sheet0.png",
    "OuterwearPlaceholder":"gui_item_outerwear-sheet0.png",
    "VestPlaceholder":"gui_item_vest-sheet0.png",
    "PantsPlaceholder":"gui_item_pants-sheet0.png",
    "BackpackPlaceholder":"gui_item_backpack-sheet0.png",
    "DeathScreen":"dead_screen-sheet0.png",
    "RestartButton":"btn_restart-sheet0.png",
    "NightOverlay":"night_overlay-sheet0.png",
    "RainFade":"rain_fade-sheet0.png",
}
# Dimensions come directly from the original Construct frame records, not
# guessed UI sizing. Catch accidentally mapping the wrong image/sprite.
expected_dimensions={
    "InventoryBackground":(419,283),
    "InventoryCard":(57,54),
    "EquipmentCoat":(91,62),
    "EquipmentVest":(91,62),
    "EquipmentBackpack":(216,62),
    "EquipmentSatchel":(216,62),
    "EquipmentFirearm":(167,55),
    "Slot":(30,30),
    "SlotBackground":(32,32),
}
found={}
for i,obj in enumerate(objects):
    animations=obj[7] if len(obj)>7 and isinstance(obj[7],list) else []
    for ai,anim in enumerate(animations):
        frames=anim[7] if len(anim)>7 and isinstance(anim[7],list) else []
        for fi,fr in enumerate(frames):
            if not isinstance(fr,list) or not fr or not isinstance(fr[0],str): continue
            for name,source in names.items():
                if Path(fr[0]).name==source:
                    if name in expected_dimensions:
                        actual=(int(fr[4]),int(fr[5]))
                        expected=expected_dimensions[name]
                        # The Construct outerwear/vest animation reuses the
                        # same PNG for native 91x62 and 92x62 frame crops.
                        # Both values come from the original export.
                        valid=(actual==expected or
                            (name in ("EquipmentCoat","EquipmentVest")
                             and actual==(92,62)))
                        assert valid, (
                            f"Original {name} GUI frame size changed",
                            actual,expected)
                    found.setdefault(name,(i,ai,fi))
optional={"DeathScreen","RestartButton","NightOverlay","RainFade",
          "HelmetPlaceholder","OuterwearPlaceholder","VestPlaceholder",
          "PantsPlaceholder","BackpackPlaceholder"}
missing=set(names)-set(found)-optional
if missing:raise SystemExit(f"Original inventory art not in original Construct frames: {missing}")
out=root/"include/minidayz/generated/inventory_ui_art.hpp"
out.parent.mkdir(parents=True,exist_ok=True)
lines=["#pragma once","#include <cstdint>","namespace minidayz::stage1 {",
       "struct OriginalUiArt {std::uint16_t object,animation;std::uint32_t frame;};"]
for missing_optional in optional:
    found.setdefault(missing_optional,(65535,0,0))
for name,(object_id,animation,frame) in found.items():
    lines.append(f"inline constexpr OriginalUiArt kOriginal{name}Art{{{object_id},{animation},{frame}}};")
lines.append("} // namespace minidayz::stage1")
out.write_text("\n".join(lines)+"\n",encoding="utf-8")
print("Mapped original 3DS inventory sprites:",found)
