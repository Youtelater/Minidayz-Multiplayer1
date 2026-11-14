# Stage 2 player equipment port

The native player equipment layer uses original 1.6.0 item IDs and `MDZItemTable` sprite-object mappings rather than substitute icons.

## Source-backed equipment slots

- Helmet: IDs 133-136, 250-277, 748, 786, 787.
- Outerwear: IDs 350-368, 702-703, 409-415, 711, 713.
- Vest: IDs 400-405 and 408.
- Pants: IDs 300-307.
- Hands / held weapon: original firearm and melee IDs 103, 107-108, 151-216, 230-246, 785.
- Backpack remains the dedicated inventory equipment slot because it changes inventory capacity.

The original player damage event subtracts four flat protection values. The generator reads the corresponding item instance variables from the original Map/TD layouts: variable 12 for helmets, 13 for outerwear, 14 for vests, and 15 for pants. These values are emitted per item ID and summed by EquipmentSystem::protection(). PlayerSystem has an armor-aware damage overload that applies the original flat reduction. Clothing warmth is also extracted from item variable 8; EquipmentSystem::temperature_offset() ports the original Player_temperature term `(outerwear - helmet) / pants`, excluding the separate Day_cold value. Separate footwear and glove equipment slots are not invented; items without an original equipment slot are rejected.

## Runtime behavior

- Dragging a single compatible item onto one of the five source equipment slots equips it.
- Replacing equipment returns the previous item to inventory atomically.
- Tapping an occupied clothing/held slot returns its item to a free/merge-compatible inventory slot.
- The backpack appears in the equipment row and can be unequipped by tapping it. Drag-to-equip is still gated on extracting the exact original per-backpack capacity values.
- Equipment is included in save version 3; older player-only and player-plus-inventory saves remain readable.
- Player rendering draws original sprite objects by animation name, then layers pants, outerwear, vest, helmet, backpack, and held weapon.
- The generated source sprite catalog provides the exact Construct object for each mapped item.

## Still required before Stage 2 completion

Clothing condition/durability, backpack capacity, the remaining item actions, and their save/load behavior still need to be ported from the original Construct variables and events. The armor-aware damage API is ready, but it still needs to be connected to the Stage 3 NPC hit path when that combat path is implemented. The temperature offset still needs to be combined with the source Day_cold/weather value. Values must not be guessed.