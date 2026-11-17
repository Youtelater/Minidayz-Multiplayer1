# Stage 2: Items and inventory

Stage 2 ports the original Mini DAYZ 1.6.0 item, inventory, backpack, equipment, and player-item-use behavior. The authoritative inputs are the repository-root original files: `data.deobfuscated.js`, `mp_itemtable.js`, the root localization XML files, and the checked-in parsed Construct project derived from the root export. The newer `Minidayz-Multiplayer-main/` directory is not used as the source of truth.

## Implemented and regression-tested

- The original English item localization and `MDZItemTable` generate a catalog preserving all 405 original item IDs and gaps.
- The original sprite catalog resolves 329 item IDs to exact Construct object identities; 76 IDs remain explicitly unmapped rather than receiving substitute art.
- InventorySystem provides 15 base slots, atomic additions/removals, source stack limits, moving, swapping, splitting, ground pickup/drop, per-stack item condition, and snapshots. The bottom 3DS touchscreen now exposes a fifth **SPLIT** action that moves half the selected stack into a free slot while preserving condition and rejecting invalid splits atomically.
- The original `Check_stack_size` baseline groups are ported. The `perk_cybb` branch returns 31,415,926 in the source; the native `uint16_t` representation uses `UINT16_MAX` as its safe upper bound when that override is explicitly supplied.
- Backpack UI grid limits come from original `Items_update` (11 positions). Root `l_eng_new.xml` documents **individual backpack capacities** under a separate description-ID scheme; the generator matches each adjacent source comment to the original item name. Taloon +5, Hunting +7, Mountain +11, School +3, Tortilla +9, Satchel +2, Army/Medical Satchel +1 unrestricted slot. Root `l_eng_ui.xml` takes priority for the conflicting Improvised Sling Bag +3 and Improvised Backpack +5 tutorial counts. The three exclusive Army/Medical Satchel cells are now wearable alongside backpacks and enforce the exact original item-ID whitelists. Fuel Pack (462) is wearable in the backpack slot but provides **zero** inventory slots: its original description grants fuel capacity only.
- Original wearable backpack IDs are represented by the dedicated backpack equipment state. The item list excludes the Civilian Tent (456) and Supplies Stash (461), which are not wearable backpacks.
- Equipping consumes a single backpack from inventory. Replacing a backpack returns the old one atomically; if the incoming pack is in a slot that disappears during a capacity change, the old pack is returned to a free slot in the surviving grid or the replacement fails without changing inventory.
- Unequipping a backpack is allowed only when its removed storage cells are empty and the surviving inventory remains valid; the original 15 base slots are preserved. Capacity cannot be manually increased or reduced independently of the equipped source backpack.
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
- Current save version **10** persists player, inventory, equipment, dropped items, deployed tent positions and contents, in-flight original medical/reading actions, and native Camel/Hamster experience and levels. Old v1-v9 save files are validated and migrated without losing existing items or tent contents.
- **Scope limit:** 8 deployed tents and 32 stored stacks per tent are *native memory-safety limits*, not extracted original gameplay capacities. The exact original tent capacity, placement restrictions, and interaction range still require a verified Construct event translation.

## Original crafting source audit

