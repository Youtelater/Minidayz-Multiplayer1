# Stage 1 — World system (Step 3) audit

**Source:** authoritative Construct export snapshot under data/stage1/source_snapshot/. **Branch:** main.

## Source facts verified

- The Map layout is 36,850 × 20,000; TD has the same dimensions.
- Game_events group Map generation has UID 6835885414231505, 3,062 nested event nodes, 6,413 condition entries, and 14,371 action entries.
- Direct child groups: Bunker_generate (443 nodes), Bridge level (63), Z_spawners (54), MiniMap_legend (7), Location loader (1,660).
- The source includes calls named Bunker_spawn and Load_bridge_element, and variables including Map_size_X, Map_size_Y, SEED_TD, TilemapspotX, and TilemapspotY.
- The source snapshot retains object indices, numeric command IDs, parameters, and subevents.

## C++ work in this slice

- WorldSystem retains the verified source map dimensions as defaults.
- Added finite/bounds validation, coordinate clamping, seed storage, and a feature registry for terrain, roads, buildings, bunkers, bridges, and spawn points.
- Added source-layout loading: it copies each placed instance's layer, object index, UID, position, size, angle, opacity, scale, and origin into WorldPlacement records.
- Source placement coordinates are preserved verbatim, including negative coordinates; no unverified coordinate-origin conversion or object classification is applied.
- The Map layout contains 1,250 source placement records. This is a faithful static layout-data foundation, not procedural map generation.
- The graphics conversion input now points at assets/gfx/, the repository's original artwork folder. Converted textures continue to be emitted under romfs/gfx/ for runtime loading.

## Not signed off yet

This is a stronger Step 3 foundation, not a completed faithful world generator. Exact Construct ACE definitions for the event command IDs are still unavailable. Translating tilemap writes, random selection, map-size changes, bunker assembly, bridge states, and spawn conditions before resolving those definitions would require guessing. The next semantic pass must resolve command definitions and trace each child group before implementing procedural algorithms.

## Verification boundary

The source uses standard C++17 facilities and the existing generated source tables. A successful CI build would verify compilation, not prove that the generated world matches the original game in an emulator or on hardware.
