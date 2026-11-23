# Step 4 — Original MiniDAYZ player comparison and Azahar test handoff

Canonical reference: root `data.deobfuscated.js`, extracted `data/stage1/construct_project_parsed.json`, original PNGs under `assets/gfx`, and the current Nintendo 3DS runtime on `main`.

## Source-derived and implemented

1. The root Construct Map puts the visible player on layer 32, with pants, hands, clothing, vest, helmets, backpacks and weapons on layers 33–39. Native 3DS uses `source_world_layer_depth` for these sprites instead of arbitrary .57f–.605f depths, preserving the layers around Buildings_bg31 and Buildings41.
2. All 20 selectable player skin objects use the original idle/run directional animations and source spritesheet frame coordinates. The original default character is object679 (skin `def_2`). Native movement selects its original run animation while moving; no invented walk artwork.
3. The true movement/collision carrier is Sprite193 PlayerBase, original frame 2×2 with hotspot (1,1), Map UID924 2×2. Native now checks the 2×2 footprint offset (-1,-1) from the hotspot rather than a guessed 10×14 centered rectangle, with axis-by-axis wall slide and collision substeps. This is a bounding-rectangle approximation; polygon-pixel parity remains to be tested.
4. Original AltMove MaxSpeed action UID156884066447061 is `100 - var6 - var7 + var4 + var3 + var5 + var8`. Native supplies verified runner (+5 / +15) and perk_regen (+10) bonuses through existing perk state. It does not invent remaining context-driven energy drink, damage, terrain, or equipment events; those belong to later gameplay steps.
5. Original dead character artwork is Sprite813 `player_skin_dead-sheet0..2.png` with `id_1..id_20` animations; selecting the proper death pose requires additional source actor/skin variable mapping. Final death animation/UI integration is intentionally not claimed complete by this movement step.

## Verification and game file

- Native test run **passed**: https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45092749626
- Original graphics audit **passed**: https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45092749574
- 3DS DevkitPro build **passed**: https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45092749577
- The verified `3DS-Executable` artifact contains the build to extract and open in Azahar. This compile-and-source verification is not a substitute for an Azahar test.

## User-side Azahar checks

Start a fresh world, move in all four directions with the Circle Pad and D-pad, approach walls, trees and doors, test sliding beside a wall, enter a building, observe roofs/foreground objects and player equipment sprite layering, then report screenshots of any missing graphics or abnormal collision. Do not overwrite existing save files unless explicitly intended. The gameplay testing phase is now handed off to the user, as requested.
