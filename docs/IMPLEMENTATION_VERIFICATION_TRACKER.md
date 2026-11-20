# MiniDAYZ 3DS — Implementation Verification Tracker

**Branch:** `main`  
**Created:** 2026-11-20  
**State:** Initial audit checklist; **no feature is claimed fully verified yet**.

This is the persistent checklist for the MiniDAYZ / MiniDAYZ+ Nintendo 3DS port. Update it as development proceeds. It is not a list of features already implemented, nor a claim that the original game contains every listed mechanic. Confirm feature presence and exact behavior in the original source first.

## How to mark an item complete

- Keep `[ ]` until the original-game source behavior has been identified, C++ implementation is integrated, correct original image frames and coordinates are mapped, relevant sounds and interactions are connected, unit/regression tests pass, and Azahar gameplay has been verified.
- For hardware-dependent features, real 3DS testing is additionally required before release. A GitHub Actions build passing **does not** constitute gameplay verification.
- For source-conditional features (e.g. traps or specific combat interactions), confirm that the feature exists in the relevant original version; mark N/A only with evidence and an explanation.
- When checking an item, add an evidence row below with original source location/event IDs, changed C++ files, commit SHA, CI run, and emulator/console test notes. Record failures and follow-up fixes explicitly.
- Leave original `assets/gfx/` PNGs unchanged. Match Construct atlas frame rectangles/animations, origins, scale and object states to converted `romfs/gfx/` textures. Map original audio events to `romfs/wav/`; do not count mere asset presence as used/verified.

## Non-negotiable original-source-first implementation rule

**Before implementing or modifying ANY gameplay system, inspect the matching original MiniDAYZ source events and asset references first.** Never implement gameplay effects, visuals, audio, rewards, frequencies, timing, or interactions by assumption.

For every action or state change (for example opening a door, chopping a tree, firing a weapon, entering a house, picking up an item):

1. Locate the original Construct object ID and the exact conditions/actions in `data.deobfuscated.js`, relevant runtime behavior in `c2runtime.js`, and any applicable original `mp_*.js` logic.
2. Trace the **entire triggered event chain**, including any audio playback (file, event trigger, volume, looping), visual/frame/animation changes, object creation/destruction, collisions, item or resource rewards, timers, UI feedback, and save/network implications.
3. Link each referenced asset to its original `assets/gfx/` sprite-frame coordinates or `media/` audio file and the converted 3DS counterpart. An event may have **multiple simultaneous effects**, none of which should be silently omitted.
4. Implement the linked effects together in native 3DS code without inventing unspecified game behavior. If a rule cannot be established from the source, mark it **unverified** and investigate before claiming parity.
5. Add automated checks and gameplay test scenarios for all verified effects, including graphics, audio, physics, loot, and persistence where applicable; attach evidence in the feature log.

**Example, not an assertion about any specific door:** if the original event for opening a door also plays a sound, changes the door sprite, changes collision, and updates state, all those effects must occur together in the 3DS port. Verify whether such a sound and other effects really exist in the original door event before implementing them.

## Source provenance gate — REQUIRED before world gameplay implementation

- [x] Run complete original event/sprite/instance/source-data indexing directly on root `data.deobfuscated.js` (source evidence: [completed audit workflow](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44833659709)).
- [x] Compare old extracted Construct snapshot to root: **1,622 event records, 311 objects, 6 layouts differ**. Source version is **not** interchangeable.
- [ ] Rebuild generated 3DS world/object/animation tables from the root source; invalidate any outdated `data/stage1` and generated native data.
- [ ] Verify all original world event conditions and effects per feature against the root trace before implementing or claiming original-game parity.
- [ ] Investigate all 119 PNGs without direct original sprite-frame references for tilemap/custom/non-frame usages; document source-unused files with evidence.

See [Root Original World Source Readiness](ORIGINAL_WORLD_SOURCE_READINESS.md) for the complete, reproducible event catalog, data provenance and world development order. **Research indexing is complete; world gameplay fidelity is not.**

## World implementation started — authoritative source generation (2026-11-20)

**Work integrated into `main`, not complete:**

