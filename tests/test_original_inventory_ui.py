#!/usr/bin/env python3
"""Source-art and touch-integrity regression for MiniDAYZ's 3DS inventory.

The underlying artwork is authored in the supplied root Construct project;
the 3DS 320x240 card positions are an adaptation, not original pixel parity.
"""
from pathlib import Path
R=Path(__file__).resolve().parents[1]
src=(R/"source/main.cpp").read_text(encoding="utf-8")
builder=(R/"tools/stage2/build_inventory_ui_art.py").read_text(encoding="utf-8")
for filename in (
    "gui_inventory-sheet0.png",
    "gui_inventory_card-sheet0.png",
    "gui_inv_card_vest-sheet0.png",
    "gui_inv_card_outerwear-sheet0.png",
    "gui_inv_card_backpack-sheet0.png",
    "gui_slot-sheet0.png",
    "gui_slot_bg-sheet0.png",
):
    assert filename in builder, f"Missing original inventory art mapping: {filename}"
    assert (R/"assets/gfx"/filename).exists(), f"Missing original root PNG: {filename}"
# gui_slot is the original backpack cell, not the bright cyan highlight
# or a second full-size equipment card.
assert "ui_art(minidayz::stage1::kOriginalSlotArt," in src
assert "ui_art(minidayz::stage1::kOriginalSlotBackgroundArt," in src
grid=src.split("// 5 x 3 source-sprite backpack cells",1)[1].split(
    "const std::size_t page_count",1)[0]
assert "kOriginalInventoryCardArt" not in grid, (
    "Backpack slots must not be composed from full equipment cards")
assert "if (selected)" in grid
assert "if(item->count>1)" in grid
# Touch down must not instantly unequip gear, or original item condition
# and drop targets are lost before the release event.
press=src.split("const int equipment_target = equipment_slot_at",1)[1].split(
    "// Page through larger backpacks",1)[0]
assert "drag_equipped_source_slot_=equipment_target" in press
assert "unequip_to_inventory" not in press
release=src.split("if(drag_equipped_source_slot_>=0)",1)[1].split(
    "else if (drag_source_slot_ >= 0",1)[0]
assert "unequip_to_inventory" in release
assert "unequip_backpack_to_slot" in release
assert "unequip_satchel_to_slot" in release
assert "touch_released" in src
# Original artwork must not vanish under its own fallback panel.
assert "C2D_Color32(70, 60, 48, 255),0.01f" in src
print("PASS: original inventory art, selection, equipment drag and depth checks")