- The root Construct export contains **19** distinct `Craft_*` functions. `tools/stage2/audit_crafting_source.py` now exports their exact source UIDs, nested condition/action expressions, and called functions to a CI artifact named `original-crafting-source`.
- **Sixteen** source-validated native crafting transactions now exist, generated from original root Construct `Craft_*` events and root XML descriptions: Improvised Bow, Sling Bag, Backpack, three Campfire Kit tinder variants, three Knife/Arrow variants, selected-stack Rags x3 to Rope, Laser AK Grip, Laser Rail Grip, Molotov Cocktail, Canvas, Makeshift Suppressor and Simple Fishing Rod. Original direct inventory `Check_space_inventory` vs `Spawn_drop` outputs are honored. Original `Craft_1`/`Craft_4`/`Craft_5` bags and bow and `Craft_21`/`Craft_22` arrows wait five seconds, and `Craft_3` campfire variants wait three seconds, with the delays verified from root Construct events. `Craft_tent` (Canvas) and `Craft_xiaoyinqi` (Makeshift Suppressor) also require the original five-second delay on the native touch UI; `Craft_fishing_rod` waits the source-defined four seconds, then creates source object t837 (item 87) at player Y + 10 with condition 100. Other source `Craft_*` functions (including world `Building_mode` actions) remain under translation; 16 transactions do not mean all 19 source functions or full crafting parity are finished.
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
not item storage; it equips with no extra inventory slots. This conflict and the exclusive item-ID restrictions are implemented from the original Construct source guards.

## Original perk progression and timed experience

- Original `Game_events` defaults `perk_cost=100`, `expierence_points=0`, `perks_learnt_count=0`. Original purchase event UID `454712397231976` raises the next cost by `100 + 20 * perks_learnt_count`. XP is **cumulative**, not spent on purchase: the source `Perks and stats menu` writes no deduction to `expierence_points` when perks are acquired.
- `SourcePerkSystem` implements source purchase levels 0 → 1 → 2 for Camel and Hamster, validates purchase thresholds and restored states, and supplies item-use bonuses only at level 2. The bottom touchscreen offers a separate Perks panel and persists this state across save/continue.
- Original `item_activate` event UID `422420671955084` consumes Newspapers (49), waits 5 seconds and awards 50 XP. Original UID `853523169384096` consumes Scout Handbook (116), waits 10 seconds and awards 200 XP. Both now have source-verified asynchronous in-game item actions, and their pending awards survive saves without loss.
- Other perks, additional source event XP awards, and complete world combat/quest XP triggers remain open. No guessed passive XP or random kill XP is awarded.

## Parts 2–8 native integration (source-backed, 16 November 2026)

- **Part 2, loot:** `GroundLootSystem::spawn_verified_source_choice`
  turns a source-selected `choose(...)` record and 32-bit draw into an
  atomic, validated ground item. Its original marker category, seed guards,
  and respawn event still need a translated caller; this is **not**
  unconditional or autonomous world loot respawning.
- **Part 3, durability:** `SourceFractionalWear` represents original
  0.2/0.25/0.4/0.5 decrements in exact hundredths, without rounding the
  condition to a byte on each event. Native tests cover fractional accumulation,
  broken items, snapshot validation, and reject invalid restores. The original
  guarded firearm/melee/obstacle event dispatch and persistent instance
  integration are still required; **do not** apply wear on every frame.
- **Part 4, perks:** Original cumulative XP thresholds, Camel/Hamster
  level 1–2 unlocking, timed Newspapers and Scout Handbook awards, and
  persistent touch-menu progression are implemented. Other perks remain.
- **Part 5, survival:** At new-game initialization and whenever equipped
  clothing warmth changes, the 3DS scene now applies the verified daytime
  expression `Player_temperature = Day_cold - clothing_warmth` without
  clipping negative values to preview HUD limits. Loaded saves retain their
  temperature. Weather scheduling and rain/night/cold-damage source events
  remain separate work.
- **Part 6, contact/armor:** `apply_verified_npc_contact_damage`
  supplies a guarded NPC hit transaction using the equipped original flat
  armor values, with no fabricated attack range, damage amount or cooldown.
  Tests cover proximity, dead actors, invalid source values, and protection.
  An exact original hit-event dispatcher must call this transaction in gameplay.
- **Part 7, backpacks:** Fourteen source backpack item types, source-derived
  capacities, the Fuel Pack exception, original condition preservation and
  lossless legacy-save migration are already live. Restricted/exclusive
  Army and Medical Satchel slots now enforce source-verified item eligibility guards.
