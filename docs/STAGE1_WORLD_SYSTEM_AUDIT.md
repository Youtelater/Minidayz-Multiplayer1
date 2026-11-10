# Stage 1 — World system (Step 3) audit

**Source:** authoritative Construct export snapshot under `data/stage1/source_snapshot/`. **Branch:** `main`.

## Verified source structure

- Main `Map` layout: 36,850 × 20,000, with 1,250 placed instances.
- The main Map layout has 103 structure-related instances across `Buildings`, `Buildings_bg`, `Buildings_base`, `Buildings_base_top`, and `Buildings_shadows`.
- It contains 44 `zed_resp` placements, 33 `LOOT_respawn` placements, and one `Tourist_points` placement.
- Map-generation group UID: `6835885414231505`; 3,062 nested event nodes.
- Direct child groups: `Bunker_generate` (443 nodes), `Bridge level` (63), `Z_spawners` (54), `MiniMap_legend` (7), and `Location loader` (1,660).

## Map grid and location placement

- Normal map grid dimensions are 16 × 16. The TD path sets both dimensions to 3. For `CurrentLevel >= 4`, the source allocates a 16 × 33 array, then resets the active `Map_size_Y` loop count to 16. The second POI pass uses Y=18..32, leaving rows 16–17 as a gap.
- `generate_array_locations` has 201 nested event nodes. Its exact map-marker codes are villages=9, military=5, cities=6, hospitals=11, fire stations=15, and chaoshi=103.
- Source quotas by level are:
  - Level 0: 20 villages, 0 military, 3 cities, 1 hospital, 1 fire station.
  - Level 1: 15 villages, 2 military, 5 cities, 2 hospitals, 2 fire stations, 1 chaoshi.
  - Level 2: 6 villages, 4 military, 10 cities, 3 hospitals, 3 fire stations, 1 chaoshi.
  - Level 3: 3 villages, 5 military, 11 cities, 4 hospitals, 2 fire stations, 1 chaoshi.
  - Level 4 first half: 4 villages, 8 military, 6 cities, 4 hospitals, 4 fire stations, 1 chaoshi. Its second half adds 4 villages, 6 military, 3 cities, 2 hospitals, 2 fire stations, 2 chaoshi. Levels above 4 use a different first-half quota and the same second-half quota.
- The source draws check_X/check_Y in the ranges 1..15 and stores at (check_X-1, check_Y-1), after testing the surrounding 3 × 3 cells. For the second half, check_Y is 18..32, producing actual grid rows 17..31. Gas stations, lite shops, and roadside cafes are separate counters and are not among these six Array marker codes.
- `generate_array_roads_1` and `generate_array_roads_2` write road codes 13, 12, and 14 into this grid. The native pass follows the source loops: connect the first two qualifying POI markers on each row/column, skip occupied cells, and turn horizontal/vertical intersections into code 14. The level-4 path also runs the source's second vertical pass with a +17 Y offset.
- `generate_array_tomap` has 176 nested nodes and maps grid values to map-marker type codes. Empty cells choose from a level-dependent marker list; village and city cells choose from source variant lists; military, road, hospital, fire-station, and other codes map to their original marker IDs.
- `generate_mapgen_toelements` calls `generate_location`; that function has 201 nested nodes and 62 location-template branches.

## Bunkers, bridges, and spawns

- `Bunker_generate` contains 136 `Bunker_spawn` calls: 39 floor pieces, 38 wall pieces, and 59 default bunker pieces. The source function creates object 975 on the `items_on_ground` layer and stores its slot/weight values.
- `Bridge level` contains 59 `Load_bridge_element` calls and 25 `Spawn_NPC` calls. The bridge function maps source family/element pairs to the original object indices and places them on the `Buildings` layer.
- `Z_spawners` contains 51 `Spawn_NPC` calls.
- The Map layout itself contains 44 zombie respawn markers, 33 loot respawn markers, and one tourist point. Runtime NPC/zombie selection and spawn conditions are not yet ported.

## Native C++ work committed

- `WorldSystem` loads the original Map layout and indexes terrain, road, building, and spawn-point placements by their original layer names, preserving 64-bit UIDs and source transforms.
- A native `WorldGrid` implements zero-based 2D cell storage, out-of-range reads as zero, the source 3 × 3 empty-neighborhood placement check, and the source horizontal/vertical road-code passes. `WorldMapMarker` now translates each generated grid cell into the original marker-type code/variant list from `generate_array_tomap`.
- `source_world_profile` contains source level 0–4 and above-4 map dimensions and split POI quotas. `generate_procedural_grid` places first-half and second-half markers using the source Y ranges and 3 × 3 spacing rule, then applies the source road passes. It uses a seeded native PRNG; its sequence is not yet proven identical to Construct.
- The source data header contains all 136 bunker spawn records and all 59 bridge element records, including bridge object-index mappings. `generate_from_source(seed, "Map", level)` loads the layout, generates the POI grid, and indexes the source bunker/bridge features.
- Bunker features preserve the original function slot/weight values and map_fs X offset. Bridge features preserve family, element, enabled flag, source object index, and original coordinates.
- Original artwork under `assets/gfx/` remains untouched.

## Remaining implementation before Step 3 sign-off

1. Resolve the exact cell-to-world coordinate transform and turn map markers into the original world-space map elements.
2. Port all 62 location-template branches and map-element creation into actual building/terrain placements.
3. Render and collide with bunker and bridge pieces using the original object artwork; verify bridge state behavior.
4. Port `Spawn_NPC`, `Trigger_spawn`, zombie spawn selection, and spawn conditions.
5. Connect generated grid and features to the runtime camera, rendering, and collision; compare the result in Manic Emu.

**Status:** source-backed grid/POI placement, bunker descriptors, bridge descriptors, and static layout indexing are implemented in C++. Roads, full building-template expansion, dynamic spawn behavior, and emulator verification remain. A successful CI build verifies compilation only, not gameplay equivalence.
