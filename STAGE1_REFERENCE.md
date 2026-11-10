# Stage 1 — Original Mini DAYZ source audit

Repository: `Youtelater/Minidayz-Multiplayer1`
Branch: `main`
Target: faithful C++ / Nintendo 3DS port.

## Scope rules

- Use `main` as the working branch.
- The supplied `data.deobfuscated.js` Construct export is the authoritative source for project structure and event logic.
- `romfs/` is now permitted for inspection only as legacy/reference material. Treat it as outdated and unverified; cross-check every candidate against the authoritative export and current repository assets before reuse.
- Do not use `raw/` or `c2injt.js` as extraction sources.
- Exclude every file ending in `mp.json`; non-JSON multiplayer material is reserved for the later multiplayer stage.
- Do not modify original media assets.
- Keep project-owned `.cpp` files under `source/` and `.hpp`/`.h` files under `include/`.

## Verified source inventory

The original Construct export is 14,558,329 bytes and has been structurally parsed. The reproducible extraction and exact counts are documented in `docs/STAGE1_STATUS.md` and `docs/STAGE1_EVENT_AUDIT.md`.

| Source | Recorded inventory | C++ relevance |
|---|---|---|
| `bunker_json` | Construct tilemap; 225 × 74; 1,224 run-length tokens expand to 16,650 cells | Bunker geometry and tile IDs |
| `l_eng_ui.xml` | 597 text records/IDs | Menu, HUD, prompts and UI strings |
| `l_eng_items.xml` | 405 item records/IDs (including empty ID 0 entry) | Stable item IDs and display names |
| `l_eng_log.xml` | 220 text records/IDs (including ID 0) | Gameplay feedback/log messages |
| `l_eng_new.xml` | 528 text records/IDs (including ID 0) | Item descriptions and displayed stats/effects |
| `gfx/spritesheet.meta.json` | Metadata v2; one visible Base layer; 290 sprite entries; 512 × 512 manual cell settings; 4 columns | Sprite frame metadata and source-to-render mapping |
| Original media inventory outside legacy `romfs/` | Previously inventoried as 510 audio files (216 M4A, 294 OGG) and 2,316 image files; filenames and event mappings still require current cross-check | Audio and artwork |
| Localization XML inventory | 30 XML files across Czech, German, English, Spanish, French, Italian, Portuguese and Russian | Localization coverage |

XML record totals include empty ID-zero records where present. Duplicate English files with a ` 2` suffix must be compared before selecting a canonical copy. Treat the above older inventory as a source checklist; do not assume it is already linked to Construct events.

## Structural extraction verified

- 29 Construct project sections; version 1.6.0; canvas 1024 × 768.
- 1,429 object definitions across 20 plugin/type IDs.
- 8 layouts, 360 layers and 5,256 placed instances.
- 6 event sheets and 22,343 recursively counted event nodes.
- 610 variable declarations and 220 named event groups.
- 6,338 animation definitions and 14,505 image-frame references.
- 489 audio resource entries.
- 115 distinct condition command IDs and 206 distinct action command IDs.

The complete parsed source snapshot is stored in ordered chunks under `data/stage1/source_snapshot/`. Reassemble and regenerate indexes using:

```sh
python tools/stage1/reassemble_snapshot.py
python tools/stage1/extract_construct.py data/stage1/construct_project_parsed.json --out data/stage1
```

Generated C++ metadata belongs under `include/minidayz/generated/stage1_data.hpp`; project headers belong under `include/`; translation units belong under `source/`. Inventories and parsed non-C++ data belong under `data/stage1/`.

## Legacy ROMFS cross-check

The current `romfs/` tree contains 611 files: 295 WAV, 290 T3X, 12 JSON, 12 BIN, one PNG and one T3S. The legacy `romfs/data/extracted_audio_and_coords_v2.json` lists 489 unique audio filenames, and all 489 filenames match the 489 entries in the authoritative Construct export's `audio_resources.csv`.

This filename match is useful corroboration only. It does not prove that the old audio files, coordinates, sprite conversions, UI strings, or gameplay data are current or usable on 3DS. The legacy UI/log JSON dictionaries are candidate references and still need ID/content cross-checking. No legacy asset is accepted solely because it exists in `romfs/`.

## Remaining Stage 1 completion checklist

- [ ] Resolve all 115 condition and 206 action command IDs against exact Construct 2 editor-side ACE metadata (plugin/behavior `edittime.js` definitions), including parameters, object references, expressions and subevent semantics. Do not guess command names.
- [ ] Translate every event sheet and nested event into traceable C++ behavior specifications and implementation.
- [ ] Trace movement, collisions/physics, weapons, combat, damage, enemy AI, NPCs, loot, inventory, crafting, vehicles, survival stats, status effects and progression.
- [ ] Reconstruct procedural world generation, terrain, roads, towns, structures and spawn rules.
- [ ] Reconstruct UI states, HUD, touch interactions, menus, tutorial, login/loading and death flows.
- [ ] Map audio resources to exact event triggers and playback parameters.
- [ ] Verify sprite-sheet rectangles, animation timing, origins, imagepoints and collision polygons against permitted original assets.
- [ ] Verify save/load, progression, achievements, quests, perks and day progression.
- [ ] Produce a source-to-C++ traceability matrix and behavior-equivalence tests on the target platform.

## Stage 2 gate

The current `source/main.cpp` is a clean platform entry point, not a gameplay implementation. The structural extraction is in place, but semantic gameplay translation and behavior verification are not complete. Do not begin Stage 2 or sign off Stage 1 until the checklist above is resolved and tested.
