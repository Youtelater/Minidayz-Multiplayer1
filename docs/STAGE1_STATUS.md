# Stage 1 — Original Construct project extraction

**Source:** `data.deobfuscated.js` / `data.deobfuscated.js` deobfuscated export  
**Repository branch:** `main`  
**Target:** faithful Nintendo 3DS C++ port

## Scope guardrails

- Inspect the original Construct project data and permitted original assets.
- Do not use `romfs/`, `raw/`, or `c2injt.js`.
- Exclude files ending in `mp.json`; multiplayer non-JSON material is reserved for its later phase.
- Do not treat the source data export as a substitute for the actual game logic.
- Preserve original dimensions, transforms, UIDs, object IDs, event-sheet order, sprite references and audio resources.

## Structural extraction already verified from the uploaded export

- 29 top-level project sections; project version 1.6.0; canvas 1024 × 768.
- 1,429 object definitions across 20 Construct plugin/type IDs.
- 8 layouts, 360 layers, 5,256 placed instances.
- 6 event sheets, 610 event variables and 39 named event groups.
- 6,338 animation definitions and 2,194 object-to-image references.
- 489 audio resource entries.
- The generated C++17 data header was compiled with g++ using strict warnings and count assertions.

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

## Repeatable extractor

Run:

```sh
python tools/stage1/extract_construct.py data.deobfuscated.js --out src/generated/stage1
```

It emits the parsed complete project, manifest, placed-instance CSV, animation CSV, image-reference CSV and audio-resource CSV. The C++ type definitions are in `src/data/construct_stage1_types.hpp`.

## Remaining work required before Stage 1 can be signed off

Structural extraction is not the same as semantic reverse engineering. The original event-sheet trees are preserved in the parsed project, but the following work must still be completed and verified against the source:

1. Translate every event condition/action and subevent into a traceable C++ behavior specification.
2. Trace the player controller, collision and physics rules, weapons, combat, damage, enemy AI, loot, inventory, crafting, vehicles and survival systems.
3. Trace procedural map/world generation, terrain, buildings, spawn tables and NPC/zombie spawn rules.
4. Reconstruct all UI states, transitions, HUD placement and touch-screen interactions.
5. Map each audio resource to its event trigger and playback parameters.
6. Verify animation frame rectangles, origins, collision polygons and sprite-sheet crops against permitted original assets.
7. Verify save/load, progression, tutorial and menu/login flows.
8. Produce a source-to-C++ traceability matrix and compile/run regression tests.

**Stage 1 status: structural export parsed; semantic gameplay specification and asset cross-checks remain in progress. Stage 2 must not start until those items are verified.**
