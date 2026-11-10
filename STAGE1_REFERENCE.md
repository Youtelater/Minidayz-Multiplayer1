# Stage 1 — Original Mini DAYZ source audit

Repository: `Youtelater/Minidayz-Multiplayer1`
Branch inspected: `main`
Purpose: establish a source-backed specification for the faithful C++/Nintendo 3DS port.

## Scope rules

- Use `main` as the working branch.
- Do not inspect anything under `romfs/` (including its graphics/audio/data), `raw/`, or the mod file `c2injt.js`.
- Exclude MP JSON files from this single-player stage. Non-JSON multiplayer files are reserved for the later multiplayer stage and are not used here.
- Do not modify original media assets.
- Do not use the `raw/` folder as a source; it is duplicate data.

## Verified source inventory

| Source | Verified result | C++ relevance |
|---|---|---|
| `bunker_json` | Construct tilemap; 225 × 74; 1,224 run-length tokens expand to exactly 16,650 cells | Tile IDs, map dimensions and bunker geometry |
| `l_eng_ui.xml` | 597 text records/IDs | Menu, HUD, prompts and UI strings |
| `l_eng_items.xml` | 405 item records/IDs (including the empty ID 0 entry) | Stable item IDs and display names |
| `l_eng_log.xml` | 220 text records/IDs (including ID 0) | Gameplay feedback/log messages |
| `l_eng_new.xml` | 528 text records/IDs (including ID 0) | Item descriptions and displayed stats/effects |
| `gfx/spritesheet.meta.json` | Metadata v2; one visible Base layer; 290 sprite entries; 512 × 512 manual cell settings; 4 columns | Sprite frame metadata and source-to-render mapping |
| Original media tree (excluding forbidden paths) | 510 audio files: 216 M4A and 294 OGG; 2,316 image files | Audio and artwork inventory; individual event/animation mappings still need extraction |
| Localization XML inventory | 30 XML files across Czech, German, English, Spanish, French, Italian, Portuguese and Russian | Localization coverage |

The XML record totals count the source entries, including the empty ID-zero records where present. Duplicate English files with a ` 2` suffix are present and should be compared before selecting a canonical copy.

## Important limitation — Stage 1 is not yet complete

The authoritative Construct project data is `data.deobfuscated.js` (14,558,329 bytes; blob SHA `bd6fc44626d2f4ede48f9a6b83dedbb23ce06a9d`). The GitHub file connector returned an empty body for this file, and the raw-file fetch rejected it as too large. The original `data.js` is also 12,442,669 bytes and could not be retrieved through the available file fetch. Therefore the project layouts, event sheets, object properties, instance coordinates, animation definitions, event-to-audio triggers, and complete gameplay rules have **not** been exhaustively extracted or verified yet.

`c2runtime.js` (350,006 bytes) is the Construct 2 engine runtime, not the game's own event-sheet logic. It can explain engine behaviour, but it cannot substitute for inspecting the project's event sheets and object data.

The repository also contains `spawn_npc_arrays.txt`; its paths identify Construct project nodes named `Spawn_NPC`, but this alone does not establish the full NPC spawning rules.

## Remaining Stage 1 completion checklist

- [ ] Retrieve and parse the complete authoritative `data.deobfuscated.js` (or equivalent original Construct project data) without using forbidden folders/files.
- [ ] Enumerate layouts, layers, object types, object properties and placed instances, including exact coordinates, sizes, origins and z-order.
- [ ] Extract animation names, frame sequences, durations, sprite-sheet references and collision/polygon data.
- [ ] Map all original audio filenames to their actual event triggers and playback parameters.
- [ ] Trace and document single-player world generation, terrain/roads/towns/structures, loot, NPC/zombie spawning and AI.
- [ ] Trace and document player movement, collisions, combat, weapons, inventory, crafting, vehicles, survival stats, status effects, UI flows and save/load.
- [ ] Cross-check extracted IDs and references against the English XML files and sprite/audio inventories.
- [ ] Produce a C++ implementation map with source paths, verified constants, dependencies and unresolved questions.

## Stage 2 gate

Do not treat this audit as permission to approximate the original game. Stage 2 should begin only after the authoritative project data has been parsed and the world-generation/layout specification is verified. Keep this file updated with evidence and mark checklist items complete only after direct inspection.
