# MiniDAYZ 3DS — Implementation Verification Tracker

**Branch:** `main`  
**Created:** 2026-11-20  
**State:** Source indexing and several narrow implementation milestones verified; all broad gameplay/release completion gates remain open pending original-game parity and Azahar testing.

This is the persistent checklist for the MiniDAYZ / MiniDAYZ+ Nintendo 3DS port. Update it as development proceeds. It is not a list of features already implemented, nor a claim that the original game contains every listed mechanic. Confirm feature presence and exact behavior in the original source first.

## World original source detailed catalog — 2026-11-21

- [x] Source-backed inventory and world category baseline saved in [World Source Detailed Inventory](WORLD_SOURCE_DETAILED_INVENTORY_2026-11-21.md), with exact source counts and separate world-generation targets.
- [x] Root-original full object/animation, 62 template branch, layout/layer and editor-placement catalog extractor **implemented** in `tools/stage1/audit_original_world_catalog.py` and added to `.github/workflows/audit-original-world.yml`. **CI execution of this newly added extractor has not yet been independently verified here.**
- [x] Implement a **source-derived building candidate extraction** into `building_object_candidates.csv`, plus all original `1041/1048/1049` family members and literal world Create action site evidence. CI now checks source 62 templates, 5,533 writes and known original paired house/door IDs. These are building discovery checks, **not completion of building gameplay or an independently observed successful workflow run**.
- [ ] Execute/check the generated detailed catalog artifact, classify all actual house designs and building variants from source Array writes, and reconcile exterior/interior/roof pairings.
- [ ] Confirm generated per-world structure counts from original root event/RNG semantics and actual gameplay. A location template is not a house and source name matches are not a unique-house count.

## Whole-project source parity checkpoint — 2026-11-21

