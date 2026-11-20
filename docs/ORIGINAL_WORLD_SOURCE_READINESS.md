# MiniDAYZ 3DS — Root Original World Source Readiness

**Audit date:** 2026-11-20  
**Authoritative original:** `data.deobfuscated.js` (root, SHA-256 `b5e15cfad3106b56457bc13c4169e59b4ff0308035912330bf144b86238571d3`)  
**Branch:** `main`  
**Scope:** source analysis only; **no 3DS world mechanics or sprite behavior are declared complete**.

## 1. Critical correction to previous stage1 source assumptions

The cached `data/stage1/source_snapshot/part_*.jsonpart` is **NOT semantically identical** to the root original source. Source evidence from [the automated comparison](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44833659709):

| Difference | Count |
|---|---:|
| Event records that differ at the same source-tree path | 1,622 |
| Object definitions that differ | 311 |
| Layout definitions that differ | 6 of 8 |
| Audio resource entries that differ | 0 of 489 |
| Root source event nodes | 22,343 |
| Cached source event nodes | 22,343 |

**Rule:** Treat all earlier `data/stage1/` extracted inventories and generated C++ tables as potentially stale until regenerated from the root original. Group names and event UIDs in the previous narrative audit are research leads; recheck each original condition, expression, action, sprite-frame, transform, and object definition in the root source. Do not use `Minidayz-Multiplayer-main/` as an authority to fix this discrepancy.

## 2. Complete source indexing and graphics accounting — executed

The original-source extractor `tools/stage1/audit_original_world_exhaustive.py` successfully ran on the root original in GitHub Actions.

| Evidence | Root-source count |
|---|---:|
| Object types | 1,429 |
| Layouts | 8 |
| Event sheets | 6 |
| Event nodes preserved with their complete original conditions, actions and ancestry | 22,343 |
| Direct condition entries | 32,919 |
| Direct action entries | 63,289 |
| Named event groups | 220 |
| Original function-call actions indexed | 7,877 |
| Source audio actions indexed | 461 |
| Candidate source frame/opacity/layer actions indexed | 3,553 |
| Original Construct sprite-frame references | 14,505 |
| Editor-placed layout instances | 5,256 |
| Original PNGs under `assets/gfx/` | 1,854 |
| Original PNGs referenced by Construct sprite-frame definitions | 1,735 |
| Original PNGs without direct sprite-frame references | 119 |
| Missing/unmatched referenced PNG paths | 0 |

The **119** PNGs without direct sprite-frame references are **not proven unused**. Audit alternate use through tilemaps, custom runtime loading, data tables, menus or non-sprite resources. Every image must be accounted for, but we must not invent gameplay uses for source-unused assets.

The source contains **repeated event UIDs** (1,532 UID values have duplicates), so **event UID alone is not a globally unique lookup key**. Use `sheet + source_event_path` (ordered nested array indices) in the trace, plus UID and group ancestry, to locate the exact original event.

### Exact reproducible source outputs

Run from the repository root:

```bash
python tools/stage1/audit_original_world_exhaustive.py --out audit-output/world
python tools/stage1/compare_original_to_cached.py
```

The CI job produces one downloadable artifact containing:

| Artifact file | Purpose |
|---|---|
| `all_original_event_traces.jsonl` | Every event's original conditions/actions, nested source path, parent UIDs and group |
| `group_declarations.csv` | All named event groups and ancestor chains |
| `function_calls.csv` | Exact original function call actions, including literal names and expression arrays |
| `function_resolution.csv` | Static function-name definitions and call references; dynamic expressions explicitly unproven |
| `audio_actions.csv` | Source sound commands, event paths and complete action arguments |
| `visual_state_actions.csv` | Source frame, opacity, layer and other candidate visual commands |
| `original_sprite_frames.csv` | Object, animation/frame indices, PNG path, original atlas rectangle, origin and collision data |
| `original_layout_instances.csv` | Every editor-placed object, layer and source transform |
| `original_png_accounting.csv` | Every original PNG and all matched Construct frame references |
| `unmatched_original_frame_paths.csv` | Referenced frame paths without matching PNGs |
| `source_vs_snapshot_diff.json` | Cached-source divergence including differing event paths and objects/layouts |
| `summary.json` | Exact source hash, source counts and verification status |

**Successful workflow and artifact:** [Source audit run 44833659709](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44833659709). The uploaded artifact is retained for 90 days; the scripts are committed permanently and regenerate it.

## 3. World implementation dependency order

Do **not** implement each row as a visual-only approximation. Always open the applicable root-source event records and their inherited conditions and dependent effects.

