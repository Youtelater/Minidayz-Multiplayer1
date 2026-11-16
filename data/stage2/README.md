# Stage 2: Items and inventory

Stage 2 ports the original Mini DAYZ 1.6.0 item, inventory, backpack, equipment, and player-item-use behavior. The authoritative inputs are the repository-root original files: `data.deobfuscated.js`, `mp_itemtable.js`, the root localization XML files, and the checked-in parsed Construct project derived from the root export. The newer `Minidayz-Multiplayer-main/` directory is not used as the source of truth.

## Implemented and regression-tested

- The original English item localization and `MDZItemTable` generate a catalog preserving all 405 original item IDs and gaps.
- The original sprite catalog resolves 329 item IDs to exact Construct object identities; 76 IDs remain explicitly unmapped rather than receiving substitute art.
- InventorySystem provides 15 base slots, atomic additions/removals, source stack limits, moving, swapping, splitting, ground pickup/drop, per-stack item condition, and snapshots. The bottom 3DS touchscreen now exposes a fifth **SPLIT** action that moves half the selected stack into a free slot while preserving condition and rejecting invalid splits atomically.
- The original `Check_stack_size` baseline groups are ported. The `perk_cybb` branch returns 31,415,926 in the source; the native `uint16_t` representation uses `UINT16_MAX` as its safe upper bound when that override is explicitly supplied.
- Backpack UI grid limits come from original `Items_update` (11 positions). Root `l_eng_new.xml` documents **individual backpack capacities** under a separate description-ID scheme; the generator matches each adjacent source comment to the original item name. Taloon +5, Hunting +7, Mountain +11, School +3, Tortilla +9, Satchel +2, Army/Medical Satchel +1 unrestricted slot. Root `l_eng_ui.xml` takes priority for the conflicting Improvised Sling Bag +3 and Improvised Backpack +5 tutorial counts. Exclusive satchel slots (+3) are tracked but cannot yet be equipped simultaneously with backpacks. Fuel Pack (462) is wearable in the backpack slot but provides **zero** inventory slots: its original description grants fuel capacity only.
- Original wearable backpack IDs are represented by the dedicated backpack equipment state. The item list excludes the Civilian Tent (456) and Supplies Stash (461), which are not wearable backpacks.
- Equipping consumes a single backpack from inventory. Replacing a backpack returns the old one atomically; if the incoming pack is in a slot that disappears during a capacity change, the old pack is returned to a free slot in the surviving grid or the replacement fails without changing inventory.
- Unequipping is allowed only when all 11 backpack storage slots are empty. Capacity cannot be manually increased or reduced independently of the equipped source backpack.
- Backpack condition is preserved through equipment swaps and inventory snapshots. Restore now **losslessly migrates** old +11-slot saves to the correct source-defined capacity: overflow stacks are merged into matching surviving stacks or moved into free surviving slots, preserving item condition. If nothing fits, restore fails atomically instead of deleting items.
- Ground pickup/drop and inventory moves preserve item condition; stacks with different conditions do not merge.
- Forty direct literal stat-effect rows are extracted from original `item_activate` and `item_activate2` events and applied by the item-use runtime. Original `perk_hamster` (+10 hunger) and `perk_camel` (+10 thirst) conditional bonuses are now connected to a persistent two-level perk system, with bonuses activating only at source level 2.
- Sixty-three original equipment property rows provide clothing warmth and flat armor protection; 168 item IDs have source-map condition defaults.
- Original repair menu actions are extracted and implemented: duct tape (+15) and epoxy glue (+25) for pants, outerwear, vest and backpack; sewing kit repairs for helmet (+50), light helmet (+25), pants (+50), and outerwear (+50). Repairs consume one matching material, require condition between 1 and 99, and clamp at 100.
- Armor protection is summed from the original flat equipment values. The player damage API applies the original flat subtraction; item condition does not scale armor or warmth.
- Raw Construct inventory behavior records are exported with their source event UIDs, conditions and actions for further porting. The six original combat/obstacle groups containing item-condition writes are also retained and CI-validated; their fractional and perk-dependent action semantics remain unported (see `docs/STAGE2_CONDITION_WEAR_TRACE.md`).

## Original weather difficulty constants

The original root `Game_events` sets **Novice Day/Night/Rain = 6/1/1**,
**Regular = 7/2/2**, **Veteran = 8/3/3**, and **Legend = 9/3/3**.
`tools/stage2/build_weather_cold_catalog.py` now verifies these exact
difficulty-conditional source event UIDs and literal global writes. The
bottom 3DS touchscreen displays the original `Day_cold - clothing warmth`
expression using the currently equipped helmet, outerwear and pants.
Rain/night scheduling and cold-damage thresholds remain unimplemented;
the source-verified display is not a substitute for those event branches.

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
The original `Craft_21` and `Craft_22` each create Crafted Arrow item 51 in inventory; the three root original knives (Hunting, Army and Butcher) remain reusable crafting tools. Three distinct arrow transactions use the same original `Wooden sticks + any knife = arrows` guide. Other crafting functions, especially building mode and placeable structures, still need exact translation.

## Source-verified tent storage (native integration)

