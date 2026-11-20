# Original MiniDAYZ world state / sprite fidelity audit

Source of truth: root `data.deobfuscated.js` (Construct project, mirrored in `data/stage1/source_snapshot/part_000..019.jsonpart`), original PNGs in `assets/gfx/`, Construct frame catalog, and relevant `mp_*.js` for multiplayer. This document tracks *actual source evidence* versus the current native 3DS implementation. **A passing CI build is not proof of gameplay parity.**

## Confirmed original Construct sprite and event evidence

| Subsystem | Original evidence | Implication for port |
| --- | --- | --- |
| Pine / broadleaf trees | Sprite object IDs **184**, **207**, **209**, **576**, **577**, **581**; each has **2** frames in its `Default` animation; object **210** (tree block) has **4** frames. | Never assume frame zero permanently. |
| Chopped / damaged trees | `Game_events` > **Hitting_obstacles** contains obstacle-hit branches for trees 207 (event UID **8754965976446553**) and 209 (UID **5386206472407495**) and paired frame changes to **frame 1** (event UIDs **2738244684146745** and **7838407830495979**); additional equivalent tree branches must be traced. Audio catalog contains `chop`, `chop2`, `chop3`, `chop4`. | Reproduce health/state changes, the appropriate original animation frame, collisions, inventory drops, sound, and persistence together. |
| Trees / collision | `Game_events` > **Map generation / Location loader** contains source object **184/207/209/576/577/581** collision-distance decisions (e.g. events **206921476978674**, **150288675015061**). | Source uses proximity-dependent collision/physics behavior; static rendering alone is insufficient. |
| House exteriors and interiors | Object pairs **321→302**, **322→311**, **323→312**, **324→313**, **333→306**. Exterior art `b_city_house_1..4`, `b_piano_house`; matching interiors `*_int`. `Game_events` location-loader events **6055506806351951**, **4849857520268099**, **9738497725914856**, **2950646084128909**, **5307563843618601** spawn related interiors, exteriors, windows/shadows and doors. | Associate roof/exterior and interior instances by source event and location; implement original player-inside visibility conditions and render/layer ordering. No blanket hide-all-buildings shortcut. |
| Doors | Object **349** has distinct two-frame animations **white**, **ga_brown**, **ga_blue**, **brown1/2/3**, **city/2/3**, **shtab**, **firestation/2**, and more. `Game_events` > **Doors_auto** event **243854400993751** sets door frame **0**, with other events setting frame **1** (e.g. **6061166023614368**); `open_door`, `close_door` and metal variants are present in audio. | Persist individual door animation, open state, obstacle/collision state, interaction trigger, and sound. |
| Bunker doors | Object **787** has 3-frame `Default` and 2-frame `open`; `Game_events` > **Bunker_generate** contains numerous object **787** frame and animation actions. | Bunker doors are a separate transition path, not interchangeable with ordinary house door graphics. |
| Bushes / other mutable objects | Berry bush object **296** has color animations (`red`, `yellow`, `blue`, `black`), each with 2 frames. Fence obstacle **319** has 5 frames. | Audit collectible/harvested and destroyed states rather than treating flora and fences as immutable sprites. |
| Roads | Existing tile metadata/flags in `include/minidayz/terrain_source_tile_flags.hpp` and original location generation. | Preserve original source tile IDs, flips/rotation, transitions, and construction ordering. |

## Native 3DS findings (main branch, inspected 2026-11-20)

- `source/main.cpp` draws static `WorldPlacement` sprites via `renderer.draw_source_sprite(placement->object, 0, placement->source_frame, ...)` (around lines 2360–2430). It does **not** read mutable tree-chopped/door-open/house-entered sprite state for these placements.
- `include/minidayz/world.hpp` stores `source_frame` on placement records but has no corresponding complete per-instance animation-state and interaction-driven transition model.
- `source/world.cpp` and `source/location_event_runtime.cpp` extract or instantiate source location data, but no verified full original interaction event interpreter for all the behaviors above is wired into the scene.
- `source/engine.cpp` *can* draw exact source object/animation/frame combinations. The remaining failure is runtime selection of the **correct frame, animation, visibility and collision** based on source events.
- Regression tests include spawn/terrain checks, but full tests for (1) chopping a tree through to stump and drops, (2) entering and leaving a house to switch roof/interior, (3) opening/closing doors with collision and sounds, and (4) persistence are **not proven**.
- CI tree count instrumentation has shown low vegetation density in a sampled generated level; check original generation event coverage before introducing any arbitrary density multiplier.

## Implementation and acceptance checklist

1. Extract a machine-readable **per-object state table** from Construct sprite definitions: object ID, animation ID/name, frame index, texture and rectangle, hotspot, animation speed and loop setting.
2. Extract the **condition → action** chains from the original event sheets, including inherited parent conditions, selection/picking rules, variable comparisons, spawned/replaced sprites and sound cues. Verify each event UID and meaning against Construct runtime, not guessed op numbers.
3. Introduce a *per-world-instance* runtime state: original object ID/instance UID, current animation+frame, health, visible/solid flags, door open state, building interior/roof relationship, damage/loot state, and a save key.
4. Wire **world generation and animation** to source object state; render only active original frame with correct hotspot, coordinates, scale, depth and layer visibility.
5. Implement tree/foliage damage and harvesting, door interaction and auto-close, building roof/interior revealing, and collision changes using original event conditions.
6. Add deterministic native tests for initial state, every transition, associated collision, sound event, saving/reloading, and representative generation density near player spawn.
7. Test visuals and controls in **Azahar**, then validate memory, rendering, audio and game behavior on **real 3DS hardware** through Homebrew Launcher.

## Scope guardrails

Do not edit `assets/gfx` PNGs or treat `images/` duplicates as primary originals. Do not invent animation names or indices when the source explicitly defines them. Source `mp_*.js` extends multiplayer; multiplayer-specific behavior must not silently replace original single-player events. This audit is a source-backed **gap analysis**, not a claim of 100% feature completion.