- Native 3DS GitHub Actions now regenerates the Construct object/layout/animation tables from root `data.deobfuscated.js` before tests and `.3dsx` compilation.
- The original location event interpreter and procedural environment/ground scatter tables now regenerate from root source, **not** the stale 12.4MB cache.
- NPC spawn call/definition tables also regenerate from root source; 162 original `Spawn_NPC` call sites were recovered.
- The isolated root-source world generation test passed [CI #44835512491](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44835512491). Generated Trees-layer placement counts for fixed test seeds and world levels 0–5: **1,676 / 1,715 / 1,641 / 1,591 / 4,117 / 4,327**. These are regression observations, NOT source-prescribed vegetation-density constants or on-screen image counts.
- An automated regression guard prevents accidentally returning to severely sparse forest maps after root-source regeneration.

**Still unverified:** actual Azahar/physical 3DS forest density, exact Construct RNG/placement parity, source-backed doors, roof/interior state transitions, tree chopping/audio/drops, real generated loot-marker creation and persistent interactive object state. Native tests and a successful build do not prove these mechanics are complete. Some pre-existing generated tables (including `world_source_data.hpp`) still need separate root-source provenance verification.

**Development sequence:** first regenerate and verify entire root-derived map pipeline; then implement interactive vegetation, houses/interiors/doors, roads/bridges/bunkers, active loot spawning, state/saves, and living-world systems. Each feature must include original event conditions and all effects (graphics, audio, collision, rewards, UI and persistence).

## Current priorities

**First:** correct source-authored procedural vegetation/tree distribution, actual on-screen visibility, tree frame changes on damage/chopping, resource drops, ground pickup and inventory state, collision, sound and persistence.  
**Then:** source-driven roofs/interiors/doors, road/location generation, and interconnected item loot tables. Complete each end-to-end system rather than simply placing images.

## World and environment

- [ ] Original terrain and biome generation
- [ ] Roads, bridges, islands, towns and location generation
- [ ] Trees, bushes, vegetation and harvesting
- [ ] Buildings, roofs, interiors, doors and furniture
- [ ] Collision, visibility, shadows and draw order
- [ ] Weather, temperature, day/night and lighting
- [ ] World objects, destruction and state changes

## Items, loot and survival

- [ ] Every original item definition and graphic
- [ ] Item spawning, rarity, loot tables and respawning
- [ ] Location-specific and container loot
- [ ] Ground pickups, drops and item stacking
- [ ] Backpacks, storage, equipment and item durability
- [ ] Crafting, recipes and resource gathering
- [ ] Hunger, thirst, health, bleeding, infection and illness
- [ ] Clothing, protection, temperature and healing

## Combat and living world

- [ ] Player movement, stamina, animations and controls
- [ ] Melee combat, firearms, ammunition and reloading
- [ ] Weapon damage, condition and accuracy
- [ ] Zombies, NPC types, AI, movement and attacks
- [ ] Enemy spawning, loot and difficulty scaling
- [ ] Vehicles, driving, fuel, repairs and collisions
- [ ] Traps, fire, projectiles and explosions where present in original source

## Game systems and release

- [ ] Main menu, HUD, inventory, map and touch controls
- [ ] Sound effects, music and environmental audio
- [ ] Tutorials, notifications and localization
- [ ] Quests, perks, achievements and progression
- [ ] World persistence, saves and loading
- [ ] Multiplayer synchronization and state handling
- [ ] Performance, memory, crashes and 3DS hardware limits
- [ ] Complete original-source parity and real-device tests

## Audit dimensions for every feature

- [ ] Original object definitions, object IDs, instance variables, source layers and generation rules identified
- [ ] All relevant animations/frames/frame coordinates and state transitions identified
- [ ] Original event conditions, actions, timing, probabilities and dependent actions identified
- [ ] Every applicable PNG and audio asset linked to its actual runtime usage (including hidden/alternate states)
- [ ] Interaction, collision, rewards/drops, inventory, save and multiplayer implications verified where applicable
- [ ] Unit/regression tests and 3DS GitHub Actions build pass
- [ ] Gameplay passes Azahar test (with screenshots and test steps recorded)
- [ ] Real 3DS Homebrew Launcher runtime passes hardware tests before publishing

## Mandatory final graphics and animation coverage

**Release requirement:** every original image in `assets/gfx/` must be individually audited and accounted for, not merely converted to a `.t3x` file. The historical asset count is 1,854 PNGs; regenerate the count from the repository at release rather than assuming it remains fixed.

- [ ] Generate a filename-by-filename manifest of every original PNG, its converted texture(s), and the relevant Construct object, animation, frame rectangle, origin, and gameplay use.
- [ ] Check every frame and alternate object state (e.g. standing/damaged/chopped tree, roof/interior, closed/open door, road variants, directional sprites, items, HUD, menus).
- [ ] Ensure every image/frame that is actually used in the original game is **reachable and displayed in the correct gameplay context** on 3DS; do not draw every variant simultaneously.
- [ ] Verify atlas/spritesheet sub-rectangles and transformations, including animated sequences, flips, rotations, scale, sorting and transparent edges.
- [ ] Confirm original game event logic activates the correct visuals and connects them to interactions, drops, audio, collisions and saving.
- [ ] Record any images not referenced by the selected original build as **unused in source**, duplicate, legacy, inaccessible, or intentionally excluded—with evidence; never invent a use just to reach 100%.
- [ ] Add automated coverage reporting (total PNGs, mapped/unmapped PNGs, referenced frames, reachable/unreachable states, missing `.t3x` assets) and perform gameplay spot checks on Azahar plus final 3DS hardware checks.

**Definition of 100% graphics coverage:** 100% of original PNGs **accounted for**, and 100% of sprites/frames **used by the target original game** correctly implemented and reachable in the port. This does not mean displaying all pictures on one screen or claiming unused source assets are active gameplay assets.

## Evidence log

| Date | Feature | Source reference / Construct event | C++ changes / commit | Automated test | Azahar | Real 3DS | Notes |
|---|---|---|---|---|---|---|---|
| 2026-11-20 | World frame-state and camera-culling foundations (partial) | Tree hit 8754965976446553 / 5386206472407495; door 6061166023614368 / 243854400993751; all 1,429 source object frame bounds | `11084734`, `75d025b3`, `20f35f0b`, `dae273f9`, `64ca021b`, `9aee02cd`, `531fb397` | Native instance-state tests passed on `dae273f9`; final CI pending | Not verified | Not verified | Source animation/frame states and conservative frame/hotspot culling added; no functioning door/chop/drop/roof transition asserted; all relevant feature boxes remain unchecked |
| 2026-11-20 | Initial checklist saved | Original source audit pending per feature | Checklist only | Not run for this document | Not tested | Not tested | All feature items intentionally unchecked |
| 2026-11-20 | Root-world regeneration and generated vegetation | Root `data.deobfuscated.js`, `create_map_element` UID 7736925111997576 | Root table regeneration in 3DS CI; original world native tests | [Native success #44837516090](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44837516090); 1,676–4,327 trees across levels 0–5 (fixed seeds) | Not tested | Not tested | NOT complete: density and image/interaction parity still require Azahar and original-game comparison |
| 2026-11-20 | World loot-marker generation | Root `create_map_location` UID 8627807469654645; `create_map_element` UID 7736925111997576; Sprite 218 instance variable 0 | Original timed-loot marker category handling and source trace | Existing native world tests reported **0 in-bounds 218 markers, 0 eligible category-15 markers** across two new-world seeds; [source trace #44837840624](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44837840624) | Not tested | Not tested | BLOCKED: original trigger/marker creation path must be implemented before location loot can be claimed working |


## Original materials / implementation locations

- Original Construct data: `data.deobfuscated.js`, `c2runtime.js`, related original `mp_*.js` as applicable.
- Original textures: `assets/gfx/`; converted Nintendo 3DS textures: `romfs/gfx/`.
- Original audio and converted sound files: `media/` and `romfs/wav/`.
- Existing native implementation, dependent on current repository structure: `source/`, `include/minidayz/`.
- Related audit documents: `docs/ORIGINAL_PARITY_REVIEW.md`, `docs/original-world-state-audit.md` (where present).

**Completion rule:** never label a system 100% complete based only on compiling, an asset inventory, or a few successful screenshots.
