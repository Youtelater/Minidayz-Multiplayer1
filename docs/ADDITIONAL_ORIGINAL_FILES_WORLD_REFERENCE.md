# Additional original source files — world implementation reference

**Audited:** 2026-11-20, root of `Youtelater/Minidayz-Multiplayer1` on `main`.
**Purpose:** Identify files useful beyond `data.deobfuscated.js`, `l_eng_items.xml`, and `mp_itemtable.js`, without confusing original mechanics with multiplayer extensions or older source caches. This is **a source inventory, not gameplay verification**.

## Authoritative game logic and its execution

| File(s) | Why useful | Use and limitation |
|---|---|---|
| `data.deobfuscated.js` and `data.js` | Construct objects, events, frames, layouts, source item/loot/spawn/door interaction conditions | CI has confirmed parsed-project equivalence between these two root files; use root version. |
| `c2runtime.js` | Numeric Construct condition/action semantics, collision, animation, picking, timing, layer transforms, engine execution | Essential to accurately decode numeric ACE IDs and inherited event conditions from source. |
| `index.html`, `index_new.html` | Actual browser script loading order and initialized extensions; includes Construct runtime and `mp_*.js` chain | Use to identify dependencies and avoid misinterpreting scripts as independent plugins. These are HTML runtime entry points, **not** 3DS UI implementation specs. |
| `offline.js`, `offlineClient.js`, `sw.js` | Offline resource/precache list, including original image and audio paths, and caching behavior | `offline.js` lists `data.js`, `c2runtime.js` and image paths such as tree sprites. Compare its manifest with `assets/gfx/` and `media/` to account for assets. Not evidence that every listed asset activates in gameplay. |

## World geometry, spawn data and source assets

| File(s) | Why useful | Use and limitation |
|---|---|---|
| `bunker_json` | Explicit original **Construct tilemap** serialized as `{"c2tilemap":true,"width":225,"height":74,...}` | Potential geometry reference for the bunker. Decode original tile IDs, run-length encoding, tile dimensions and placement rules before using; do not treat it as arbitrary JSON decorations. |
| `spawn_npc_arrays.txt` | Text list of original `Spawn_NPC` expression paths in Construct project data | Useful **search index**, not a full spawn table. Paths must be checked against root `data.deobfuscated.js` because old cached snapshots differ. |
| `assets/gfx/` | Original PNG sprites and sprite sheets, including trees, buildings, roads, terrain, UI, items, multiple frames | Primary original art. Preserve filenames and match exact Construct frame rectangles/origins. |
| `media/` | Original OGG/M4A audio (e.g. door, chopping, weather, vehicles) | Pair **each actual event trigger** with its sound and parameters. Duplicate formats do not mean separate game sounds. |
| `gfx/` | Separate `frame_001.png`, `frame_002.png` … images | Inspect provenance/duplicates against Construct frame references before adopting. **Not** a substitute for the original `assets/gfx/` set. |
| `images/` | Additional duplicate/partial image set | Do not use in place of original `assets/gfx/`; could help investigate path aliases or duplicate images where necessary. |

## Item IDs, text and gameplay feedback

| File(s) | Why useful | Use and limitation |
|---|---|---|
| `l_eng_items.xml` | Item ID → English item name | Preserve numeric IDs, including gaps. |
| `mp_itemtable.js` | `MDZItemTable` maps item ID → Construct object sprite ID (e.g. `t384`) | **Very important** for linking world drops and pickup images. Do not confuse source item ID with Construct sprite object ID. |
| `l_eng_new.xml` | Per-item descriptions, hints and displayed stats/effects in source text | Useful to check the intended effect of fire, food, clothing, tools, resources; **description alone does not prove actual mechanics**. |
| `l_eng_log.xml` | Game feedback strings and state outcomes: empty trunks, broken doors, NPC companion behavior, survival changes | Cross-check with Construct log-call conditions. Example source strings include “The trunk is empty” and “This door is completely broken.” |
| `l_eng_ui.xml` | Original UI, quest/task, map-loading and achievements labels | Includes “Generating world,” road placement, minimap creation and “Chop down 3/6/10 trees”; useful for finding missing world tasks and feedback. |
| `l_eng_items 2.xml`, `l_eng_ui 2.xml` | Additional copies of English localization files | Check exact byte or parsed-content equality before considering redundant. |
| `l_fr_*.xml`, `l_es_*.xml`, `l_de_*.xml`, etc. | Additional original localization content | Useful for eventual multilingual release; keep text IDs aligned with the original. |

## Multiplayer extensions and world synchronization

These are **real source files**, but they implement **multiplayer extensions**, not necessarily original single-player rules. Inspect them *as additional behavior* after comparing each mechanism to root Construct events.

