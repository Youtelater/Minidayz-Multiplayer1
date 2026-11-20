# Original MiniDAYZ graphical source review — 20 Nov 2026

This review examines the **actual pixel content** of all **1,854** PNG
files in `assets/gfx` using 12 generated contact sheets. These sheets,
their exact filename/dimension index, and a reproducible generator are
available as the GitHub Actions artifact
`minidayz-original-gfx-visual-review`.

## Counts (filename-based grouping only)

| Group | Images |
|---|---:|
| Menu artwork | 21 |
| Interface and minimap | 235 |
| Original player animation sheets | 81 |
| Tilemaps, ground, trees, bridges | 40 |
| Buildings, environmental and vehicle structures | 212 |
| Zombies, NPCs, animals | 241 |
| Achievements and named item icons | 36 |
| All other original sprites (including equipment and weapons) | 988 |
| **Total** | **1,854** |

All original PNGs are represented by corresponding original-named
(normalized where necessary) `.t3x` files in `romfs/gfx`. This
establishes conversion coverage **only**, not correct display mapping.

## Confirmed visual distinctions

1. `menu_btn_wood-sheet0.png` is a **packed multi-state sprite atlas**,
   not a single button. Stretching the full 256x256 source texture across
   a menu row produces the multicolor stripes visible in Manic Emu.
   `tools/stage1/build_menu_button_frame.py` currently crops an
   individual source frame; verify that the actual running build loads
   `romfs:/gfx/menu_btn_wood_single.t3x`, rather than the atlas.
2. `gui_slot-sheet0.png` is an **item icon atlas**, not an empty
   backpack cell. Drawing its first frame in every empty cell yields
   repeated canned drinks. Actual blank slot/card artwork is separate:
   `gui_inventory_card-sheet0.png`, `gui_slot_bg-sheet0.png`, and
   other original GUI assets. Verify the **running** build uses the
   corrected inventory path, not old atlas-first-frame drawing.
3. `gui_inventory-sheet0.png` contains the original inventory
   panel/background; `gui_item_helmet`, `gui_item_outerwear`,
   `gui_item_vest`, `gui_item_pants`, and `gui_item_backpack`
   contain equipment images. Do not render these without verifying
   frame selection and the native 320x240 coordinate transform.
4. `ground_lvl1_tilemap.png` through `ground_lvl7_tilemap.png`,
   `tilemap_forest.png`, `tilemap_water.png`,
   `tilemap_sand.png`, and `ground_enviroment_tilemap.png`
   contain original tile atlases. They require tile indices and source
   placements; displaying a whole atlas or a repeated single tile
   is **not** equivalent to recreating the original map.
5. Numerous `b_*.png` images are **building exteriors or interiors**
   (city houses, hospitals, military, villages, shops). They are not
   missing from the repository; runtime positioning and visibility
   must be verified against original Construct generation events.
6. `player_skin_*-sheet*.png` are multi-frame directional
   animation sheets. Choosing the wrong region or whole sheet
   destroys animation fidelity.
7. `minimap_tile`, `minimap_mark`, `minimap_player`, and
   `minimap_dragger` are separate original assets. The user's
   screenshot shows that the current rectangular native minimap layout
   is still a **simplification**, not a complete original minimap port.

## Emulator evidence supplied by the user

- Main-menu rows contain repeated colored bars, unlike the intended
  wooden-button artwork.
- Bottom inventory shows repeated cans, distorted labels, and different
  card geometry from the original game.
- The map is usable but visually simplified.
- The world has original ground and some objects; visual parity of all
  roads, trees, buildings, and source tilemaps is not established.

**Do not claim visual fidelity** merely because CI compiles, source
spritesheets exist, or the asset converter processes all PNGs.

## How to inspect the actual full set

Run `python3 tools/stage1/build_original_graphics_review.py` with
Pillow installed. Open the 12 `data/graphics_review/contact_*.jpg`
files in order and consult `all_original_images.csv` to resolve
each thumbnail index to its full original filename and pixel dimensions.

This is a source-art review. It makes **no changes** to the gameplay
renderer, Construct event handling, or T3X image contents.
