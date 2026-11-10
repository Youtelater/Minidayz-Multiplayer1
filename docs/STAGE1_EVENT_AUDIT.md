# Mini DAYZ Construct event-sheet audit

Source: complete supplied `data.deobfuscated.js` Construct project export (14,558,329 bytes). The event counts below are generated from the repository's Stage 1 manifest. Legacy `romfs/` material is not used to override these source counts.

## Event-sheet coverage

| Sheet | Top-level entries | Nested event nodes | Variable declarations |
|---|---:|---:|---:|
| Game_events | 518 | 21,348 | 888 |
| Menu_Events | 153 | 923 | 100 |
| Login_events | 27 | 60 | 8 |
| Loading_events | 15 | 8 | 9 |
| Everywere | 4 | 4 | 1 |
| Tutorial_events | 13 | 0 | 11 |
| **Total** | **730** | **22,343** | **1,017** |

Construct event nodes contain nested conditions, actions and subevents; top-level entry counts alone understate the logic. Tutorial_events contains variable declarations and event-sheet include entries, hence zero locally defined event nodes in this traversal.

## Confirmed gameplay/system groups in Game_events

The named groups directly identify original systems to port: Map generation, Bunker_generate, Bridge level, Z_spawners, Player stats, States, Animations, Interaction, Firearm shooting, Alternate attacks, Grenade launcher, Melee fight, Throwing, Using inventory items, Picking player items, Picking inventory items, Droppin items, Gardening, Stash, Drivable vehicles, Vehicle collisions, Vehicles UI, Achievements, Tutorial_map, Inventory_guide, Firearm_guide, Perks_guide, Bleeding_guide, Death_guide, Sleeping, Quests, Airdrop, Next_level_menu, Perks and stats menu, and Building mode.

Game_events variables include final-day progression, map-generation seeds, zombie-wave state, player skin and GUI controls, tutorial state, audio settings, reload/save state and item discovery.

## Construct command coverage

Nested traversal found 115 distinct condition command IDs and 206 distinct action command IDs across the event sheets. IDs are preserved rather than guessed into names. Resolve them against Construct object/plugin metadata before reimplementation. Full condition/action arrays remain in the parsed export and generated event index.

## Translation boundary

This is a complete structural event-tree inventory, not a claim that 22,343 event nodes have already been rewritten into native C++. Behavior-level translation still requires resolving Construct command IDs, parameters, object references, expressions and subevent semantics. Subsystem groups provide traceable scope; behavior-equivalence tests remain required before Stage 1 sign-off.
