# MiniDAYZ original graphics versus Nintendo 3DS runtime — Step 1 source audit

Source: **root original** `data.deobfuscated.js`, `assets/gfx/`; never `images/` duplicates or separately modded source. Updated 2026-11-22.

## Complete original asset inventory

The existing [Original Graphics and Sprite Sheets CI](../.github/workflows/audit-graphics.yml) decoded the complete canonical PNG set:

| Original source graphics | Count |
|---|---:|
| Canonical original PNG assets | 1,854 |
| Preconverted, nonempty T3X assets | 1,854 |
| Original Construct animation-frame references | 14,505 |
| PNGs with multiple Construct frame rectangles | 895 |
| PNGs with one source frame rectangle | 840 |
| Explicit source tilemaps or other texture sources | 44 |
| PNGs without a direct Construct animation-frame reference | 75 |
| Referenced source PNG basenames missing from assets/gfx | 0 |

**Important:** `T3X present` does *not* mean the object should appear in gameplay, or that its original conditions are wired. `no_frame_mapping` does not prove an asset is unused; other runtime paths and tilemaps may use it. The graphics audit also records 17 original source frame rectangles that overlap (but extend outside) their canonical source PNG. The independent 3DS build validator now confirms **32** intentionally off-edge original frame rectangles have transparent padding in the packed atlases (build #45073236616); they are source-layout geometry, not unconverted frames.

## Runtime visibility is conditional, not “draw every PNG”

Original Construct object IDs are the stable authority for rendering, collision, and event actions. Our separate [visual-role trace](../tools/stage1/audit_original_visual_roles.py) walks all original event sheets and Map layout templates:

| Object | Original PNG | Actual role |
|---|---|---|
| 181 | `player_collision_base-sheet0.png` | Nonvisual player collision body |
| 193 | `player_base-sheet0.png` | Player position and event base |
| 218 | `spawn_loot_point-sheet0.png` | Loot event/category marker |
| 345 | `obst_base-sheet0/1.png` | Red obstacle/collision shape, not scenery |
| 346 | `warmzone-sheet0.png` | House/warm-zone interaction trigger, not wall art |
| 348 | `big_obstacle_base-sheet0/1.png` | Green building solid/collision mask, not wall art |
| 404 | `ach_city_counter-sheet0.png` | Original 1000×1000 magenta achievement counter, not scenery |
| 508 | `map_element-sheet0.png` | Mapgen source marker |
| 514 | `mapgen-sheet0.png` | Procedural map source controller |
| 857 | `polygon_target_base-sheet0.png` | Collision/target editor aid |
| 984 | `collision_checker_general-sheet0.png` | Weather/collision checker |
| 571 | `build_ghost-sheet0.png` | **Conditional** build preview; not always visible |
| 573 | `build_ghost_obstacle-sheet0/1.png` | **Conditional** obstacle/bed build preview; not always visible |

Source-specific exception: Sprite571 and Sprite573 are **genuine graphical art** when construction mode is active. Their Map layout templates are placed outside the playfield with zero opacity; their event actions are in `GUI → Building mode`. The 3DS renderer must preserve them as atlases and draw them only under the live build-event conditions, not as static world décor.

Original **real** artwork must still render: wall/exterior house321, visible doors349, genuine building shadows347, and live tree artwork184. Do not hide an entire asset just because it shares a family name with a mask.

## Runtime architecture and corrected bug

- Top-screen world renderer: `source/main.cpp` queries `VisibleSourcePlacementIndex`, preserves source Map layers and insertion order, filters static logic-helper IDs, then draws exact object/animation/frame via `Renderer::draw_source_sprite`.
- `include/minidayz/source_world_sprite_visibility.hpp` is the **generic top-world static-pass filter**, not a modification of the original Construct object or collision. All original `WorldPlacement` records and UIDs remain available to event code.
- Original image hotspots and frame dimensions come from root Construct's frame metadata via `tools/stage1/build_sprite_catalog.py`. The runtime does **not** draw a full atlas PNG for an item that belongs to a multi-frame sprite.
- Screen debug overlay now reports `TEX LOADS` and `DROPS`; the latter counts frames missing from the source catalog, T3X loads that fail, and exhaustion of all 32 in-use GPU atlas slots during the same frame. Invisible sprites avoid unnecessary GPU texture loads.
- `tests/test_source_visual_role_contract.py` compares the original frame mappings, original staging positions, original Building mode event ancestry, and the 3DS generic render gate.

## Current verification limits

**Original source classification:** complete PNG file inventory plus all authored object-frame references; the role/activation audit is strongest for the explicit world helpers listed above. It does not prove that *every* source action has been ported.

**3DS build and native tests:** required after any rendering change. Their success does not prove pixel-identical gameplay.

**Known important risks:** 3DS graphics memory and the 32 active sprite atlas pages; actual 3DS draws with up to 32 active GPU sprite atlas pages; offscreen source Map templates versus source-generated instances; original environment Tilemap patches, conditional visual effects, and UI all still need exact runtime and Azahar parity checks. Do not label the entire graphics engine “100% source-identical” until these checks are closed.

**User-approved testing plan:** run gameplay in Azahar after code/source corrections; visual issues not yet observed in the newest build must stay unverified.


## Source and build verification evidence

- [Original visual-role source trace #45073042851](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45073042851) — source role and Building mode event ancestry audit passed.
- [Original Graphics and Sprite Sheets #45073236623](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45073236623) — all 1,854 PNGs present, 14,505 original source frames indexed, 0 absent source PNG references.
- [Native System Regression Tests #45073236585](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45073236585) — player/world native tests passed, including 15 original root sprite role mappings, conditional green ghost usage and no always-on world mask sprites.
- [3DS executable build #45073236616](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45073236616) — success; original frame mapping validator checked **14,505 frames**, **1,812 converted atlas pages**, **1,735 source-frame sheets**, **32 off-edge frames transparently padded**.
- Gameplay in Azahar on the new build has **not yet been verified**. Passing these jobs is proof of source/table/build-level checks, not a claim of perfect rendering on screen.


## Exhaustive per-PNG source role register

Source event, object, and Map-template checks now produce
`audit-output/graphics/full_original_visual_role_inventory.json` covering **all 1,854 canonical PNGs**, not only assets referenced by sprite animations. For each file, it records original Construct object IDs, authored frame count, Map-template count, source event action-reference count, and safe role flags (nonvisual gameplay marker, conditional construction preview, visible/hidden template, or requires runtime analysis). Tilemap/texture references remain separate from sprite animation frames. This register cannot prove every later gameplay event is implemented, and does not mislabel unreferenced images as unused.

