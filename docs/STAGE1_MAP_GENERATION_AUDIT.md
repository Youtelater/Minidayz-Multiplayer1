# Stage 1 — Map generation event-tree audit

Source: `Game_events` in the supplied `data.deobfuscated.js` Construct project export.

This audit records the original event structure and raw Construct command references. It does **not** claim that the map-generation logic has been translated into C++.

## Verified scope

- Group name: `Map generation`
- Group UID: `6835885414231505`
- Event nodes including the group marker: **3,062**
- Direct child group declarations: **5**
- Distinct condition object/command pairs in this subtree: **150**
- Distinct action object/command pairs in this subtree: **652**

## Direct child groups

| Child group | Event nodes including child group marker |
|---|---:|
| `Bunker_generate` | 443 |
| `Bridge level` | 63 |
| `Z_spawners` | 54 |
| `MiniMap_legend` | 7 |
| `Location loader` | 1,660 |

## Translation boundary

The source tree includes nested conditions, actions, expressions, object references, and subevents. The object names and numeric command IDs must be resolved against the corresponding Construct plugin/type metadata before their behavior can be faithfully rewritten. The numeric IDs are intentionally preserved rather than guessed into human-readable meanings.

The next semantic pass must trace, at minimum:

- world/map seed and initialization behavior;
- map tile and location loading;
- bunker construction and bunker entry/exit behavior;
- bridge state and bridge-level transitions;
- zombie spawn placement and spawn conditions;
- map/minimap legend and location presentation.

**Status: source group indexed; gameplay semantics and C++ behavior are not yet signed off.**


## Seed and random-number trace (source correction)

The `Game_events` variable table declares `SEED_RUN` as a numeric 0/1-style flag with default value `0`. The `Start events` group contains a `SEED RUNS` branch that tests `SEED_RUN == 1` and toggles the separate `SEED_gunnerzeds`, `SEED_VIP`, `SEED_morezeds`, `SEED_Wildlife`, `SEED_stalker`, `SEED_pubg`, `SEED_playcasual`, and `SEED_TD` flags. A separate start event sets `SEED_RUN` back to `0`. These are game-mode/challenge flags; they are not evidence of a numeric per-world seed.

The map-generation entry point is invoked through the `generate_array_locations`, `generate_array_roads_1`, `generate_array_roads_2`, `generate_array_tomap`, and `generate_mapgen_toelements` function events. The current native implementation uses `std::mt19937` to make the provisional C++ placement repeatable, but the original Construct random-expression semantics and call order have not yet been resolved. Therefore identical C++ seeds are repeatable in the port, but they are **not yet proven to reproduce the original game's world**.

`mp_worldseed.js` is a separate multiplayer-side helper file in the repository, not part of the original `Game_events` Construct export. Its PRNG/hash implementation must not be treated as the original single-player world-generation algorithm.

**Required next step:** resolve the original System/Construct random expression IDs and their runtime implementation, then port the same random-call order before claiming original seed compatibility. Do not rename `SEED_RUN` as a world seed or claim the current PRNG is source-equivalent.
