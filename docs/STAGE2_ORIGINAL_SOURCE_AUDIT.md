# Stage 2 original-source audit

- Authoritative original source: `data.deobfuscated.js` (Library file, 14,558,316 bytes).
- SHA-256: `b5e15cfad3106b56457bc13c4169e59b4ff0308035912330bf144b86238571d3`.
- Original project title/version: Mini DAYZ 1.6.0.
- The original source contains the `Check_stack_size` event and `perk_cybb` branch.
- Original item IDs and Construct object names are mapped by `MDZItemTable`; localized names are in `l_eng_items.xml`.
- The source-backed native item catalog currently preserves 405 item IDs and gaps.
- The original source identifies a dedicated `Backpack` equipment slot and the `player_backpacks` visual layer.

Do not infer item effects or backpack slot counts from item names. The raw source event expressions remain the authority for those values; add native constants only after extracting the matching source expression and event UID.


## Verified build outputs

The native CI run generated 405 original item definitions, resolved 329 item-to-Construct sprite mappings, and extracted 2,223 original inventory behavior event records. Both the native regression workflow and the 3DS executable build completed successfully after the inventory fixes. The 76 item IDs without an `MDZItemTable` mapping remain explicitly unresolved; no substitute art or item effects are fabricated.


## Player appearance and equipment layers verified in original source

The original Construct object table defines separate player visual layers named
`player_pants`, `player_hands`, `player_clothing`, `player_vests`,
`player_helmets`, `player_backpacks`, and `player_weapons`. The source
also runs `animation_redraw` after equipment-layer updates. These are distinct
from the inventory grid and must be represented by the native player/equipment
runtime; they are not interchangeable generic clothing slots.

The native player runtime now exposes a validated `PlayerState` snapshot and
restore path so position, survival stats, animation state, and life state can
be persisted. Restore rejects non-finite values, invalid enum values, and
inconsistent alive/dead state without mutating the active player.

## Stage 2 completion boundary

Player save-state support does not complete Stage 2. The native runtime still
needs source-derived item-to-equipment-layer classification, equipment
transfers between inventory and player layers, visual redraw integration,
source-backed clothing/armor effects, exact backpack capacity rules, and
item-use effects. Those values must be extracted from the original event
conditions/actions before implementation; item-name guesses are not acceptable.


## Original root and bundled source inventory

The port must use the original source files together, not only the deobfuscated event export:

- `data.deobfuscated.js` and `data.js`: original Construct project data and event/object definitions used by the Stage 1 extraction pipeline.
- `Minidayz-Multiplayer-main/c2runtime.js`: original Construct runtime behavior reference.
- `mp_itemtable.js`: original item-ID to Construct object-name mapping.
- `Minidayz-Multiplayer-main/l_eng_items.xml`: original English item names and IDs (405 entries).
- `Minidayz-Multiplayer-main/l_eng_ui.xml`: original English UI labels and menu text.
- `assets/gfx/`: original PNG sprite sheets, including the original `gui_*` controls, panels, status meters, inventory cards, equipment slots, and buttons.
- `romfs/gfx/`: converted 3DS texture assets used by the renderer.

The Stage 2 build now generates `include/minidayz/generated/ui_catalog.hpp`
from the original `l_eng_ui.xml`. The native main-menu Continue and New Game
labels read IDs 2 and 1 from that generated catalog instead of duplicating
English strings in C++.

## UI implementation boundary

The original UI artwork is present in the project and the UI localization
catalog is now source-generated. That does **not** mean the original UI has
been fully ported. The current lower-screen inventory/equipment view and
top-screen menu are still a native preview layout; the original Construct UI
panels, status indicators, popups, interaction controls, map/perk/quest flows,
and their event-driven visibility/behavior still require porting and testing.
Use the original UI XML, `gui_*` sprite sheets, Construct object table, and
event conditions/actions as the references for that work. Do not substitute
generic panels or guessed text/IDs.
