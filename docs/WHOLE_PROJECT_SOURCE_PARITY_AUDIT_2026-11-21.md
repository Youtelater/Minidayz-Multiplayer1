# Whole-project original MiniDAYZ ↔ native 3DS parity audit — 2026-11-21

**Verdict: INCOMPLETE / NOT ORIGINAL-GAME EQUIVALENT.** This document audits work already present on `main` against original source obligations and explicitly separates implemented/tested code from the unimplemented or unverified remainder. A passing .3dsx build is not an event-chain parity certificate.

## Source and reproducibility

- **Authoritative game:** root `data.deobfuscated.js` (structurally equivalent to root `data.js`), `c2runtime.js`, `mp_itemtable.js`, root localization XML, root `assets/gfx/`, and original `media/`; do not silently substitute `Minidayz-Multiplayer-main/` or the stale `data/stage1/source_snapshot/`.
- **Root difference warning:** archived snapshot differs from root in **1,622 event bodies, 311 objects and 6 layouts**. Older documents based on snapshot are leads, not independent proof.
- **Exhaustive original event index:** 22,343 event nodes, 32,919 conditions, 63,289 actions, 461 audio actions, 14,505 frame references, 1,735 directly referenced PNGs and 119 PNGs requiring classification (see `docs/ORIGINAL_WORLD_FULL_SOURCE_AUDIT.md`). These count **source obligations**, not translated behavior.
- **Live cross-system audit:** [successful CI run #44932251628](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44932251628) executed `tools/stage1/audit_started_systems_parity.py` against the **root original** and independently cross-checked its 461 audio actions against `audit_original_world_exhaustive.py`. The [five-file artifact](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44932251628/artifacts/14718311471) contains the exact nested original event paths, raw conditions/actions and function/audio references. A static index can identify candidate mismatches but cannot prove all actions are absent/present without actual native execution traces.
- **Latest observed game-code CI checkpoint:** 3DS build and native tests passed on `451df705` ([3DS build](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44931021633), [native tests](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44931021591)). This is compilation and native-test evidence, **not** Azahar or physical 3DS parity evidence.

## Audit by game system

The source event counts below **overlap between workstreams** because the original Construct group ancestry is shared; never add them together. Audio numbers are directly labeled original actions, not verified playback. Status **PARTIAL** means real native code exists without full event/action coverage; **BLOCKED** denotes a known missing main gameplay pathway.

| ID | System | Original grouped event rows | Original direct audio actions | Native evidence / verified foundation | Missing or not verified |
|---|---|---:|---:|---|---|
| A01 | World terrain, POIs, roads, bridges, bunkers | 3,258 | 6 | `source/world.cpp`, `source/location_event_runtime.cpp`; root extractors, fixed-seed tree counts, layouts and generated placements | Original 398 dynamic SetTile painting actions, exact Construct event condition/RNG ordering, layout transitions, original marker distributions, map rendering parity. **PARTIAL** |
| A02 | Trees, doors, roofs, houses, environment interactions | 656 | 29 | Six original source-gated tree rules/hitbox links, doors frame/sound dispatch, original 42 Construct family memberships, roof opacity, collision/hotspot corrections | **BLOCKED:** live projectile/melee → tree damage & HP initialization; original chop/fall audio; all door object 348 picker/collider behavior; full roof ancestry/layer changes; furniture/container interactions; complete persistence. |
| A03 | Items, loot, backpack, storage | 3,648 | 233 | `inventory.cpp`, `equipment.cpp`, `item_use.cpp`, stash/tent and lower-screen touch UI; original ID/texture metadata | **BLOCKED:** source item-use/drag/drop/conditional branches beyond native handlers; location category-15 loot markers observed absent in fixed-seed test worlds; full inventory effect and item/world audio reachability; in-game proof. |
| A04 | Hunger, thirst, illness, temperature and weather | 206 | 11 | Player/survival/stat systems, weather and rain source helpers | Source timing/stacking/all stat effects, injury/environment interaction branches, AI/player transitions and Azahar behavior. **PARTIAL** |
| A05 | Crafting and construction | 82 | 1 | `crafting.cpp`, original recipe extraction and transaction tests | Every source craft/expression condition, wait/animation, building-mode placement, tool wear, production/drop and world integration. **PARTIAL** |
| A06 | Player, clothing, skins, movement | 1,599 | 170 | Original sprite-frame/hotspot and equipment catalogs; player and animation native code | Original AltMove speed/acceleration, all equipment overlay states, original movement/camera/player hit reactions, original exact spawn RNG, full emulator comparison. **PARTIAL** |
| A07 | Firearms, reload, melee, projectiles, explosions | 2,018 | 27 | Player input and some attack/weapon primitives; item tables and source event indexing | Complete native combat event-chain and projectile collision/damage, magazines/ammo/reload, melee hit, grenades, enemy and obstacle effects, original sounds. **BLOCKED** |
| A08 | NPCs, zombies, animals and spawning | 3,694 | 0 | `npc_system.cpp`, source spawn definitions and skin visuals | Original AI state machines, aggro/ranges, attacks, deaths, drops, persistent position, spawn source conditions and timed/despawn rules. **BLOCKED**. A grouped 0 **direct** audio count does NOT imply no NPC sound elsewhere. |
| A09 | Vehicles | 380 | 0 | Source vehicle/structure refs indexed; some world placements | Driving/steering/collision, vehicle audio, fuel, repairs, entering/exiting, passenger/state/saving and original branches. **BLOCKED**. Grouped audio count is not a proof of no sounds. |
| A10 | Boot menu, HUD, map, quests and achievements | 8,230 | 103 | Native 3DS boot/new/continue, original menu art, achievements/unlock metadata and touch layout | Source menu event chain, unlock progression, language switching, quest triggers, HUD overlays, original flow/animation/sounds. **PARTIAL** |
| A11 | Save, continue, progression, multi-layout and network | 2,192 | 0 | Native save & visual-state journal, world seed rebuild; item save migration tests | Original dynamic tree HP/destruction, dropped loot, doors/colliders, containers, NPCs, vehicles and timers; level layout transitions; multiplayer is not implemented. **BLOCKED** |
| A12 | Audio, image reachability, animations/performance | 415 | 17 | Preconverted `romfs/gfx` / `romfs/wav`, actual menu/inventory/door audio; frame/hotspot atlas logic | Trace **all 461** original sound action sites through live native triggers (individual sounds may be repeated); classify 119 PNGs; all animation/frame branch states; real GPU/FPS/3DS tests. **PARTIAL** |

### High-priority event-chain traces and known missing actions

| Priority | Root-original event/source anchor | Existing native layer | Audit requirement / observed gap |
|---|---|---|---|
| P0 | Tree hits: `8754965976446553` / `5386206472407495` / `7227858680569601` / `9193333425135665` / `4761103342332918` / `3531921662641626` | `source_tree_fall_rules.hpp`, `source_world_event_effects.hpp`, `world.cpp` | The tree fall handler exists but live `main.cpp` was **not** found to call `apply_source_tree_hit`; check projectile object **1031**, inherited hit guards, source HP initializers, chop/treefall audio, item spawn, collider and persistence. |
| P0 | Door open `6061166023614368`, close `243854400993751`, auto-door `5688415260272274` | `source_door_auto.hpp`, `main.cpp` | Door graphics/audio dispatch exists; missing full object **348** companion, per-instance source picker/collision polygon/timing parity. |
| P0 | House opacity `5249390718058362` | `source_roof_interaction.hpp`, `main.cpp` | Original **1041/1048/1049 are families**, not sprite IDs. Native correctly resolves members but does not yet implement complete inherited object overlap/layer/shadow/interior state conditions. |
| P0 | Original `LOOT_respawn` `3772264929481614`, `spawn_drop` `5418835019314848`, `Trigger_spawn` `7952760861768009` | `location_event_runtime.cpp`, `main.cpp`, `original_timed_loot.hpp` | Category-15 marker creation absent in tested generated worlds. Do not incorrectly use zombie `zed_resp` marker object **218** events as an explanation for missing ground loot. |
| P1 | Original inventory groups `3544291115846117`, `1529638233252698`, `115345084350351`, `594580094599696`, `795480497256684` | `inventory.cpp`, `item_use.cpp`, `main.cpp` | Map each inherited action and side effect against item IDs, use/discard/drag/equipment stat and sounds. See `docs/STAGE2_INVENTORY_SOURCE_PARITY.md`. |
| P1 | Original menu groups `5021901739871839`, `4625067508108619`, `890899747398625`, `928340114546882` | `main.cpp` | Preserve unlock rules, quest/progression effects, menu option/locale and sound/animation differences. See `docs/MENU_SOURCE_PARITY.md`. |

## Action-level completeness gates (apply to every event)

For every original event, create a row keyed by original event UID **and nested sheet path** (UID alone may be ambiguous), including its ancestor/picker conditions and RNG order. Preserve original action index and Construct ACE numeric tuple. For each action verify:

1. Is the event actually dispatched in native gameplay when its source conditions hold? Are picked/created family members and reference objects identical?
2. Do position, scale, Imagepoint, sprite-sheet coordinates, origin and animation/frame/layer/opacity changes match?
3. Are hitboxes, polygon collision, physics, invulnerability, destruction and object creation correct?
4. Are inventory rewards, source item IDs, chances, degradation, crafting and world drops correct?
5. Are WAV name, trigger condition, volume, timing and overlaps correct (including function-called sounds)?
6. Are timers, sequence of condition evaluation, state variables, quests, stat changes, NPC/vehicle response and save/reload correct?
7. Did source-vs-native seeded unit/runtime traces pass, and did Azahar and then real 3DS show expected visuals and inputs?

Mark **PASS** only with source path + native function + test/CI + gameplay evidence. Mark **MISSING**, **DIVERGENT** or **UNVERIFIED** rather than equating source indexing to behavior. Source action count is not a percentage complete.

## Audit findings versus implementation tasks

**Confirmed for this audit:** a cross-system source-action index successfully executed and was independently consistency-checked; candidate native implementations, important event UIDs and known gaps above have been reconciled with current repo docs and source implementations. **Not completed:** manual evaluation of all 63,289 source actions against native code, exhaustive event-dispatch runtime instrumentation, Azahar gameplay capture or a claim that every missing action is identified. Closing that gap requires action-by-action equivalence traces, not more declarations that the audit is 100%.

**Next audit-to-code correction order:** (1) live tree hit events + source HP and sounds, (2) door companion/polygon and roof inheritance, (3) missing world loot-marker source chain, (4) item use/crafting and survival effects, (5) combat/NPC/vehicle and persistence, (6) quests, menu, audio, sprite and full emulator/hardware signoff.

Do not mark Task 1 World Interactions complete until its entire chain, including audio, loot rewards, collisions and state restore, passes on-device gameplay verification.
