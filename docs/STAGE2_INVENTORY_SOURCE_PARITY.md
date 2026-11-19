# Stage 2 — Original MiniDAYZ Inventory on Nintendo 3DS

This file records what is **implemented and verified in native C++** on the 3DS and what still requires source event and emulator parity. The original Construct source is `data.deobfuscated.js` / `data/stage1/construct_project_parsed.json`; original art comes only from root `assets/gfx/` converted to `romfs/gfx/`. Item art IDs come from root `mp_itemtable.js` and Construct sprite records. These files **do not replace the single-player gameplay event engine**.

## Screen rule
- **TOP display (400×240):** game world, survivor character, original worn gear and active held rifle/melee overlay; no inventory panels.
- **BOTTOM touchscreen (320×240):** original inventory backdrop, original equipment cards, source slot artwork, backpack/satchel pages, inventory controls, descriptions, crafting, storage and perks.
- Original texture aspect ratios are retained where possible; native layout is a **resolution adaptation**, not the literal original phone-screen coordinates.

## Implemented systems
1. **Item identity:** root `mp_itemtable.js` original Construct object IDs and `l_eng_items.xml` item names and item descriptions; no guessed replacement pictures.
2. **Independent source equipment slots:** helmet, outerwear, vest, pants, melee tool (`EquipmentSlot::hands`), firearm (`EquipmentSlot::weapon`), main backpack and satchel. Firearm codes 151–199/230–246 etc. and melee codes 200–216 are separate according to original root item categories and artwork. The 3DS portrait firearm card displays original rifle art rotated without squashing. Both a pickaxe and a firearm can be carried simultaneously.
3. **Backpack:** native 15 base slots, authentic 30×30 `gui_slot` art, source backpack/satchel capacity and ID whitelist, 15 visible cells/page, source condition and stack visuals.
4. **Item actions:** primary/secondary Use, drop and pickup, half-stack split, move/merge/swap, select and item details. Exact source item/equipment durability and hundredths are preserved.
5. **Equipment:** touch to select worn item without unexpectedly removing it; drag to a valid backpack slot to unequip, drag inventory to the correct original equipment category, and source-defined clothing repairs. Original item conditions, fractional wear and bags remain transactional.
6. **Storage:** native tent/stash transfers and pages within the bottom screen. Exact slot hitboxes reject gutters/out-of-bounds taps, show source `gui_slot` texture, and play item transfer sound on successful transactions.
7. **Crafting:** source-extracted item IDs, ingredient totals, stack-specific actions, repair tools preserved where specified, destination inventory vs ground, verified source wait actions, transactional rollback.
8. **Audio:** original `romfs/wav/` item pickup, drop, equipment drop, zipper and crafting sounds have real native NDSP triggers; not every original audio event is proven equivalent.
9. **Saving:** source IDs, inventory storage and independent melee/rifle equipment are stored in the existing native save format. Old incorrect 3DS saves are migrated without losing gear/condition, including swaps of both categories.
10. **GUI details:** scrollable original item descriptions for backpack items and worn gear; original armor, warmth and condition information is displayed on equipment cards.

## Source event coverage — still not full parity
The source event ledger produced by `tools/stage2/audit_original_inventory_events.py` verifies exact original group UIDs and a large event tree:
- `Using inventory items`, UID **3544291115846117**: hundreds of nested event actions, some require world, AI, combat or vehicles and are not all implemented in the native item-use system.
- `Picking inventory items` UID **1529638233252698** and `Picking player items` UID **115345084350351**.
- `Droppin items` UID **594580094599696**.
- `Gui_drag` UID **795480497256684** and `Pickpile` UID **4618141206649221**.
- `Stash` UID **709191320420159**, `Sub_menu` UID **7834464419753373**, `Interaction` UID **970660140616416**.

Do not confuse `data/stage2/inventory_original_event_groups.json` or compilation with a completed port of all Construct branches.

## Automated verification
- `tests/inventory_touch_layout_tests.cpp`: 8 independent equipment hitboxes, true 30×30 backpack slots, 30 tent/stash touch cells, gutters/outside bounds.
- `tests/equipment_system_tests.cpp`: simultaneous firearm and melee, original protection, repairs, durability, save migration including a reversed old pairing.
- `tests/save_game_roundtrip_tests.cpp`: native gear/inventory persistence with exact fractional rifle wear.
- `tests/inventory_system_tests.cpp`, `tests/inventory_fractional_wear_tests.cpp`, `tests/crafting_system_tests.cpp`, `tests/item_use_tests.cpp`, `tests/tent_storage_tests.cpp`, `tests/stash_storage_tests.cpp` and source crafting/gear tests.
- `tests/test_original_inventory_ui.py`: source sprite links, lower-screen panel boundary, equipment interaction safety and original PNG references.
- GitHub Actions must pass both native and `.3dsx` jobs before release.

**Unresolved before declaring 100% source parity:** execute/compare the original item-use and drag/interaction branches not yet translated, validate every original UI animation/audio trigger, confirm item and scene contexts with a browser-vs-3DS gameplay trace, and inspect fresh Manic Emu screenshots of the rebuilt interface. This project does not gain full original gameplay parity merely because its inventory C++ tests pass.
