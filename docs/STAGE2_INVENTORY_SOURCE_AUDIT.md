# Stage 2 — Original MiniDAYZ Inventory: Source UI vs Nintendo 3DS

## Original-source evidence (repository root only)

The original `data.deobfuscated.js` -> `data/stage1/construct_project_parsed.json` project has separate `GUI_layout` artwork and `Game_events -> GUI` interaction groups. The original graphical sprites are **not** generic rectangles. The original Construct sprite frames contain:

| Source sprite PNG | Frame dimensions | Role |
|---|---:|---|
| `gui_inventory-sheet0.png` | 419 × 283 | Inventory backdrop |
| `gui_inventory_card-sheet0.png` | 57 × 54 | Inventory equipment/card panel |
| `gui_inv_card_outerwear-sheet0.png` | 91/92 × 62 | Outerwear and related equipment cards |
| `gui_inv_card_vest-sheet0.png` | 91/92 × 62 | Vest equipment card |
| `gui_inv_card_backpack-sheet0.png` | 216 × 62 | Backpack equipment panel |
| `gui_inv_card_kuabao-sheet0.png` | 216 × 62 | Satchel equipment panel |
| `gui_slot-sheet0.png` | 30 × 30 | Source inventory item slot |
| `gui_slot_bg-sheet0.png` | 32 × 32 | Source drag/selection highlight—not the permanent default slot background |

The source `Game_events -> GUI` includes:
- `GUI` UID **3771661017579915**.
- `Gui_drag` UID **795480497256684**: source drag/highlight/control event branch and replacement-related functions.
- `Mobile_GUI` UID **7016557248179007**: mobile control choices and inventory close/recheck branches.
- `Pickpile` UID **4618141206649221**: ground pickup UI and related events.
- `Info_popup` UID **162109377509535**: original item popup flow.
- `Sub_menu` UID **7834464419753373**: original menu interactions.

These are original event UIDs, **not** claims that every original event has been ported.

## Corrections made in the 3DS renderer

The user-supplied emulator screenshot previously showed a stretched bright-cyan 5×3 grid and plain equipment boxes. That was not visually equivalent to the original game's textured inventory.

- `include/minidayz/inventory_touch_layout.hpp` defines **shared** 320×240 UI geometry for both rendering and touch hit-testing.
- Main equipment cards now preserve original sprite aspect ratios (coat/vest ~92×62 and backpack/satchel 120×34 scale of 216×62), instead of forcing seven identical 76×36 boxes.
- The 15-item-page backpack renders original **30×30 `gui_slot`** source frames in an 8×2 arrangement; the final 16th position is intentionally unused. This is a **3DS reflow**, not an original pixel-for-pixel layout.
- Item textures resolve by their original MDZItemTable IDs, with stack counts only when greater than one and source-derived condition/durability.
- Original outerwear and vest card panels display source-derived condition, HEAT and ARMOR information where defined.
- The cyan `gui_slot_bg` is not used as a permanent default item-cell background.
- Dragging, touch actions, and backpack pagination use hitboxes from the same layout used for drawing.
- The original 419:283 inventory background is displayed at proportional 320×216 within the 320×240 touchscreen.

## Verification and still-open parity questions

Build-level checks are in `tests/inventory_touch_layout_tests.cpp`, `tests/test_original_inventory_ui.py` and the sprite geometry assertions in `tools/stage2/build_inventory_ui_art.py`. These checks run in the 3DS and native GitHub Actions workflows.

**Status: SOURCE GRAPHICS REPAIRED; FULL ORIGINAL INTERACTION PARITY NOT CERTIFIED.**

The original GUI has separate equipment pockets, item popup and ground-pickpile workflows. The current 3DS touch adaptation still needs direct original-vs-emulator testing for drag origin/target rules, equipment pocket mapping, capacity changes, all source item-condition branches, edge scrolling, use/alt-use and the Pickpile and Sub_menu event hierarchies. The original game has a much larger canvas; identical pixel coordinates are impossible without reflowing the interface.
