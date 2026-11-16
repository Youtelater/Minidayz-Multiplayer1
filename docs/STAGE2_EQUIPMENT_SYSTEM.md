# Stage 2 player equipment port

The native player equipment layer uses original 1.6.0 item IDs and `MDZItemTable` sprite-object mappings rather than substitute icons.

## Source-backed equipment slots

- Helmet: IDs 133-136, 250-277, 748, 786, 787.
- Outerwear: IDs 350-368, 702-703, 409, 413-415, 711, 713.
- Vest: IDs 400-405, 408, and CBRN suit IDs 410-412.
- Pants: IDs 300-307.
- Hands / held weapon: original firearm and melee IDs 103, 107-108, 151-216, 230-246, 785.
- Backpack is a dedicated inventory equipment slot because it controls the 11-slot inventory extension.

The original player damage event subtracts four flat protection values. The generator emits 63 source equipment-property rows from original Map/TD item instances. It reads variable 8 for clothing warmth and variables 12, 13, 14, and 15 for helmet, outerwear, vest, and pants protection respectively. These values are emitted per item ID and summed by EquipmentSystem::protection(). Clothing warmth is also extracted from item variable 8; EquipmentSystem::temperature_offset() ports the original Player_temperature clothing term `(outerwear - helmet) / pants`, excluding the separate Day_cold value. Separate footwear and glove equipment slots are not invented; items without an original equipment slot are rejected.

## Runtime behavior

- Dragging a single compatible item onto one of the five source equipment slots equips it.
- Replacing equipment returns the previous item to inventory atomically.
- Tapping an occupied clothing/held slot returns its item to an available inventory slot.
- The backpack equipment slot is separate from the 15 base inventory slots. The original `Items_update` function renders Backpack storage slots 1-11, giving 26 total inventory slots when a wearable backpack is equipped. The build generator verifies those exact source indices.
- Backpack unequip returns the pack only to one of the 15 base inventory slots. It is refused while any of the 11 backpack storage slots are occupied. The UI does not mistake a free backpack-storage slot for a valid unequip destination.
- Replacing a backpack from an extended slot preserves the old pack in the same surviving slot when possible; otherwise it uses a free slot in the resulting grid or fails atomically.
- Older snapshots with guessed capacity values are normalized safely on restore. Item condition (original instance variable 6) is preserved through inventory stacks, ground transfers, equipment swaps, backpack swaps and save/load.
- Equipment and per-item condition are included in save version 6. Player-only, inventory-only, version-3 equipment, version-4 equipment/inventory, and version-5 ground-loot saves are migrated.
- Player rendering draws original sprite objects by animation name, then layers pants, clothing, vest, helmet, backpack, and held weapon.
- The generated source sprite catalog provides the exact Construct object for each mapped item.

## Remaining before Stage 2 completion

Item condition storage and persistence are implemented using original instance variable 6; the source condition loss/repair rules still need to be ported. The armor-aware damage API is ready, but it still needs to be connected to the complete original combat path. The source difficulty Day_cold baseline (Novice 6, Regular 7, Veteran 8) is now applied as an absolute Player_temperature target together with the exact clothing expression. Map-specific Day_cold overrides and the original night/rain temperature events still need to be connected. World loot spawning and the remaining item actions still need their original Construct semantics. Values must not be guessed.
