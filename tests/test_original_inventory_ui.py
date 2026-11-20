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
# The original gui_slot sprite SHEET contains item icons (the first is a
# soda can), NOT an empty slot texture. Never paint it on empty cells.
assert "kOriginalSlotArt" not in src
assert "renderer.draw_item_icon(item->item_id," in src
# The original cyan gui_slot_bg is a drag selection graphic, not the
# parchment base, so the new square cells deliberately do not use it.
assert "kOriginalSlotBackgroundArt" not in src.split(
    "// Original GUI 'gui_slot' is a 30x30 PIXEL square.",1)[1].split(
    "const std::size_t page_count",1)[0]
grid=src.split("// Original GUI 'gui_slot' is a 30x30 PIXEL square.",1)[1].split(
    "const std::size_t page_count",1)[0]
assert "kOriginalInventoryCardArt" not in grid, (
    "Backpack slots must not be composed from full equipment cards")
assert "if(selected || drag_target_slot_==index)" in grid
assert "if(item->count>1)" in grid
assert "minidayz::items::ui::kSlotSize" in src
assert "minidayz::items::ui::kEquipment" in src
assert "minidayz::items::ui::kActions" in src
assert "minidayz::items::ui::kPageNavigation" in src
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
# A stylus drag that returns to its starting cell is NOT a second tap:
# otherwise a consumed medical item or food can activate accidentally.
assert "tap_was_selected_ && !drag_moved_)" in src
# Source equipment requires a genuine stylus drag into a visible storage cell.
# A stationary touch selects worn gear for inspection, not auto-unequip.
assert "drag_moved_ && drag_target_slot_>=0" in src
assert "selected_equipment_slot_=equipment_target" in src
assert "if(source<5 || source==7 || source==8)" in src
assert "equip_quick_pistol_from_inventory" in src
assert "unequip_quick_pistol_to_inventory" in src
assert "active_quick_pistol_" in src
assert "gear_slot_for_card" in src
# Equipment card 7 is the firearm UI position, but Weapon is native
# EquipmentSlot 6. An unconverted cast makes guns impossible to equip.
assert "const auto target = gear_slot_for_card(drag_target_equipment_slot_);" in src, (
    "Stylus equipment drops must translate firearm card 7 to Weapon slot 6")
assert "static_cast<minidayz::items::EquipmentSlot>(\n                    drag_target_equipment_slot_)" not in src
# Item details for the selected firearm must not index a seven-element
# equipment condition array using the UI card number 7.
assert "equipment_.snapshot().conditions[gear_index_for_card(slot)]" in src, (
    "Selected firearm details must read the weapon condition from slot 6")
assert "equipment_.snapshot().conditions[static_cast<std::size_t>(slot)]" not in src
assert "kOriginalInventoryCardArt" in src
# World visuals stay on the TOP 400x240 display. Every inventory,
# equipment, crafting, item details and storage card is rendered solely
# inside Scene::render_bottom for the 320x240 touchscreen.
top=src.split("    void render(minidayz::Renderer& renderer) noexcept override {",1)[1].split(
    "\nprivate:\n",1)[0]
bottom=src.split("    void render_bottom(minidayz::Renderer& renderer) noexcept override {",1)[1].split(
    "    void render(minidayz::Renderer& renderer) noexcept override {",1)[0]
assert "kOriginalSlotArt" not in bottom
assert "kOriginalEquipmentBackpackArt" in bottom
assert "kOriginalEquipmentSatchelArt" in bottom
assert "if (crafting_open_)" in bottom
assert "if (storage_open_)" in bottom
assert "if (details_open_)" in bottom
assert "kOriginalSlotArt" not in top
assert "active_held_slot_" in top

assert "1.5707963268f" in src, "Original rifle art must rotate into portrait weapon card"
assert "active_held_slot_" in src, "Separate firearm/melee mode must control player stance"
assert "std::uint16_t item_id=selected && !selected->empty()" in src
assert "source_item_description(item_id)" in src
assert "details_scroll_+=3" in src
assert "drag_touch_x_=input.touch.px" in src
assert "drag_touch_y_=input.touch.py" in src
assert "if(drag_moved_ && (drag_source_slot_>=0 ||" in src
assert "27.0f,27.0f,0.98f" in src, "Original item must follow touchscreen drag in foreground"
assert "stage2_audio_.play_item_pickup();" in src
# Tent/stash cells must not display a soda can in empty cells; real items
# render only when their inventory slot is populated.
storage=src.split("        if (storage_open_) {\n            // The tent item",1)[1].split("        if (crafting_open_) {",1)[0]
assert "kOriginalSlotArt" not in storage
assert "if (!item || item->empty()) continue;" in storage
assert "storage_at(" in src
assert "ui::kEquipment" in src
# Original artwork must not vanish under its own fallback panel.
assert "C2D_Color32(70, 60, 48, 255),0.01f" in src
print("PASS: original inventory art, selection, equipment drag and depth checks")
