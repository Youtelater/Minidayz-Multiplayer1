# Stage 2 original-source audit

- Authoritative original source: `Minidayz-Multiplayer-main/data.js` (original deobfuscated Construct export in this repository).
- Verified source size: 12,442,669 bytes; SHA-256: `1678ecbfb4e2cb50d431afe732772f5ef9fc86e27ed6fe96de4f8fbdfb045b75`.
- Original project title/version: Mini DAYZ 1.6.0.
- The original source contains the `Check_stack_size` event and `perk_cybb` branch.
- Original item IDs and Construct object names are mapped by `MDZItemTable`; localized names are in `l_eng_items.xml`.
- The source-backed native item catalog currently preserves 405 item IDs and gaps.
- The original source identifies a dedicated `Backpack` equipment slot and the `player_backpacks` visual layer.

Do not infer item effects or backpack slot counts from item names. The raw source event expressions remain the authority for those values; add native constants only after extracting the matching source expression and event UID.


## Verified build outputs

The native CI run generated 405 original item definitions, resolved 329 item-to-Construct sprite mappings, and extracted 3,680 original inventory behavior event records. The native regression workflow passed on commit `61e84b901f5845b49aadf07f3001a9f180b2d4de`; 3DS build verification for the latest source-path corrections is pending. The 76 item IDs without an `MDZItemTable` mapping remain explicitly unresolved; no substitute art or item effects are fabricated.


## Original companion-file and artwork inventory

- The authoritative item object table is `Minidayz-Multiplayer-main/mp_itemtable.js`; the source-backed item names, UI strings, and player feedback are in `l_eng_items.xml`, `l_eng_ui.xml`, and `l_eng_log.xml` respectively. The repository contains 28 original `l_*.xml` localization files across the supported languages.
- The repository contains 1,854 PNG files under the original `assets/gfx` artwork directory. The Construct sprite-frame extractor resolves original frame references against that directory and fails on missing or byte-conflicting candidates rather than silently substituting artwork.
- The Makefile's `stage1-assets` target rebuilds the snapshot from the original `data.js`, extracts directly from that same file, validates the generated source inventory, and builds the original sprite and item catalogs. The item behavior, effect, and equipment catalogs are also part of this target.

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
