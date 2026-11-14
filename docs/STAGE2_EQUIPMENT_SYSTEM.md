# Stage 2 player equipment port

The native player equipment layer uses original 1.6.0 item IDs and `MDZItemTable` sprite-object mappings rather than substitute icons.

## Source-backed equipment slots

- Helmet: IDs 133-136, 250-277, 748, 786, 787.
- Outerwear: IDs 350-368, 702-703, 409, 413-415, 711, 713.
- Vest: IDs 400-405, 408, and CBRN suit IDs 410-412.
- Pants: IDs 300-307.
- Hands / held weapon: original firearm and melee IDs 103, 107-108, 151-216, 230-246, 785.
- Backpack remains a dedicated equipment slot. The original `Check_space_backpack`, `Check_item_backpack`, and `Drop_backpack` event paths are now captured for translation; no per-backpack capacity is asserted until those source rules are fully decoded.

The original player damage event subtracts four flat protection values. The generator now emits 63 source equipment-property rows from original Map/TD item instances. It reads variable 8 for clothing warmth and variables 12, 13, 14, and 15 for helmet, outerwear, vest, and pants protection respectively. These values are emitted per item ID and summed by EquipmentSystem::protection(). PlayerSystem has an armor-aware damage overload that applies the original flat reduction. Clothing warmth is also extracted from item variable 8; EquipmentSystem::temperature_offset() ports the original Player_temperature term `(outerwear - helmet) / pants`, excluding the separate Day_cold value. Separate footwear and glove equipment slots are not invented; items without an original equipment slot are rejected.

## Runtime behavior

- Dragging a single compatible item onto one of the five source equipment slots equips it.
- Replacing equipment returns the previous item to inventory atomically.
- Tapping an occupied clothing/held slot returns its item to a free/merge-compatible inventory slot.
- The backpack appears in the equipment row. The native inventory can now drop an equipped backpack into a ground-item record, preserves its condition, and refuses to remove it while extended slots contain items. Capacity-changing equip behavior remains gated until the exact original storage rules are decoded; the native lookup deliberately returns unresolved rather than inventing a slot count. Item condition (original instance variable 6) is preserved through inventory stacks, ground transfers, equipment swaps, backpack swaps and save/load. The source map provides default condition values for 168 item IDs; unrepresented IDs currently fall back to 100 until their original instance data is located.
- Equipment and per-item condition are included in save version 4; player-only, inventory-only and version-3 equipment saves are migrated.
- Player rendering draws original sprite objects by animation name, then layers pants, outerwear, vest, helmet, backpack, and held weapon.
- The generated source sprite catalog provides the exact Construct object for each mapped item.

## Still required before Stage 2 completion

Item condition storage and persistence are implemented using original instance variable 6; the source condition loss/repair rules still need to be ported. Backpack storage/capacity semantics and the remaining item actions still need their original Construct semantics and end-to-end save/load verification. The armor-aware damage API is ready, but it still needs to be connected to the Stage 3 NPC hit path when that combat path is implemented. The temperature offset still needs to be combined with the source Day_cold/weather value. Values must not be guessed.