- **Part 8, tents/storage:** The native 3DS deploy actions now reject
  obstructed corners and overlap with previously deployed tents before
  consuming the item. Storage transactions remain atomic and persisted;
  distances are **native safety checks**, not a claim about exact original
  Construct placement behavior. Supplies Stash source actions are still
  under audit.

Native tests and 3DS builds must pass for the relevant commit before
claiming a verified binary. These improvements do **not** yet make
Stage 2 100% source-equivalent.

## Original 16-perk progression update (16 November 2026)

- Native `SourcePerkSystem` now holds **all 16 original** two-level
  Perks and stats menu variables, including the 14 previously missing
  categories. Every purchase uses the exact cumulative-XP thresholds and
  source cost increment `100 + 20 * perks_learnt_count`.
- The original purchase icon index-to-variable associations are extracted
  from the nested Construct action events and checked in CI. In particular,
  slots 8–11 are **melee defense, melee attack, trunkster, hunter**.
- The bottom 3DS touchscreen offers six pages of original perk progression;
  no guessed perk bonuses are applied. Camel/Hamster level-two item bonuses
  remain wired; the remaining fourteen perks' individual gameplay effects
  still need guarded source event translation.
- Save version **9** persists all 16 levels and keeps the existing inventory,
  tent, medical timer, and original XP fields. Version-8 files are explicitly
  migrated, preserving Camel/Hamster levels and initializing only the new
  perk fields. Native tests cover the migration and corrupted states.
- Eighteen additional source-identifiable *literal XP ADD actions* are exposed
  as a guarded event-UID transaction. They do **not** fire autonomously:
  the original NPC, wave, quest, and other source event prerequisites must be
  implemented before calling them. The existing delayed book/newspaper awards
  must not be double-dispatched.

## Source-verified weather and regeneration (17 November 2026)

- `weather_source.hpp` now expresses the original Update_Stats event
  assignments, not merely the difficulty constants: daytime subtracts
  actual clothing warmth (UID 4824763499434787), nighttime directly
  assigns Night_cold (UID 1386894733047937), and rain's source branches
  assign either Rain_cold or Rain_cold / 2 (UIDs 110661674372728,
  551577037849239, 927448526676784 and 629017975270922).
- The native daytime world update calls this event-exact helper. The
  original Day_mode/Rain scheduler, the source conditions selecting the
  rain branches, and cold-damage trigger conditions remain unported.
- `regeneration_source.hpp` implements the literal 5-second
  source regeneration amounts Novice=2, Regular=1.75, Veteran=1.5,
  and the source Regen perk's below-50-health additional heal. It
  accepts every required original instance variable and status guard
  explicitly to avoid silently inventing how the source actor's
  var50, status52 or other variables map into the native preview state.
  **No live regeneration ticks fire until those guards are mapped.**
- Source audit, native behavior tests, and workflow checks validate
  the known values and fail on changed original regeneration data.

## Source satchel equipment integration (17 November 2026)

- The original `Satchel` (458/752), `Army Satchel` (459/753),
  and `Medical Satchel` (460/754) can now occupy a **separate
  equip slot alongside a normal backpack** on the 3DS bottom touchscreen.
  Drag a satchel to the seventh equipment cell or tap it to unequip.
  The untouched gameplay view remains on the top screen.
- Source-verified unrestricted capacities stack with the main backpack:
  +2 for Satchel, +1 for Army/Medical, **plus three exclusive slots
  in each Army/Medical Satchel**. The source-provenance event guards
  315583148629362/699699206748072/962634450988744 (Army, var9=10)
  and 926789927221036/353303031815206/250048865204980
  (Medical, var9=11) supply their exact permitted item ID sets.
  Every native add, pickup, drag/drop move and restore rejects forbidden
  cargo in those slots. They are visibly marked ARMY / MED on the
  lower 3DS touchscreen.