| File(s) | Specific role supported by inspected file |
|---|---|
| `mp_worldseed.js` | References `generate_array_locations`, `generate_array_roads_1/2`, and `generate_array_tomap`; useful for deterministic multiplayer world consistency. |
| `mp_worldstate.js` | References `Day_survived`, `Timer_ALL`, `Timer_Hours`, `Timer_Minutes` world-state fields; useful for syncing changing environment/time. |
| `mp_interactions.js` | Multiplayer interaction code; source includes red/yellow/blue/black configuration references. Verify precise berry/object/door triggers in original Construct events first. |
| `mp_props.js` and `mp_props.json` | Multiplayer prop creation/update/removal and Construct property hooking. |
| `mp_structures.js` and `mp_structures.json` | Definitions of barricades, barbed wire, tents, snares, fences, beds and other buildable structures; JSON lists `tNNN` sprite definitions and part frames. |
| `mp_entities.js` | Multiplayer entity sync and sprite identities: inspect for zombie, NPC and other entity state representation. |
| `mp_players.js`, `mp_players.json`, `mp_playerstore.js`, `mp_playerstore.json` | Player inventory/state serialization, recovery/checkpoints and local persistence/role-specific save behavior. |
| `mp_events.js`, `mp_authority.js`, `mp_transport.js`, `mp_transport.json`, `lan_bridge.js` | Multiplayer events, authority and networking transport. |
| `mp_throwables.js`, `mp_throwables.json` | Multiplayer projectile/object definitions (grenades, flares, molotovs, smoke, airdrop crate). |
| `mp_runtime_compat.js`, `mp_runtime_combat.json` | Construct runtime field aliases and hook compatibility; helpful for diagnosing numeric runtime APIs, but not a substitute for original `c2runtime.js`. |
| `mp_kits.json`, `mp_kit.js`, `mp_overlay.json`, `mp_menu.js`, `mp_lang.js` | Multiplayer starter kits, overlays, menus, localization; later multiplayer/UI parity. |
| Other `mp_*.json` | Human-readable reference or configuration for multiplayer scripts | Examine provenance; not always runtime-loaded. Cross-check `index.html` load order. |

## Visual effects and platform-specific extras

| File(s) | Importance |
|---|---|
| `mdz_lighting.js` | Additional light-grade/fog/shafts and shader logic. Confirm this is an extension rather than original 1.6.0 behavior; implement only when applicable to target version. |
| `mdz_shader_menu.js` | Added shader-menu presentation logic, distinct from original world rules. |
| `cordova.js`, `cordova_plugins.js`, `plugins/cordova-plugin-file/` | Original web/mobile file APIs relevant to saving/export; native 3DS should reproduce user-visible save semantics, not embed Cordova. |
| `appmanifest.json`, `package.json`, `offline.js` | Distribution and resource metadata; not world-generation authority. |
| `www.7z`, `New folder.rar` | Uninspected archives. Contents and source version **unknown**; do not assume they contain missing mechanics or override root files. |
| `c2injt.js`, `data 2.txt` | Other large files. Their exact role/equivalence **has not been established**; review only if root-source or browser boot investigation requires them. |

## Critical work order for 3DS world development

1. **Authoritative root Construct + runtime:** `data.deobfuscated.js` and `c2runtime.js`, with exact event conditions and functions. Do **not** use the stale `data/stage1/source_snapshot` as source of truth.
2. **World location/data evidence:** `bunker_json`, current original `Spawn_NPC` paths, and Construct generation functions. Confirm relevant source paths in root original; no guesses about density.
3. **All world graphic/audio outcomes:** original `assets/gfx/` and `media/`; `offline.js` can help cross-check missing asset paths.
4. **Original item mapping:** `l_eng_items.xml` + `mp_itemtable.js` + `l_eng_new.xml`; determine object state/drop trigger in Construct before choosing rewards.
5. **World feedback/UI:** `l_eng_log.xml`, `l_eng_ui.xml`; use actual event conditions for text/quest status.
6. **Multiplayer-specific world features:** `mp_worldseed.js`, `mp_interactions.js`, `mp_props.js`, `mp_structures.js`, `mp_worldstate.js`, `mp_entities.js`; cross-check their behavior against original single-player event logic before merging.

## Known limitations / required follow-up verification

- The original-world event extraction correctly captures all root-source syntax, but **runtime numeric ACE interpretation**, world RNG sequence and full interaction branches still require validation. A list of file names is **not** a complete gameplay implementation.
- Original `assets/gfx/` count: **1,854** PNGs; latest root-frame audit matched **1,735** and left **119** without direct frame references. Compare `offline.js`, `gfx/`, and Construct frames to classify the remaining files.
- `spawn_npc_arrays.txt` may contain paths into an older export; revalidate each against the authoritative root project.
- `bunker_json` is a tilemap data reference; exact generation, layout use, tile IDs and collision still need a source-event match.
- Archives, alternative copies and injected/runtime additions should not silently override authoritative original gameplay.
- Keep using `docs/ORIGINAL_WORLD_FULL_SOURCE_AUDIT.md` and generated per-event audit outputs for game-logic evidence.

**Scope of this update:** source file discovery and categorization only. No world-gameplay code, original images or sounds have been changed.
