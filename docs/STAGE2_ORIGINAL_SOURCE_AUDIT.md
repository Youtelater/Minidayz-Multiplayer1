# Stage 2 original-source audit

- Authoritative original source: root-level `data.deobfuscated.js` (14,558,329-byte deobfuscated Construct export; the original version for this port).
- Original project title/version: Mini DAYZ 1.6.0.
- The original source contains the `Check_stack_size` event and `perk_cybb` branch.
- Original item IDs and Construct object names are mapped by root-level `mp_itemtable.js`; localized names are in root-level `l_eng_items.xml`.
- The source-backed native item catalog currently preserves 405 item IDs and gaps.
- The original source identifies a dedicated `Backpack` equipment slot and the `player_backpacks` visual layer.

Do not infer item effects or backpack slot counts from item names. The raw source event expressions remain the authority for those values; add native constants only after extracting the matching source expression and event UID.


## Verified build outputs

The native regression workflow passed on commit `9dd896b738c078b44fbe59933c01f0e4b20571f4` (run `43634698519`) after switching the pipeline back to the root-level `data.deobfuscated.js`, `mp_itemtable.js`, and localization files. Its world, item/inventory, equipment, ground-loot, item-use, NPC-spawn, and player test steps all completed successfully. The matching 3DS build is still being verified; do not mark Step 1 complete until it passes.


## Original companion-file and artwork inventory

- The authoritative item object table is root-level `mp_itemtable.js`; item names, UI strings, player feedback, and item descriptions are in root-level `l_eng_items.xml`, `l_eng_ui.xml`, `l_eng_log.xml`, and `l_eng_new.xml`. The root contains 30 original localization XML files across the supported languages.
- The root-level original artwork inventory contains 1,854 PNGs under `assets/gfx/`, 167 under `images/`, and 295 under `gfx/` (2,316 images total). The Construct sprite-frame extractor resolves exact source references against root-level artwork and fails on missing or byte-conflicting candidates rather than silently substituting artwork.
- The Makefile's `stage1-assets` target rebuilds the snapshot from root-level `data.deobfuscated.js`, extracts directly from that same file, validates the generated source inventory, and builds the original sprite and item catalogs. The item behavior, effect, and equipment catalogs are also part of this target.

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
