# Original MiniDAYZ ↔ Nintendo 3DS: building loading and drawing audit

**Date:** 2026-11-22  
**Source of truth:** repository-root `data.deobfuscated.js`, `c2runtime.js`, and canonical `assets/gfx`. Do not treat this audit document, a cached generated table, or `Minidayz-Multiplayer-main` as the original-game source.

## Original Construct game

1. The original **Map** layout holds authored objects and Map layers. The world-generation event graph builds its cell/point-of-interest array and invokes `create_map_element` (event UID `7736925111997576`) through `Game_events`.
2. The source **Mapgen508.var1=7** branch has **51** literal building-variant selectors. Its actions create a primary Sprite, followed by `Spawn another object` actions that attach interiors, roofs, doors, wall-zone triggers and shadow sprites at authored *frame imagepoints*. The source first-frame hotspot, object dimensions, object-layer assignment, SetAnimation/SetFrame, per-child opacity, width/height expressions and active mask matter.
3. The source category-7 building branch contains **153** explicitly authored Sprite218 loot-marker spawns, independent of the visual house base/roof. Not every marker immediately calls `Trigger_spawn`.
4. Construct draws an instance's **original animation/frame rectangle** from its PNG, located by its frame hotspot and transformed by Sprite scale/rotation. Layers control order: `Buildings_shadows` (9), `Buildings_base` (17), `Buildings_base_top` (20), `Buildings_bg` (31), `player` (32), `Buildings` (41), `Trees` (43). Source `House_transparenting` (root UID `5249390718058362`) moves roofs/foundations between layers and changes alpha as the player overlaps them.

## Current native 3DS implementation

1. `WorldSystem::begin_source_generation()` loads the original **Map** layout, then performs grid creation, map markers, root-derived location-event proposals, bunker locations, bridges and linked closed-door Solids. This is **not executing the original browser runtime directly**; it is a native C++ translation of source events and tables.
2. `WorldSystem::expand_map_element()` matches the original 51 category-7 primary Spawn descriptors and original companion actions. Imagepoints are generated from the root source by `tools/stage1/build_source_house_companions.py`; object frame geometry is read from source-derived `source_building_frame_geometry`. This corrects a previously confirmed bug that gave generated house foundations **zero** width/height.
3. `tools/stage1/build_sprite_catalog.py` reads the source's exact frame X/Y/width/height, extracts that rectangle from **assets/gfx** and packs it into `romfs:/gfx/*_pNNN.t3x`. The native renderer looks up the requested original object/animation/frame, not the entire sheet, and loads matching texture pages when needed.
4. `GameplayScene::render` queries only camera-visible placements, reorders by active source layer/creation order, filters originally hidden or zero-alpha objects, and draws the source frame with its authored angle, opacity and pixel scale. The root-derived roof/door interaction model also moves source sprites between layers at runtime.
5. **Audit fixes:** Source rotation now uses a shared original-frame hotspot-aware draw origin; camera culling uses the same transform. The 32-entry 3DS T3X cache now avoids freeing pages already submitted for drawing in the **current** GPU frame.

## Fidelity verdict by subsystem

| Component | Result | Evidence and limits |
|---|---|---|
| Root house variants | Source-matched for directly traced branches | 51 original category-7 primary Spawn variants, checked at extraction time |
| Root house companions and imagepoint positions | Source-derived | Exact Spawn-another-object action order/imagepoint references; original SetFrame/SetAnimation and size-delta handlers |
| Texture coordinates | Source-derived | All source Sprite rectangles extracted into T3X frames, not guessed PNG crops |
| Building sprite dimensions/hotspots | Fixed and regression-checked | All 51 variants tested, including six world maps with actual house placements |
| Map layer and visible sprite order | Reimplemented | Original named Map layers and creation order, plus dedicated 3DS depth scale |
| Roof/door alpha, frame and collision | Reimplemented, not established pixel-identical | Source events, frame masks and layer changes translated into C++; visual playthrough deferred |
| Procedural locations and RNG | **Not seed-by-seed identical** | Native grid/location orchestration and C++ random stream, rather than direct browser execution |
| Texture loading and memory | **Not architecturally identical** | Original Construct/browser image/GPU pipeline vs pre-cropped T3X and 32 cached pages; GPU resources limited on 3DS |
| Azahar visual output | **Not verified** | Native tests and a successful `.3dsx` compile cannot prove buildings visibly render on an emulator |

## Regression evidence and next checks

- Original-source house builder fails closed if 51 primary branches or 153 loot marker actions change.
- Native world tests generate authentic buildings on six fixed maps (118 / 108 / 77 / 57 / 174 / 181 primary foundations), with correct nonzero frame geometry, valid layer, and visibility-index inclusion.
- `tests/test_original_house_render_assets.py` verifies that original base/companion atlas frames are packaged as T3X (51 distinct base objects and 107 distinct companion frames previously observed).
- `tests/terrain_source_visibility_tests.cpp` now tests hotspot rotation and matching camera culling. This is wired into Native System Regression Tests rather than being only a dormant local test.
- Build and graphics-audit jobs must pass for the exact commit. Azahar remains deferred per project instruction; when ultimately used, verify **at a generated town**, not solely at the wilderness spawn.
- If the corrected build still fails to display houses at a town, inspect **T3X load failures**, page-capacity overflow and Citro2D resource limits first. Do not fabricate buildings, change original probabilities or relocate the original survivor spawn as a workaround.

**Conclusion:** The original content and the core create/attach/draw pipeline are deliberately source-derived, but the overall runtime is **not yet demonstrably identical**. The 3DS has different graphics loading, random-number execution and screen/camera constraints. Emulator pixel parity remains open; automated code and asset checks cannot replace it.