| System | Original event/function families to trace | Output that must be implemented together |
|---|---|---|
| Normal and alternate maps | `Map generation`, `generate_array_locations`, `generate_array_locations_TD`, `generate_array_roads_1/2`, `generate_array_tomap`, `generate_mapgen_toelements` | Source spawn/seed logic, terrain/biome transitions, roads/intersections, bridge and bunker entrance positions, level-specific distribution, location quotas, RNG order |
| Location templates and life cycle | `Location loader`, `generate_location`, `create_map_location`, `create_map_element`, `erase_map_location`, `erase_map_element`, `Reload_locations` | Every actual structure, object layer, placement/transform and source-built loot/NPC marker; loading/unloading correctness |
| Forests and vegetation | `Hitting_obstacles`, vegetation objects, object-specific source spawning | All tree/bush species, distribution, damage/harvest conditions, original sprite changes, collision state, exact drops, sound, respawn/persistence |
| Houses and other buildings | `Buildings and obstacles`, `House_transparenting`, `Obstacles_transparenting`, `Big_Obstacles_transparenting` | Exterior and interior paired art, original opacity/layer behavior, furniture/walls, entry/exit, walkability, interiors/loot, rendering order |
| Doors and bunker entrances | `Doors_auto`, `Bunker_generate` and source object door branches | Opening and closing conditions, exact per-instance frames, door type and sounds, shadow/opacity, collision, re-trigger, bunker state |
| World loot and item interactions | `Spawn parameters`, `LOOT Respawn`, `Trigger_spawn`, `Spawn_drop`, `Check_loot`, `Global_respawn`, `Pickpile`, pickup and merged-interact events | Actual spawned marker categories, location-specific drop/rarity expressions, ground pickups/drops, containers, crafting/resource yields, timing and persistence |
| Structures and traps | `Building mode`, `Barberwire&Barricades`, traps, gardening, stash, fireplace | Valid placement, resource use, hit/destruction, updates to pathing/collision/rendering, growth, saving |
| Living world | `Z_spawners`, `NPCs`, `NPC_manager`, `Zombies`, `Bots`, `Animals` | Valid world locations, spawn conditions and counts, AI, collisions, sounds, combat interaction, loot |
| World events and environment | `Time cyles and weather`, `Rain`, `Day&Night`, `Airdrop`, `HeliEvents`, `generate_helicrashes`, quests, bridge and level changes | All timed/world effects, lighting and visibility, environmental audio, quest objects and rewards, entity/spawn consequences |
| Vehicles | `Drivable vehicles`, `Vehicle collisions`, `Vehicle Layering` | Correct world spawns, collision, draw order, repairs/fuel/damage only as original events specify |
| State/save | All original save/load and load/unload dependencies, multiplayer extension when applicable | Stable world-instance identity, changed door/tree/container state, harvested objects, generated loot and locations, future MP synchronization |

## 4. Required per-event implementation contract

For **every** original world action, explicitly trace:

1. **Condition chain**: all ancestor events and direct condition records; player/object selection and distance checks; control/button state; modes/levels/seed; timers; RNG.
2. **Entity/world state**: original object and instance, initial and mutated variables, creation/destruction, layer and z-order, collision/picking, save consequences.
3. **Graphics**: original PNG, spritesheet frame rectangle, original animation and frame, size/hotspot, rotation/flip, opacity/transparency and order.
4. **Audio**: original sound action event, exact cue/variant, parameters, trigger, looping and stopping; do not invent effects.
5. **Loot/rewards**: all source event `Spawn_drop` and related calls, exact original item IDs, position expressions, weights/rarities, pickup and inventory links.
6. **Cross-system effects**: NPCs/animals, navigation and visibility, quest/tutorial, health/survival, weather, temporary events, multiplayer replication as applicable.
7. **Evidence**: test each triggered state and linked effect in native unit tests and Azahar, then real 3DS before declaring completion.

Example: opening a door is not complete merely because its sprite changes; original conditions, audio, collision/shadow, layer, persistence and other event consequences must be reproduced whenever confirmed in source. For a tree, damage and falling frames alone are insufficient without drops, audio and save state.

## 5. Correct audit completion claims and remaining verification gates

**Completed in this research-only pass:** complete machine-readable root-original Construct event syntax coverage, exhaustive source event ancestry and direct condition/action preservation, source call/audio/visual index, all source sprite-frame references and editor-instance listings, all 1,854 PNGs accounted for in a file-level audit, and explicit measurement of cached-source drift. The audit can now be used as a source-of-truth lookup during implementation.

**NOT completed or proved by syntax indexing:** semantic execution of all Construct actions/conditions and expression syntax in `c2runtime.js`, original probability/call-order fidelity, proof that all 119 unreferenced images are unused, 3DS implementation of world interactions, automated original-vs-port gameplay comparisons, or real-console verification. Those are implementation and fidelity tasks, not grounds to claim the original game has been ported yet.

**Next world development gate:** regenerate the native generated catalogs and world source data from the **root original**, not the stale snapshot; then begin world and location event parity in dependency order. Do not build world simulation against `data/stage1` tables until their source provenance is fixed.
