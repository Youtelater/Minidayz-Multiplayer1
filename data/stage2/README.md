# Stage 2: Items and inventory

Stage 2 is a source-backed port of the original Mini DAYZ 1.6.0 item, inventory, and player-equipment rules. The authoritative inputs are the repository-root original files: `data.deobfuscated.js`, `mp_itemtable.js`, the root localization XML files, and the checked-in parsed Construct project derived from the root export. The newer `Minidayz-Multiplayer-main/` directory is not used as the source of truth.

## Implemented inventory and backpack behavior

- The original English item localization and `MDZItemTable` generate a catalog preserving all 405 original item IDs and gaps.
- The original sprite catalog resolves 329 item IDs to exact Construct object identities; 76 IDs remain explicitly unmapped rather than receiving substitute art.
- InventorySystem provides 15 base slots, atomic additions/removals, source stack limits, moving, swapping, splitting, ground pickup/drop, per-stack item condition, and snapshots.
- The original `Check_stack_size` baseline groups are ported. The `perk_cybb` branch returns 31,415,926 in the source; the native `uint16_t` representation uses `UINT16_MAX` as its safe upper bound when that override is explicitly supplied.
- Backpack storage is derived from the original `Items_update` Construct function: the `Backpack` category renders source slot indices 1-11. Together with the 15 base slots this gives 26 total slots.
- Original wearable backpack IDs are represented by the dedicated backpack equipment state. The item list excludes the Civilian Tent (456) and Supplies Stash (461), which are not wearable backpacks.
- Equipping consumes a single backpack from inventory. Replacing a backpack returns the old one atomically; if the incoming pack is in a slot that disappears during a capacity change, the old pack is returned to a free slot in the surviving grid or the replacement fails without changing inventory.
- Unequipping is allowed only when all 11 backpack storage slots are empty. Capacity cannot be manually increased or reduced independently of the equipped source backpack.
- Backpack condition is preserved through equipment swaps and inventory snapshots. Restore validates the complete slot layout and normalizes legacy expanded-capacity saves without stranding items.
- Ground pickup/drop and inventory moves preserve item condition; stacks with different conditions do not merge.

## Other implemented source data

- Forty direct literal stat-effect rows are extracted from original `item_activate` and `item_activate2` events.
- Sixty-three original equipment property rows provide clothing warmth and flat armor protection; 168 item IDs have source-map condition defaults.
- Raw Construct inventory behavior records are exported with their source event UIDs and conditions/actions for further porting.

## Remaining Stage 2 work

- Port source item-condition wear, repair, and other durability rules without assuming that condition scales armor or warmth.
- Port conditional perk bonuses and remaining item-specific secondary effects without guessing expression semantics.
- Complete loot spawning, world pickup/drop interaction, and item actions in the actual game runtime.
- Finish the full inventory/equipment UI integration and save/load testing in the 3DS build.
- Connect armor protection and clothing warmth formulas to the live combat/weather runtime.

Original Construct data and event expressions remain authoritative. Unresolved values must stay explicitly unresolved rather than being inferred from item names.
