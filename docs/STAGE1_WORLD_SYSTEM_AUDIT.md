# Stage 1 — World system (Step 3) audit

**Source:** authoritative Construct export snapshot under `data/stage1/source_snapshot/`. **Branch:** `main`.

## Verified source structure

- Main `Map` layout: 36,850 × 20,000, with 1,250 placed instances.
- The main Map layout has 103 structure-related instances across `Buildings`, `Buildings_bg`, `Buildings_base`, `Buildings_base_top`, and `Buildings_shadows`.
- It contains 44 `zed_resp` placements, 33 `LOOT_respawn` placements, and one `Tourist_points` placement. These are source layer placements, not a claim that all runtime spawn rules have been ported.
- Map-generation group UID: `6835885414231505`; 3,062 nested event nodes.
- Direct child groups: `Bunker_generate` (443 nodes), `Bridge level` (63), `Z_spawners` (54), `MiniMap_legend` (7), and `Location loader` (1,660).

## Map-grid and road-generation findings

The event snapshot exposes named function bodies, not just the group names:

- The source initializes `Map_size_X` and `Map_size_Y` to 16 for the normal map path.
- The TD path sets both dimensions to 3.
- A `CurrentLevel == 4` path sets `Map_size_Y` to 33.
- `generate_array_locations` contains 201 nested event nodes. It sets different POI quotas for the source's map-level branches, including villages, military locations, cities, hospitals, fire stations, gas stations, shops, and roadside cafes.
- `generate_array_roads_1` has normal and special-level branches. It works against the same 2D array and avoids existing location codes before writing road code 13.
- `generate_array_roads_2` adds the other road orientations/codes (12 and 14) to the grid.
- `generate_array_tomap` contains 176 nested event nodes and translates the grid values into the map's visual representation.
- `generate_mapgen_toelements` iterates generated map markers and calls `generate_location`; the latter contains 201 nested event nodes and 62 direct child branches for location templates.

The Array object documentation confirms the source model is a zero-based, up-to-three-dimensional grid with explicit size, indexed reads/writes, and out-of-range reads returning zero. The original event expressions and branch conditions still need to be translated and tested in native code; the numeric command IDs are not being guessed.

## Bunker, bridge, and spawn findings

- `Bunker_generate` contains 136 calls to `Bunker_spawn`, with source arguments distinguishing floor, wall, and default bunker pieces.
- `Bridge level` contains 59 calls to `Load_bridge_element` and 25 calls to `Spawn_NPC`.
- `Z_spawners` contains 51 calls to `Spawn_NPC`.
- The named function bodies and call parameters are retained in the authoritative snapshot. Their runtime effects, placement rules, object templates, and spawn conditions still require C++ translation and regression checks.

## Native C++ work committed

- `include/minidayz/world.hpp` now models source-backed world features with 64-bit Construct UIDs, source layer/object IDs, and original geometry.
- `source/world.cpp` now loads the authoritative layout and indexes source placements by their source layer into terrain, road, building, and spawn-point features. Bunker and bridge layer names are also recognized if present.
- `WorldSystem::generate_from_source(seed, "Map")` provides a seeded entry point for loading the source layout. The seed is retained; this method does not yet claim to reproduce Construct's random sequence or procedural output.
- All original placement coordinates and transforms are preserved, including negative coordinates used by off-layout art.
- Original artwork under `assets/gfx/` remains untouched.

## Remaining implementation before Step 3 sign-off

1. Port the exact 2D array location quotas and road-generation branches, including Construct-compatible random behavior.
2. Port location-template expansion and map-element creation from `generate_location`.
3. Port bunker assembly and `Bunker_spawn` behavior.
4. Port bridge assembly, bridge state, and `Load_bridge_element` behavior.
5. Port player/NPC/zombie spawn selection, spawn conditions, and runtime integration.
6. Connect generated terrain/roads/structures/spawns to world rendering and collision, then compare against the original game in Manic Emu.

**Status:** source-backed layout indexing is implemented. Procedural generation and runtime behavior are still in progress; Step 3 is not signed off. A successful CI build verifies compilation only, not gameplay equivalence.
