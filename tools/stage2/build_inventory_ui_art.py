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
    "Slot":"gui_slot-sheet0.png",
    "SlotBackground":"gui_slot_bg-sheet0.png",
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
                    found.setdefault(name,(i,ai,fi))
missing=set(names)-set(found)
if missing:raise SystemExit(f"Original inventory art not in original Construct frames: {missing}")
out=root/"include/minidayz/generated/inventory_ui_art.hpp"
out.parent.mkdir(parents=True,exist_ok=True)
lines=["#pragma once","#include <cstdint>","namespace minidayz::stage1 {",
       "struct OriginalUiArt {std::uint16_t object,animation;std::uint32_t frame;};"]
for name,(object_id,animation,frame) in found.items():
    lines.append(f"inline constexpr OriginalUiArt kOriginal{name}Art{{{object_id},{animation},{frame}}};")
lines.append("} // namespace minidayz::stage1")
out.write_text("\n".join(lines)+"\n",encoding="utf-8")
print("Mapped original 3DS inventory sprites:",found)