- The original root `l_eng_items.xml` identifies item **456** as **Civilian Tent**. The original root `l_eng_ui.xml` states that civilian tents can be deployed and that items stored there persist permanently. `tools/stage2/validate_tent_storage_source.py` verifies both source facts.
- `TentStorageSystem` deploys one actual tent item, preserves its condition, transfers item stacks atomically while preserving their condition, and rejects corrupt save snapshots.
- The bottom 3DS touchscreen cycles **Backpack → Crafting → Tent Storage → Perks → Backpack**. In storage mode, tap the left pane to store an inventory stack and the right pane to retrieve it; tap the header of either pane to page. The top screen continues to render the world and deployed tents.
- Current save version **8** persists player, inventory, equipment, dropped items, deployed tent positions and contents, in-flight original medical/reading actions, and native Camel/Hamster experience and levels. Old v1-v7 save files are validated and migrated without losing existing items or tent contents.
- **Scope limit:** 8 deployed tents and 32 stored stacks per tent are *native memory-safety limits*, not extracted original gameplay capacities. The exact original tent capacity, placement restrictions, and interaction range still require a verified Construct event translation.

## Original crafting source audit

- The root Construct export contains **19** distinct `Craft_*` functions. `tools/stage2/audit_crafting_source.py` now exports their exact source UIDs, nested condition/action expressions, and called functions to a CI artifact named `original-crafting-source`.
- **Nine** source-validated crafting transactions are implemented: `Craft_1` (Improvised Sling Bag), `Craft_4` (Improvised Backpack), `Craft_5` (Improvised Bow), and three tinder alternatives for `Craft_3` (Campfire Kit). `Craft_3` calls `Check_space_inventory(-1, 23)`, unlike the three ground-spawn recipes. Another three transactions come from `Craft_21`/`Craft_22`, turning a wooden stick into one crafted arrow using one reusable original knife. Other original crafting functions have different item/stack and world-placement semantics and are not yet translated.
- The bottom touchscreen crafting panel now pages three recipes at a time; adding verified source recipes does not require shrinking the screen layout.

## Original backpack description mismatch

The original root `l_eng_new.xml` has separate IDs for backpack descriptions
(e.g., Taloon Backpack is description ID 301, inventory ID 450). It also
disagrees with the root tutorial about the two improvised packs (+4/+6 in
descriptions versus +3/+5 in the tutorial). The port keeps the existing
tutorial-verified +3/+5 and uses the named original descriptions for the
other packs. Army and Medical Satchel descriptions specify three additional
**exclusive** slots that require item-category restrictions; those are not
silently added as general-purpose slots. Fuel Pack describes a fuel reservoir,
not item storage; it equips with no extra inventory slots. This conflict and those equipment restrictions remain
explicit until the corresponding original Construct equip events are ported.

## Original perk progression and timed experience

- Original `Game_events` defaults `perk_cost=100`, `expierence_points=0`, `perks_learnt_count=0`. Original purchase event UID `454712397231976` raises the next cost by `100 + 20 * perks_learnt_count`. XP is **cumulative**, not spent on purchase: the source `Perks and stats menu` writes no deduction to `expierence_points` when perks are acquired.
- `SourcePerkSystem` implements source purchase levels 0 → 1 → 2 for Camel and Hamster, validates purchase thresholds and restored states, and supplies item-use bonuses only at level 2. The bottom touchscreen offers a separate Perks panel and persists this state across save/continue.
- Original `item_activate` event UID `422420671955084` consumes Newspapers (49), waits 5 seconds and awards 50 XP. Original UID `853523169384096` consumes Scout Handbook (116), waits 10 seconds and awards 200 XP. Both now have source-verified asynchronous in-game item actions, and their pending awards survive saves without loss.
- Other perks, additional source event XP awards, and complete world combat/quest XP triggers remain open. No guessed passive XP or random kill XP is awarded.

## Remaining Stage 2 work

- Port the original item-condition wear/loss events and connect them to the correct gameplay actions. Do not infer wear from damage or item names.
- Connect the existing armor-aware player damage API to the original NPC/combat hit path. NPC spawn, movement and chase are implemented, but the complete source combat/attack path is not.
- Connect the existing clothing warmth expression to the original weather/temperature update, including the separate `Day_cold` contribution. Do not substitute an invented weather formula.
- Finish original loot spawning and remaining item-specific secondary actions from the exported Construct traces.
- Add the remaining original perk effects and connect their exact XP award triggers; Camel/Hamster leveling and item bonuses are now in runtime.
- Validate per-item backpack and satchel descriptions against the original Construct equip conditions. Source-documented capacity values are now applied; simultaneous satchel/backpack equipment and exclusive item filters remain incomplete.
- Extract original tent capacity, placement restrictions, and interaction radius; current native storage bounds and interaction distance are technical defaults, not claims of source fidelity.
- Finish the full source inventory/equipment UI and end-to-end save/load testing on the 3DS runtime.

The native regression and 3DS build workflows passed on commit `4e91ae672230afcda9e72e9ab1615574256ebbd8`. Those results are not a physical 3DS hardware test, and they must be rerun after subsequent source changes. Unresolved values remain explicitly unresolved rather than being inferred from item names.
