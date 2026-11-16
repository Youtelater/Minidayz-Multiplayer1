# Stage 2: Items and inventory

Stage 2 ports the original Mini DAYZ 1.6.0 item, inventory, backpack, equipment, and player-item-use behavior. The authoritative inputs are the repository-root original files: `data.deobfuscated.js`, `mp_itemtable.js`, the root localization XML files, and the checked-in parsed Construct project derived from the root export. The newer `Minidayz-Multiplayer-main/` directory is not used as the source of truth.

## Implemented and regression-tested

- The original English item localization and `MDZItemTable` generate a catalog preserving all 405 original item IDs and gaps.
- The original sprite catalog resolves 329 item IDs to exact Construct object identities; 76 IDs remain explicitly unmapped rather than receiving substitute art.
- InventorySystem provides 15 base slots, atomic additions/removals, source stack limits, moving, swapping, splitting, ground pickup/drop, per-stack item condition, and snapshots.
- The original `Check_stack_size` baseline groups are ported. The `perk_cybb` branch returns 31,415,926 in the source; the native `uint16_t` representation uses `UINT16_MAX` as its safe upper bound when that override is explicitly supplied.
- Backpack UI grid limits come from original `Items_update` (11 positions). Root `l_eng_new.xml` documents **individual backpack capacities** under a separate description-ID scheme; the generator matches each adjacent source comment to the original item name. Taloon +5, Hunting +7, Mountain +11, School +3, Tortilla +9, Satchel +2, Army/Medical Satchel +1 unrestricted slot. Root `l_eng_ui.xml` takes priority for the conflicting Improvised Sling Bag +3 and Improvised Backpack +5 tutorial counts. Exclusive satchel slots (+3) are tracked but cannot yet be equipped simultaneously with backpacks. Fuel Pack (462) carries fuel, not inventory slots, and is no longer treated as a storage backpack.
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

## Original temperature-expression correction

The original Construct player-stat event UID `4824763499434787` sets `Player_temperature = Day_cold - (outerwear warmth + helmet warmth + pants warmth)`. Native equipment calculations now use that exact additive source formula instead of the former incorrect division. The game still needs full timed weather, rain, night, and perk integration.

## Source-backed crafting implementation

The bottom 3DS touchscreen now has a craft list opened from the backpack
footer. Three transactions are generated from the original root tutorial
recipe text and original item IDs: Ashwood Stick + Rope -> Improvised Bow,
Rope + Burlap Sack -> Improvised Sling Bag, and Improvised Sling Bag +
three Wooden Sticks -> Improvised Backpack. The generator rejects missing
or altered root tutorial text; failed crafting does not consume ingredients. The source events `Craft_1`, `Craft_4`, and `Craft_5` each call original `Spawn_drop`, so the native craft result now appears at player coordinates as an original ground item (not directly in inventory).
The original root tutorial also describes **Woodpiles + Rags**, **Woodpiles +
Wooden Sticks**, and **Woodpiles + Newspapers** as three tinder alternatives
for a Campfire Kit. The source `Craft_3` calls
`Check_space_inventory(-1, 23)` rather than `Spawn_drop`, so all three
transactions now put the resulting item into inventory, with atomic rollback.
Other original crafting functions still require event-level translation,
including the variable-yield arrow crafting and placeable structures.

## Source-verified tent storage (native integration)

- The original root `l_eng_items.xml` identifies item **456** as **Civilian Tent**. The original root `l_eng_ui.xml` states that civilian tents can be deployed and that items stored there persist permanently. `tools/stage2/validate_tent_storage_source.py` verifies both source facts.
- `TentStorageSystem` deploys one actual tent item, preserves its condition, transfers item stacks atomically while preserving their condition, and rejects corrupt save snapshots.
- The bottom 3DS touchscreen cycles **Backpack → Crafting → Tent Storage → Backpack**. In storage mode, tap the left pane to store an inventory stack and the right pane to retrieve it; tap the header of either pane to page. The top screen continues to render the world and deployed tents.
- Save version **6** persists deployed tent locations and contents and migrates version 5 saves by reading the original complete C++ struct (including alignment padding).
- **Scope limit:** 8 deployed tents and 32 stored stacks per tent are *native memory-safety limits*, not extracted original gameplay capacities. The exact original tent capacity, placement restrictions, and interaction range still require a verified Construct event translation.

## Original crafting source audit

- The root Construct export contains **19** distinct `Craft_*` functions. `tools/stage2/audit_crafting_source.py` now exports their exact source UIDs, nested condition/action expressions, and called functions to a CI artifact named `original-crafting-source`.
- The currently implemented **three** source-validated recipes are `Craft_1` (Improvised Sling Bag), `Craft_4` (Improvised Backpack), and `Craft_5` (Improvised Bow). The other functions have different result semantics (for example `Check_space_inventory` rather than `Spawn_drop`) and must not be misrepresented as ground-spawn recipes.
- The bottom touchscreen crafting panel now pages three recipes at a time; adding verified source recipes does not require shrinking the screen layout.

## Remaining Stage 2 work

- Port the original item-condition wear/loss events and connect them to the correct gameplay actions. Do not infer wear from damage or item names.
- Connect the existing armor-aware player damage API to the original NPC/combat hit path. NPC spawn, movement and chase are implemented, but the complete source combat/attack path is not.
- Connect the existing clothing warmth expression to the original weather/temperature update, including the separate `Day_cold` contribution. Do not substitute an invented weather formula.
- Finish original loot spawning and remaining item-specific secondary actions from the exported Construct traces.
- Complete perk progression and connect its evaluated source flags to item use.
- Validate per-item backpack and satchel descriptions against the original Construct equip conditions. Source-documented capacity values are now applied; simultaneous satchel/backpack equipment and exclusive item filters remain incomplete.
- Extract original tent capacity, placement restrictions, and interaction radius; current native storage bounds and interaction distance are technical defaults, not claims of source fidelity.
- Finish the full source inventory/equipment UI and end-to-end save/load testing on the 3DS runtime.

The native regression and 3DS build workflows passed on commit `4e91ae672230afcda9e72e9ab1615574256ebbd8`. Those results are not a physical 3DS hardware test, and they must be rerun after subsequent source changes. Unresolved values remain explicitly unresolved rather than being inferred from item names.