- The new equip/swap/unequip transactions preflight source-specific
  capacity **and the outgoing/incoming restricted-slot item filters**,
  preserving all items on rejected swaps. Native save v10 persists
  satchel ID and condition without changing the original 15+ backpack
  snapshot binary layout, and loads v1-v9 saves including the former
  single-slot satchel representation.
- `Supplies Stash` (461) has exactly six cells in the original item
  description and is **not** a wearable backpack. Native `StashStorageSystem`
  now provides six-slot atomic stack transfers, condition/fractional-wear
  preservation, checked deployment with non-overlapping tent/stash safeguards,
  top-screen world sprites, and lower-screen touch storage access. Save v13
  persists stashes and v12-and-earlier loads with empty stash state. Native
  collision bounds are explicit safety checks; precise original Construct
  placement conditions still require source event translation.

## Remaining Stage 2 work

- Port the original item-condition wear/loss events and connect them to the correct gameplay actions. Do not infer wear from damage or item names.
- Connect the existing armor-aware player damage API to the original NPC/combat hit path. NPC spawn, movement and chase are implemented, but the complete source combat/attack path is not.
- Connect the existing clothing warmth expression to the original weather/temperature update, including the separate `Day_cold` contribution. Do not substitute an invented weather formula.
- Finish original loot spawning and remaining item-specific secondary actions from the exported Construct traces.
- Add the remaining original perk effects and connect their exact XP award triggers; Camel/Hamster leveling and item bonuses are now in runtime.
- Validate any remaining per-item backpack and satchel behavior against the original Construct equip conditions. Simultaneous satchel/backpack equipment and the three exclusive slot ID filters are now implemented.
- Extract original tent capacity, placement restrictions, and interaction radius; current native storage bounds and interaction distance are technical defaults, not claims of source fidelity.
- Finish the full source inventory/equipment UI and end-to-end save/load testing on the 3DS runtime.

The native regression and 3DS build workflows passed on commit `4e91ae672230afcda9e72e9ab1615574256ebbd8`. Those results are not a physical 3DS hardware test, and they must be rerun after subsequent source changes. Unresolved values remain explicitly unresolved rather than being inferred from item names.

## Touchscreen inventory safety fixes (17 November 2026)

- Stack splitting skips source-restricted satchel cells that reject the incoming item, rather than stopping at the first incompatible empty cell.
- Unequipping equipment searches only item-eligible inventory cells; matching stack merges also require identical condition, and are preferred over consuming an additional empty cell.
- Unequipping an equipped backpack or satchel targets a genuinely empty surviving **base** inventory cell, because those operations cannot merge a worn bag with a partial stack.
- A second worn Army/Medical Satchel now retains its unrestricted and exclusive contents while the main backpack is unequipped or changed to a different capacity. These slots are remapped atomically, preserving original item IDs, conditions, and category eligibility; occupied main-backpack-only cells still block downsizing.
- Inventory regression tests exercise the remapping, failed downsizing, saved state restoration, and restricted Army Satchel eligibility after main pack swaps.
- These are native interaction safety fixes; the original-source gameplay gaps above and a fresh full 3DS CI/runtime validation are still required before claiming 100% Stage 2 parity.

## Original secondary craft on lower 3DS touchscreen (17 November 2026)

- The original secondary Rags (26) item_activate2 action generates Rope (41) only from a selected stack with **at least three Rags**, not any sum of separated piles. This was already implemented for direct Use2; it is now available in the lower-screen craft list as a tenth transaction.
- The source validator runs before generating the main recipe table. The generator imports its checked condition and item IDs, preventing a guessed or inconsistent extra recipe.
- The craft list shows the largest eligible single-stack count instead of the total Rags quantity, and the transaction never consumes an unqualified stack. The existing nine source-backed tutorial recipes remain unchanged.
- The remaining original Craft_* functions with world-building, placement, pickpile and special cooldown events need their distinct source-event mechanics, not invented two-item recipes.

## Crafting and fractional wear integration (17 November 2026)

