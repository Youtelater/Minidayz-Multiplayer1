# Stage 1 — World system (Step 3) audit

**Source:** authoritative Construct export snapshot under data/stage1/source_snapshot/. **Branch:** main.

## Source facts verified

- The Map layout is 36,850 × 20,000; TD has the same dimensions.
- Game_events group Map generation has UID 6835885414231505, 3,062 nested event nodes, 6,413 condition entries, and 14,371 action entries.
- Direct child groups: Bunker_generate (443 nodes), Bridge level (63), Z_spawners (54), MiniMap_legend (7), Location loader (1,660).
- The source includes calls named Bunker_spawn and Load_bridge_element, and variables including Map_size_X, Map_size_Y, SEED_TD, TilemapspotX, and TilemapspotY.
- Location loader contains many nested tile and placement operations. The source snapshot retains object indices, numeric command IDs, parameters, and subevents.

## C++ work in this slice

- Added WorldSystem with verified source map dimensions as defaults.
- Added finite/bounds validation, coordinate clamping, seed storage, and a feature registry for terrain, roads, buildings, bunkers, bridges, and spawn points.
- No artwork or image assets are generated, edited, or loaded by this module.
- Makefile already discovers source/*.cpp automatically.

## Not signed off yet

This is the Step 3 foundation, not a completed faithful world generator. Exact Construct ACE definitions for the event command IDs are still unavailable in this repository. Translating tilemap writes, random selection, map-size changes, bunker assembly, bridge states, and spawn conditions before resolving those definitions would require guessing. The next pass must resolve command definitions and trace each child group before implementing the procedural algorithms.

## Verification boundary

The module uses standard C++17 facilities. A successful CI build would verify compilation, not prove that the generated world matches the original game in an emulator or on hardware.
