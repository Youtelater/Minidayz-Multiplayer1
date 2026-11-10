# Stage 1 — Original Construct project extraction and semantic port audit

**Source:** the supplied 14,558,329-byte `data.deobfuscated.js` export  
**Working branch:** `main`  
**Target:** faithful Nintendo 3DS C++ port

## Scope guardrails

- Use `main` as the working branch.
- `romfs/` may be inspected only as a legacy/reference artifact; treat its contents as outdated and unverified, never as authoritative extraction sources. Cross-check any candidate against the supplied Construct export and current repository assets.
- Do not use `raw/` or `c2injt.js` as extraction sources.
- Exclude every file ending in `mp.json`; multiplayer non-JSON material is reserved for the later multiplayer stage.
- Do not modify original media assets.
- Keep project-owned `.cpp` files under `source/` and `.hpp`/`.h` files under `include/`.
- Keep extracted non-C++ inventories under `data/stage1/`.

## Structural extraction verified from the supplied export

- 29 project sections; Construct version 1.6.0; canvas 1024 × 768.
- 1,429 object definitions across 20 plugin/type IDs.
- 8 layouts, 360 layers, and 5,256 placed instances.
- 6 event sheets; 22,343 nested event nodes.
- 610 event-variable declarations and 220 named event groups.
- 6,338 animation definitions and 14,505 image-frame references.
- 489 audio resource entries.
- The extraction script normalizes spaced negative numeric literals and emits the parsed project, event index, manifest, CSV inventories, and a generated C++17 header under `include/minidayz/generated/stage1_data.hpp`.

## Layout inventory

| Layout | Size | Instances |
|---|---:|---:|
| GUI_layout | 1024 × 768 | 302 |
| Polygon | 1500 × 1500 | 1,606 |
| Map | 36850 × 20000 | 1,250 |
| TD | 36850 × 20000 | 978 |
| Tutorial | 8300 × 2000 | 754 |
| Menu | 2500 × 1500 | 330 |
| Login | 850 × 890 | 30 |
| Loading | 2000 × 1500 | 6 |

## Reproducible extraction

The complete parsed source snapshot is now stored in ordered chunks under `data/stage1/source_snapshot/`. Reassemble it, then regenerate the indexes and inventories from the repository copy:

```sh
python tools/stage1/reassemble_snapshot.py
python tools/stage1/extract_construct.py data/stage1/construct_project_parsed.json --out data/stage1
```

Outputs:
- `data/stage1/source_snapshot/part_*.jsonpart` — complete parsed source snapshot, split into ordered Git-friendly chunks.
- `data/stage1/construct_project_parsed.json` — reconstructed complete parsed source project and event trees.
- `data/stage1/event_sheet_index.json` — nested event index, variables, groups, and command-ID frequency tables.
- `data/stage1/stage1_manifest.json` — source and extraction manifest.
- `data/stage1/instances.csv`, `animations.csv`, `image_refs.csv`, `audio_resources.csv`.
- `include/minidayz/generated/stage1_data.hpp` — C++17 constexpr metadata tables.

## Remaining work before Stage 1 sign-off

Structural extraction and indexing are complete, but the original game has **not** yet been fully translated into C++ or behaviorally verified. These items remain open:

1. Resolve all 115 distinct condition command IDs and 206 distinct action command IDs against the exact Construct 2 editor-side ACE metadata (plugin/behavior `edittime.js` definitions), including parameters, object references, expressions, and subevent semantics. The repository currently has no exact editor-side ACE definitions; see `docs/STAGE1_COMMAND_ID_MAPPING.md`. Do not guess command names.
2. Translate every event sheet and nested event into traceable C++ behavior specifications and implementation.
3. Trace player movement, collisions/physics, weapons, combat, damage, enemy AI, NPCs, loot, inventory, crafting, vehicles, survival stats, status effects, and progression.
4. Reconstruct procedural world generation, terrain, roads, towns, structures, spawn tables, and NPC/zombie spawn rules.
5. Reconstruct UI states, HUD placement, touch interactions, menus, tutorial, login/loading, and death flows.
6. Map audio resources to exact event triggers and playback parameters.
7. Verify sprite-sheet rectangles, animation timing, origins, imagepoints, and collision polygons against permitted original assets.
8. Verify save/load, progression, achievements, quests, perks, and day progression.
9. Produce a source-to-C++ traceability matrix and run regression/behavior-equivalence tests on the target platform.

## Current verified deep-dive

`docs/STAGE1_MAP_GENERATION_AUDIT.md` records the current structural audit of the `Map generation` group, including its nested event-node count, child groups, and exact object/command references. This is an audit index, not a completed gameplay port.

**Stage 1 status: structural extraction verified; semantic behavior translation, asset/event cross-reference, and behavior-equivalence testing remain incomplete. Stage 2 must not start until those items are completed and verified.**


## Legacy ROMFS cross-check (non-authoritative)

The user has authorized inspection of `romfs/` as legacy reference material. The current tree contains 611 files (295 WAV, 290 T3X, 12 JSON, 12 BIN, one PNG and one T3S). The legacy audio index contains 489 unique filenames, matching all 489 audio resource filenames in the authoritative Construct export. This is filename-level corroboration only; it does not validate the age, contents, format, coordinates, or runtime compatibility of the legacy assets. Legacy UI/log dictionaries remain candidates for cross-checking, not sources of truth.


## Converted WAV coverage

The 295 files under `romfs/wav/` cover all 275 unique audio stems present in the 489 source audio resource entries (extension-independent filename comparison). There are 20 additional WAV stems not found in the source audio resource list; treat them as legacy candidates pending event/reference verification. The source export includes multiple M4A/OGG variants for some stems, so the 489-to-295 comparison is by unique basename, not file count. This is filename coverage, not a playback/format compatibility test.