- `tools/stage2/audit_crafting_candidates.py` exports the original nested event conditions for outstanding crafting functions. Its audit is CI-validated, not an invented crafting ruleset.
- The original item condition trace has 142 source write actions across six original gameplay groups. A strict source-wear generator exports **93 individually identifiable literal fractional wear actions**, excludes 49 ambiguous/dynamic writes, and preserves original group/event/action UIDs.
- `source_wear.hpp` offers a guarded source-UID dispatcher with source-correct `perk_wxnj` suppression. `InventorySystem::apply_verified_wear_action` and `EquipmentSystem::apply_verified_wear_action` now apply exact per-item fractional wear, including transfers between equipped, ground, tent, and backpack inventory. Native v12 saves retain equipped fractions separately and item-slot fractions within the former ABI padding; v10 and earlier zero previously undefined padding, and v11 item fractions are preserved. **Live firearm, melee, NPC and obstacle event guards still need to be wired up during combat integration; dynamically computed/ambiguous source actions are intentionally not guessed.**
- Both 3DS and native builds must pass after source edits; emulator/hardware behavior has not yet been validated for delayed crafting or newly integrated recipes.

## Original crafting source classification (17 November 2026)

- `tools/stage2/build_source_craft_events.py` indexes all **19 original Construct `Craft_*` functions**, retaining the exact source function UID, inventory-vs-ground-vs-world-object-vs-building-mode output, result ID/object ID, and source wait duration. The catalog is regenerated and tested by 3DS/host CI.
- Sixteen ordinary source-verified native transactions are in the lower-screen crafting menu. A source-created Fishing Rod uses Construct object t837 with source Y+10 placement and a 4-second delay. `Craft_31`-`Craft_34` are distinct source pickpile sequences producing Campfire Kit (23) after three seconds; the two bed variants enter source `Building_mode(12)`. They require original world resource/placement event interpretation and cannot be represented as unguarded inventory recipes. Do not claim full original crafting parity until these world events are connected.
- Fractional durability is now exact for inventory and equipped items and is saved, including 0.01% increments. New tests cover dropping, picking up, tent storage, equipping, unequipping, repair, and migration from v10/v11 binary saves. The complete original action guard evaluation and other dynamic wear expressions remain unresolved.

## Native source event additions (17 November 2026)

- Supplies Stash ID 461 uses the original six storage cells; native transfer, save, reload, collision and touch activation integration is covered by host regression tests. Save format v13 persists stash locations/cargo and continues v1-v12 saves with no invented stashes.
- Item details now display original fractional durability to two decimal places, and worn equipment durability bars preserve subpercent wear. Touchscreen unequipping matches exact fraction as well as whole-percent condition before merging.
- `tools/stage2/audit_stage2_audio_source.py` identifies exact Construct events for original WAV clips. Native NDSP audio loads `crafting.wav`, `drop1.wav`, `drop2.wav`, and `zippero.wav` from `romfs:/wav`; the successful crafting, item-to-ground drop, and backpack equip/unequip paths now trigger their respective original clips. No clips are synthesized; unavailable/unsupported files fail safely. Console/emulator audio playback still needs a live runtime check.
- The original five-second `Player stats -> States` body warmth event UID 2665582317889917 is now dispatched during active gameplay. Source action UID 471091765829229 adds `Player_temperature/2` to actor variable 16 and the negative-temperature branch resets to zero and applies 0.25 cold damage. Native weather **exposure** is stored separately from the body's persistent temperature. Save format v14 migrates old v1-v13 preview exposure to neutral body warmth instead of reinterpreting it as saved body cold, and v14 saves retain the new body state. Weather phase switching (rain/night) and all other source condition ancestry still require integration; this does not mean full survival parity.
- The rest of Stage 2's true original world-building Craft_* branches, guarded combat and object wear, full source loot respawn timing, remaining perk gameplay effects and end-to-end physical hardware testing are still incomplete. CI passing validates these added native mechanics, not entire original-game parity.
