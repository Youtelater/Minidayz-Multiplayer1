# Stage 2: Items and inventory

Stage 2 is a source-backed port of the original Mini DAYZ 1.6.0 item, inventory, and player-equipment rules.

## Implemented

- The original English item localization and MDZItemTable generate a catalog preserving all 405 original item IDs and gaps.
- The original sprite catalog resolves 329 item IDs to exact Construct object identities; 76 IDs remain explicitly unmapped rather than receiving substitute art.
- InventorySystem provides 15 base slots, up to 32 native slots, atomic additions, source stack limits, removal, moving, swapping, splitting, ground pickup/drop, and backpack equipment state.
- The original Check_stack_size baseline groups are ported. The perk_cybb branch returns 31,415,926 in the source; the native uint16_t stack representation safely caps that at UINT16_MAX.
- Item condition (original instance variable 6) is preserved through inventory, ground transfers, equipment, and save snapshots; preservation is not evidence of an original condition-based protection multiplier.
- Forty direct literal stat-effect rows are extracted from original item_activate and item_activate2 events.
- Sixty-three original equipment property rows provide clothing warmth and flat armor protection; 168 item IDs have source-map condition defaults.
- Raw Construct inventory behavior records are exported with their source event UIDs and conditions/actions for further porting, including the original backpack-space, backpack-item, backpack drop, and equipment-slot event paths.

## Remaining before Stage 2 completion

- Translate the extracted original backpack-space, backpack-item, Drop_slot, and Drop_backpack event logic into the runtime. The native capacity lookup deliberately remains unresolved (zero); do not enable guessed per-pack capacity values.
- Port source item-condition wear, repair, and other durability rules.
- Port conditional perk bonuses and remaining item-specific secondary effects without guessing expression semantics.
- Complete loot spawning, world pickup/drop interaction, and item actions in the actual game runtime.
- Finish end-to-end inventory/equipment UI integration and save/load testing in the 3DS build.
- Keep armor protection and clothing warmth flat as in the source item properties; connect the original Player_get_hit and Player_temperature event formulas to the live combat/weather runtime. Do not multiply either value by item condition unless an original event proves that rule.

Original Construct data and event expressions remain authoritative. Unresolved values must stay explicitly unresolved rather than being inferred from item names.