**The original-vs-native started-systems audit is complete as a static source index, NOT as a gameplay-parity certification.** See [Original vs 3DS Started Systems Audit](ORIGINAL_VS_3DS_STARTED_SYSTEMS_AUDIT.md) and [passing source comparison #44932346147](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44932346147). Original-source checks covered **22,343** events, **63,289** actions, all **461** audio actions, and classified **18,937** event rows into started systems; **70** audio actions currently lack a workstream classification but are preserved for further review.

**Definite gaps uncovered:** chopping/treefall sounds are supported in the native WAV player but have no live tree dispatch; actual player hit/projectile input does not call the original tree-hit executor. **Partial/unverified:** original door companion object 348, roof ancestor overlap/layers, dynamic world save state, generated location loot markers, and original audiovisual sequences for all started systems. **Nothing was silently checked as complete** just because a source action or WAV was found. Audit report, exact event-condition/audio CSVs and audio cue/WAV candidate crosscheck are archived by [the new reproducible workflow](../.github/workflows/audit-started-gameplay-parity.yml).

## Whole-project original-source parity review — 2026-11-21

**Audit verdict: NOT SOURCE-EQUIVALENT / INCOMPLETE.** Reviewed original source obligations across **12 started systems** against current 3DS code, previous specialized source audits, and successful root-source whole-game audit CI [#44932251628](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44932251628). The [full system-by-system audit](WHOLE_PROJECT_SOURCE_PARITY_AUDIT_2026-11-21.md) records source event counts, high-priority original event UIDs, known missing actions, verification criteria, and what cannot be certified by static analysis. The [source-event/action/artifact bundle](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44932251628/artifacts/14718311471) is reproducible.

- [x] Whole-project **source event/condition/action and audio obligation indexing** run successfully from root original; **this is an audit-tool milestone only**.
- [x] Root original versus 3DS **initial system gap ledger** saved, with known blockers explicitly distinguished from implemented modules.
- [ ] Interpret and map **all 63,289 source actions**, including inherited conditions, function-triggered effects, and RNG order, to actual native handlers; report action-level coverage.
- [ ] Verify **all 461 original audio action sites** have correctly gated live 3DS playback (not merely packaged audio files).
- [ ] Verify every original gameplay system in actual Azahar/real 3DS gameplay and save/restore; no complete parity is yet established.

**Do not treat these first two audit checkmarks as game completion.** Task 1 live tree hit and damage dispatch, tree sound/HP state, door companion object 348, full house transparency source conditions, world loot marker generation and complete dynamic persistence remain open.

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
- [x] Rebuild generated 3DS world/object/animation tables from root `data.deobfuscated.js` in build/CI; the older cached extraction must not override them. Verified native root-table workflow [#44839509781](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44839509781).
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

## World-object interaction identity and active loot implementation (2026-11-20)

**Implemented in native C++ on `main`, subject to Azahar/real-hardware verification:**

- `WorldPlacement` now distinguishes an original `source_uid` (the Construct event/authoring reference), a source/authored-or-generated `runtime_uid` (distinct object state identity), and deterministic one-based `instance_id` for fast per-instance lookup. This prevents repeated tree creation events from being treated as one shared tree when implementing damage, door states, container depletion and saving.
- The world generator assigns IDs to static Map instances, event-created sprites and procedural `create_map_element` objects. Reproducibility and duplicate-ID guards are exercised across fixed world seeds in `tests/world_system_tests.cpp`.
- The original loot marker `LOOT_respawn` instance variable is preserved by the source event runtime. Event-created object 218 markers can now enter the guarded timed-loot path only when their original category is valid; event-source predicates remain required.
- CI **Native System Regression Tests** passed on commit `2652d1da`: [workflow run](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44837619846). The new duplicate-ID diagnostic is included in later commits. A successful native test does not establish gameplay parity.

**Still missing/unfinished:** source-driven tree damage + falling + item rewards, door open/close sound/animation/collision chains, full roofs/interiors and furniture, exact item spawn chances in every location, state persistence, source RNG parity, on-device testing. Keep the overall feature boxes unchecked.

## Additional world runtime implementation — 2026-11-20

**Changes committed to `main`:**

- `WorldSystem` now captures original per-instance visual-state baselines and supports change-only snapshots, source UID-safe tree/door/roof frame replay, and all-or-nothing validation before restoration. A separate checksum-protected `world_visual.sav` journal ties those changes to the exact world seed and level without changing the existing v20 player save ABI.
- The gameplay scene now writes the world visual journal alongside the survivor save and restores matching world visuals after Continue regenerates the same map. New Game / next-level travel never replay stale visual state. This is **visual-state persistence**, not complete world-persistence parity: HP, door collision, destruction timers, containers and all other dynamic variables still need source-driven saving.
- `source_world_event_effects.hpp` implements **source-event-gated** per-tree fall actions for the six tree species in `source_tree_fall_rules.hpp`: exact sprite frame 1 and 2–4 corresponding source item IDs, with no duplicated drop after a tree has fallen. Original source-evaluated random coordinates must be provided; no fake spawn probabilities or chance-based drops are introduced. The action is atomic for the ground-loot inventory and one tree's visual state.
- That executor also implements door object 349 frame 0/1 changes for the original open/close event UIDs. These APIs **do not automatically trigger** without the original ancestor conditions, and do not yet update door shadows/collisions or dispatch sounds.
- `Stage2Audio` has a separate on-demand world-effects voice and verified original WAV paths for `chop(1..4)`, `treefall_02/03` and wood/metal open/close door cues. This is **audio playback support**, not proof that the correct event branches have been hooked into gameplay.
- Regression coverage checks individual state isolation; atomic replay validation; corrupt, stale, and mismatched save files; tree fall item IDs and duplication; and door frame state transitions.

**Verification evidence:** Native system regression tests passed [#44839509693](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44839509693) after fixing generated item/loot dependencies in CI. A prior 3DS build passed [#44839473994](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44839473994); check the newer audio-enabled commit's own build before declaring that build verified.

**Not complete — do not check off the overall world feature list:** root-source tree hit conditions/health/durability and event dispatch; tree falling sound and coordinate RNG in actual gameplay; source House_transparenting per-object opacity, shadow, layer and interior conditions; correct automatic door trigger/audio/collision; interior loot and NPC lifecycle; active category-15 loot markers (current world tests found zero); original save semantics for all dynamic world objects; asset coverage and Azahar/physical 3DS parity.

**Source warning:** The root `House_transparenting` chain [trace run #44839251402](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44839251402) changes object 1049 opacity to 25/100, updates sprite layers and tests overlaps between 181/193/1048/1049/346. It is **not** a simple unconditional 'hide roof near player' condition. The native implementation must preserve these source guards rather than invent proximity distances.

## Task 1 — World interactions live implementation (2026-11-21)

**Scope:** Original trees, doors, roofs, collisions, sound, resource rewards, interior behaviors and persistence. This is the *active* workstream; do not begin Task 2 as a standalone milestone before Task 1 is complete. Root original `data.deobfuscated.js` and original PNG frame imagepoints are authoritative.

| Subsystem | Original source implementation added | Verified in native CI | Azahar/3DS gameplay | Remaining original event-chain work |
|---|---|---|---|---|
| Procedural tree hitboxes | Six `create_map_element` event branches now create linked `obst_base` sprite **345** on `zed_resp`, **frame 10**, exactly at each tree's source `Imagepoint 1`. Tree/obstacle share runtime links. | `world_system_tests.cpp` checks imagepoint transform, links, tree fall and item IDs. World test passed [#44930617833](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44930617833). | Not tested | Live projectile **1031** collision and original tree instance variable 0 initial HP/guards still need source-derived combat dispatch. |
| Falling trees and rewards | Tree hit/fall executor sets tree frame **1**, adds the original item IDs, then sets linked hitbox frame **8**, with rollback on error and duplicate-fall rejection. | Native world tests pass. | Not tested | Original `chop`/treefall audio triggers, exact action RNG order, fall-side collider/player damage and original HP persistence not yet live. |
| Automatic doors | Native player movement now selects source object **349** frame 0/1 using its **per-animation `in` imagepoint**, the original PlayerBase Y comparison, and matching wood/metal open/close WAVs. Door is no longer treated as an unchanging solid rectangle when open. | Original door frame and live automatic-door regression passed [#44929083923](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44929083923). | Not tested | Companion obstacle-base/shadow object **348**, all source picker guards, polygon collision and door timing need end-to-end parity. |
| Roofs and house interiors | Fixed a source misunderstanding: **1041, 1048, 1049 are Construct families, not sprite objects**. Extracted all **42 source families** including **17 interior members** and **48 roof members**. Original member PNG `Imagepoint 1` and **25%/100%** opacity decisions now feed live player movement; roofs excluded from solid walls by actual family membership. | Native world tests printed **30 roofs / 80 interior placements / 30 valid frames** in fixed-seed world [#44930617833](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44930617833). | Not tested | Exact 1048/346 overlap ancestry, layer reordering and all interior transitions still incomplete. |
| Root-source asset and local-build integration | Original imagepoint generators for doors, six tree hitboxes, and roof family members plus family-membership tables run in native CI, 3DS workflow and standalone devkitPro Makefile. Original PNGs remain untouched. | Earlier 3DS build of the live door/tree work passed [#44929664354](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44929664354). | Not tested | Need successful 3DS build of the latest full roof-family integration and Azahar comparison. |

**Regressions fixed while integrating:** A prior attempt treated family 1049 as one sprite and failed at source-frame extraction. That code was removed, then replaced with real project[4] member mapping and member imagepoints. Another CI failure came from the old test expecting the non-Sprite family 1049 to collide; its assertion now checks actual roof members. A native player test needed family table generation before compilation. See latest native/3DS workflows for final confirmation.

**Latest full CI validation of the integrated code:** [Native System Regression Tests #44930751439](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44930751439) **passed** and [Build 3DS Executable #44930751169](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44930751169) **passed** on code commit `e3916e32`. Later tracker-only commits do not change game code. No claim of Azahar or real-hardware confirmation.

**Task 1 is NOT checked complete:** live source projectile hit/HP initialization, linked tree fall audio, source audio probabilities and object state storage, exact door 348 companion logic, full roof/house source conditions, interactive containers/furniture, original collision polygon parity, and in-emulator testing remain. Do not convert successful compilation into a 100%-finished claim.

## Current priorities

**First:** correct source-authored procedural vegetation/tree distribution, actual on-screen visibility, tree frame changes on damage/chopping, resource drops, ground pickup and inventory state, collision, sound and persistence.  
**Then:** source-driven roofs/interiors/doors, road/location generation, and interconnected item loot tables. Complete each end-to-end system rather than simply placing images.

## World object geometry fixes — root original source, 2026-11-20

- The original Construct Sprite hotspot now acts as the **rotation pivot** in `terrain_source_collision.hpp`; the previous inverse transform accidentally rotated collision around the sprite's top-left, displacing rotated walls and doorway hit tests. Tests in `terrain_collision_tests.cpp` and `player_system_tests.cpp` cover centered and asymmetric hotspots, rotated 90 degrees.
- Source `House_transparenting` event UID `5249390718058362` sets roof object `1049` opacity to **25** or **100** and changes visual layers. The opacity-only roof overlay now does not block movement as an entire house-sized collision rectangle. This is a **collision guard**, **not yet the full opacity-event evaluator**.
- `extract_construct.py` now extracts the initial Construct frame `origin_x/origin_y` along with object dimensions into `ObjectDef`. Procedural `create_map_element` placements and root-location create actions inherit these source values instead of `0x0` object sizes and zero origins. World tests confirm real source dimensions and hotspots on generated trees. **Exact later Set Size / Set ImagePoint / animation-dependent collider polygons remain unimplemented.**
- **Verification policy:** tests and a successful 3DS build are necessary, not sufficient. All house/tree/collision boxes remain open until correct original geometry, source event conditions, state transitions, sound, and Azahar/real console tests pass.

## Active cross-system progress board — keep moving through the whole game

This is a **work queue**, not a claim that any listed system is 100% complete. Track multiple workstreams instead of stopping all development for a single unfinished interaction. A failed or blocked feature stays listed, with its original source reference, while work on independent features continues.

| Workstream | Verified foundation | Next implementable work | Full-game gate |
|---|---|---|---|
| World terrain and procedural generation | Root-original extractors; fixed-seed generated vegetation tests | Validate roads, bridges, towns, bunkers, and map density against source branches | Original-generation parity and Azahar comparison |
| Buildings, roofs, doors, interactions | Source sprite geometry, frame/effect helpers | Dispatch house opacity/roof reveal, door states, interior collisions and original sounds | In-game entrance/exit and interactive-object tests |
| Vegetation and resource gathering | Six source-UID-gated original tree-hit/fall executors pass native tests | Wire player projectile/melee collision and initial tree HP, fall/drop/audio and persistence | Actual chopping with rewards and original timing |
| Items, inventory, survival and crafting | Item/localization/source graphics indexed; some native gameplay modules already present | Audit item-ID ↔ sprite ↔ spawn/recipe chains and fill missing inventory/loot/survival events | Live pickup, use, craft, survival effects and saved inventory |
| Location loot and world respawns | Original event traces and marker category handling | Fix root location creation-to-spawn chain; 0 eligible markers in tested new worlds | In-game loot and correct category/timer probabilities |
| Combat, enemies and NPCs | Native combat/NPC foundations and original NPC event extraction | Complete original contact damage, AI, spawns, firearms/reloads and enemy drops | Full live encounter tests |
| Vehicles and world objects | Original source files and mechanics indexed | Source-check vehicles/traps/structures and implement missing control/state transitions | Actual vehicle/structure interactions and saving |
| UI, maps, sound, quests and progression | Main menu/render/audio foundations already built | Connect HUD, bottom-screen inventory/map, world cues, quests and achievements | Original visual/audio and control parity |
| Saving, multiplayer and release | Some per-instance visual-state replay and save infrastructure exists | Save dynamic destruction, loot, doors, NPC/vehicle state; then multiplayer sync | Reload, synchronization, emulator and physical 3DS |
| Asset completeness and performance | Original source frame/PNG audio audit tools | Classify remaining 119 unreferenced PNGs and validate full T3X/WAV coverage; profile hardware | 100% source-used assets reachable and verified |

**Workflow rule:** whenever one workstream is blocked, document the precise blocker and continue another independent workstream. For every implementation change, append test results and update its row here. Do not falsely convert broad gates to `[x]` merely because a component passes CI. Priority is world/interaction parity while also continuing item, combat, audio, saving and UI integration where source evidence permits.

## Current completion ledger — narrow milestones versus full gameplay

**How to read the unchecked boxes below:** They are *end-to-end completion gates*, not "nothing implemented." The `[x]` items in this ledger are smaller tasks with direct source, code and CI evidence. A large category remains `[ ]` until its *entire original event chain* (including visual, audio, physics, rewards, state, and Azahar verification) passes. Do not check all of a section merely because its core code compiles.

### Verified implementation milestones

- [x] Root original Construct data structurally compared and indexed; stale snapshot identified and excluded from the world-data build path. [Source audit](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44833659709).
- [x] Original source sprite-sheet frame extraction, frame selection, origin/hotspot and world placement geometry **implemented and exercised by native tests**, including procedural tree and house size fixes. [Latest native tests](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44841936380).
- [x] Fixed-seed, root-source procedural tree spawning regression tests passed (levels 0–5 yielded 1,676 / 1,715 / 1,641 / 1,591 / 4,117 / 4,327 generated placements); not an Azahar density-parity claim. [Native verification](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44837516090).
- [x] Correct original hotspot used as rotation pivot in sprite collision geometry; regression cases include asymmetric and rotated shapes. [Latest native tests](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44841936380).
- [x] Standalone original-source door-frame/tree-fall effect helpers, per-instance visual-state snapshot/replay and original world-effect WAV playback support added with regression tests; gameplay dispatch and full event semantics are **not** proven. [Native tests](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44839509693).
- [x] Latest `main` native regression workflow and 3DS build completed successfully at commit `e57e37b6`. [Native tests](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44841936380) · [3DS build](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44841936432).

### Original six-species tree hit → fall transition (2026-11-20)

- [x] Extracted and asserted all six original tree hit UIDs, their fall child UIDs, source Sprite subtract-one action (command 87 on instance variable 0), and fall condition (command 71). [Root original trace CI](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44844077733); source script `tools/stage1/audit_tree_hit_source.py`, exact source records in uploaded artifact.
- [x] Added source-UID-gated `apply_verified_tree_hit` to `include/minidayz/source_world_event_effects.hpp` and six hit/fall pairs to `include/minidayz/source_tree_fall_rules.hpp`. The source-authored hit subtracts 1 from the **caller's original per-tree health variable 0**, and a nonpositive resulting value atomically invokes existing original frame-1/drop-ID fall behavior. Wrong IDs, duplicate fall and invalid RNG-derived drop coordinates are rejected without consuming HP or creating partial loot. `tests/world_system_tests.cpp` exercises both ordinary hits and fall, state isolation, incorrect event UID and rollback.
- [ ] **Not wired to live 3DS attacks yet:** the ancestor bullet/sprite collision and hit-timing conditions, initial per-tree HP from source instances, projectile contact, hit/chop/fall audio triggers, source RNG draw order, source object/HP persistence and tutorial progression. This is a tested original-event **executor**, not an assertion that the player can chop trees in the running game. Keep the broad "Trees, bushes, vegetation and harvesting" checkbox open.

### Open end-to-end verification gates

- [ ] Confirm biome/road/bridge/building generation reproduces the original event probabilities and map placement, including Azahar visual testing.
- [ ] Connect real player chopping/harvesting to exact tree hit, sound, fall frame, original item drops and collision/removal events.
- [ ] Connect actual house entry/exit, roof opacity/layer, furniture, automatic doors, sound and changing collider behavior to the full Construct event hierarchy.
- [ ] Demonstrate original location-specific loot marker creation and item spawning in a new world; existing tested seeds showed zero eligible category-15 markers.
- [ ] Verify all dynamic world state persists (not only sprite frame/opacity), including destruction, container contents, collisions and relevant respawn timers.
- [ ] Complete per-PNG/frame reachability accounting and emulator/physical-console verification.

**Bottom checklist status:** The major World, Items, Combat, Game Systems, Audit Dimensions and Graphics Coverage headings below remain unchecked *intentionally*. Their sub-features are partially coded, not yet proven fully complete. These are the gates we will check individually as we finish them, recording the commit and actual gameplay evidence.

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
| 2026-11-20 | Original six-species tree hit-to-fall executor (partial) | Root `Game_events > Hitting_obstacles`: hit 7227858680569601 / 8754965976446553 / 5386206472407495 / 9193333425135665 / 4761103342332918 / 3531921662641626; six fall child UIDs | `source_tree_fall_rules.hpp`, `source_world_event_effects.hpp`, `tests/world_system_tests.cpp`; commits `74364ea7`, `f9135cd3`, `8cecd8b8` | [Full native regression passed #44844077738](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44844077738); [root original tree hit-chain CI passed #44844077733](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44844077733) | Not tested | Not tested | Test-only supplied original variable-0 HP: decrements exactly 1; zero-or-less frame-1 fall with original drops; duplicate fall and failed RNG rollback. Live bullet hit conditions, original initial HP, audio, full persistence and gameplay still unimplemented; do not check overall trees box. |
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
