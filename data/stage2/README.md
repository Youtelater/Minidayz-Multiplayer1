# Stage 2: Items and inventory

Stage 2 ports the original Mini DAYZ 1.6.0 item, inventory, backpack, equipment, and player-item-use behavior. The authoritative inputs are the repository-root original files: `data.deobfuscated.js`, `mp_itemtable.js`, the root localization XML files, and the checked-in parsed Construct project derived from the root export. The newer `Minidayz-Multiplayer-main/` directory is not used as the source of truth.

## Implemented and regression-tested

- The original English item localization and `MDZItemTable` generate a catalog preserving all 405 original item IDs and gaps.
- The original sprite catalog resolves 329 item IDs to exact Construct object identities; 76 IDs remain explicitly unmapped rather than receiving substitute art.
- InventorySystem provides 15 base slots, atomic additions/removals, source stack limits, moving, swapping, splitting, ground pickup/drop, per-stack item condition, and snapshots.
- The original `Check_stack_size` baseline groups are ported. The `perk_cybb` branch returns 31,415,926 in the source; the native `uint16_t` representation uses `UINT16_MAX` as its safe upper bound when that override is explicitly supplied.
- Backpack UI grid limits are derived from original `Items_update` (11 possible storage positions). The root `l_eng_ui.xml` also explicitly defines **3** added storage slots for the Improvised Sling Bag (18 total) and **5** for the Improvised Backpack (20 total); the generator now extracts these original capacities and applies them. The other wearable packs still use the 11-slot grid ceiling pending their exact original per-pack equip rules.
- Original wearable backpack IDs are represented by the dedicated backpack equipment state. The item list excludes the Civilian Tent (456) and Supplies Stash (461), which are not wearable backpacks.
- Equipping consumes a single backpack from inventory. Replacing a backpack returns the old one atomically; if the incoming pack is in a slot that disappears during a capacity change, the old pack is returned to a free slot in the surviving grid or the replacement fails without changing inventory.
- Unequipping is allowed only when all 11 backpack storage slots are empty. Capacity cannot be manually increased or reduced independently of the equipped source backpack.
- Backpack condition is preserved through equipment swaps and inventory snapshots. Restore validates the complete slot layout and normalizes legacy expanded-capacity saves without stranding items.
- Ground pickup/drop and inventory moves preserve item condition; stacks with different conditions do not merge.
- Forty direct literal stat-effect rows are extracted from original `item_activate` and `item_activate2` events and applied by the item-use runtime. The original `perk_hamster` (+10 hunger) and `perk_camel` (+10 thirst) conditional branches are represented by explicit `ItemUsePerks` flags; the missing perk-progression/runtime state must supply those flags before those bonuses can activate in gameplay.
- Sixty-three original equipment property rows provide clothing warmth and flat armor protection; 168 item IDs have source-map condition defaults.
- Original repair menu actions are extracted and implemented: duct tape (+15) and epoxy glue (+25) for pants, outerwear, vest and backpack; sewing kit repairs for helmet (+50), light helmet (+25), pants (+50), and outerwear (+50). Repairs consume one matching material, require condition between 1 and 99, and clamp at 100.
- Armor protection is summed from the original flat equipment values. The player damage API applies the original flat subtraction; item condition does not scale armor or warmth.
- Raw Construct inventory behavior records are exported with their source event UIDs, conditions and actions for further porting. The six original combat/obstacle groups containing item-condition writes are also retained and CI-validated; their fractional and perk-dependent action semantics remain unported (see `docs/STAGE2_CONDITION_WEAR_TRACE.md`).

## Source-backed crafting implementation

The bottom 3DS touchscreen now has a craft list opened from the backpack
footer. Three transactions are generated from the original root tutorial
recipe text and original item IDs: Ashwood Stick + Rope -> Improvised Bow,
Rope + Burlap Sack -> Improvised Sling Bag, and Improvised Sling Bag +
three Wooden Sticks -> Improvised Backpack. The generator rejects missing
or altered root tutorial text; failed crafting does not consume ingredients. The source events `Craft_1`, `Craft_4`, and `Craft_5` each call original `Spawn_drop`, so the native craft result now appears at player coordinates as an original ground item (not directly in inventory).
These are the only recipes currently translated without assuming undocumented
requirements or outputs. Additional original crafting actions still need
event-level extraction and porting.

## Remaining Stage 2 work

- Port the original item-condition wear/loss events and connect them to the correct gameplay actions. Do not infer wear from damage or item names.
- Connect the existing armor-aware player damage API to the original NPC/combat hit path. NPC spawn, movement and chase are implemented, but the complete source combat/attack path is not.
- Connect the existing clothing warmth expression to the original weather/temperature update, including the separate `Day_cold` contribution. Do not substitute an invented weather formula.
- Finish original loot spawning and remaining item-specific secondary actions from the exported Construct traces.
- Complete perk progression and connect its evaluated source flags to item use.
- Extract the per-item capacity of each remaining wearable backpack from original Construct equip conditions; the 11-slot UI grid does not prove that all packs have eleven usable storage slots.
- Finish the full source inventory/equipment UI and end-to-end save/load testing on the 3DS runtime.

The native regression and 3DS build workflows passed on commit `4e91ae672230afcda9e72e9ab1615574256ebbd8`. Those results are not a physical 3DS hardware test, and they must be rerun after subsequent source changes. Unresolved values remain explicitly unresolved rather than being inferred from item names.
