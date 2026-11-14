# Stage 2 player equipment port

The native player equipment layer now uses the original 1.6.0 item IDs and `MDZItemTable` sprite-object mappings rather than substitute icons.

## Equipment slots

The source-backed wearable/held slots are:

- Head: original masks and headwear (IDs 133-136, 250-277, 748, 787).
- Outerwear: original jackets/clothing and full-body protective suits (IDs 350-368, 702-703, 409-415, 711, 713).
- Vest: original vests and heavy armor vest (IDs 400-405, 408).
- Pants: original pants (IDs 300-307).
- Hands: original gloves (IDs 137, 701, 757).
- Feet: original boots/shoes (IDs 139-141, 700).
- Weapon: original firearms and melee weapons (IDs 103, 107-108, 151-216, 230-246, 785).

Backpack remains the original dedicated inventory equipment slot because it changes the active inventory capacity. It is not duplicated in the seven-slot clothing/weapon structure.

## Runtime behavior

- Dragging a single compatible item onto a slot equips it.
- Replacing equipment returns the previous item to inventory atomically.
- Tapping an occupied slot returns its item to a free/merge-compatible slot.
- Equipment state is included in save version 3. Version 1 player-only and version 2 player-plus-inventory saves remain readable.
- Player rendering draws the source player sprite and then the equipment object's exact original animation by animation name (`idle_*` / `run_*`), following the original player layer order. This handles objects whose animation indices differ from the base player object.
- The source item sprite catalog supplies the exact Construct object used for each equipped visual.

## Still required before Stage 2 completion

This is a working equipment-slot and visual-layer foundation, not a claim that all equipment effects are finished. Exact protection values, temperature insulation, clothing condition/durability, backpack-specific capacity values, and all original item-use effects must still be translated from their original Construct instance variables and event actions. They must not be guessed.
