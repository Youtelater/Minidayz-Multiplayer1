## Part 1 — building generator maintenance

- [x] Removed the roadside shelter feature added in `e3bc5f6`. An existing save from that revision can still contain its saved ground items.
- [x] Retained the event walker and variant guard fixes. The loot placement regression pairs markers in generated order when an action ID is reused.
- [x] Fixed companion placement for missing numeric attachment points. It uses the parent sprite hotspot, with authored frame points still used where present. Native world regression covers the variant 36 collider that uses this fallback.
- [x] Bridge and bunker loaders validate replacement placements before changing the world. Native regression checks record counts, reloads, alternate bunker coordinates, and preservation of existing features if a replacement is out of bounds.
- [x] Checked point-of-interest quotas for eight seeds across all six map levels. The six full-generation cases check road presence and each bridge and bunker placement against the configured coordinates, with bunker slot weights checked as well. Native world tests passed.
- [x] Bridge travel builds the destination before replacing the active map. A generation failure keeps the current map, survivor, loot, tents and stashes available. The bridge prompt offers a retry.
- [x] Updated the spawn regression check for the travel function signature and checked that spawn uses the destination seed and level. The local fidelity and screen-depth checks passed.
- [x] Native road tests check horizontal and vertical segments, intersections, and preservation of existing location cells across eight seeds on each of the six levels. The full native world suite passed.
- [x] The native suite and 3DS executable build passed on `9c4279b`. Azahar verified New Game, Continue, all five forward bridge transitions, and the final-level travel limit using separate save fixtures. The transitions retained survivor health, and Continue reopened a save written after travel.
- [x] Native travel regression forces a destination-generation failure, checks that the active world is preserved, and retries successfully. The staged loader also handles allocation failure while generating the destination.
- [x] The 3DS build passed on `f0e2d2a`. Azahar retested the first bridge crossing on that executable. A native fault-injection check also verified retry after allocation failure during setup and generation.
- [ ] Verify failure and retry in the emulator and measure peak memory on real 3DS hardware.
- [x] Location Array 525 row 5 now reaches native element placement as Mapgen var3. Existing category 4 variant 42 placement (event 593462046150106) applies its original var3 equals zero guard. Native regression verifies values 0, 1 and 0.5, and the world suite passed.
- [ ] Conditional building placements, initial building loot dispatch and full map-generation parity remain open.

## Part 2 — per-house roof and wall ownership correction

- [x] Live roof updates no longer share one global wall-overlap flag across unrelated houses. The picked original 346 wall triggers retain their owning foundation runtime UID, so the roof decision, linked door opacity and indoor rain are restricted to the relevant house. Implementation commit `e6f74de5`.
- [ ] Verify adjacent-house transitions, unlinked authored wall instances, exact original picker semantics, tint/outline and full 3DS build. This correction is not full Construct event parity.
- [ ] Part 1 generation reconciliation remains open.

## Part 2 — manual door input integration

- [x] Root original source event 5295137988838652 stops AltMove when PlayerBase reaches movement marker506; child 742251994234684 selects marker type 2 and 9527050336890525 requires player collider181 overlap with door349. Source open/close UIDs 7258332154939593 / 353730214514351 use Wait(0.01), linked Solid348 removal/recreation and original sound variants.
- [x] Native 3DS A input with no selected inventory slot now selects the nearest overlapping source door and queues the manual event family using its runtime UID, source frame, original delay and linked collision state. Automatic events share the wait/transition guard. Implementation commit `d753309f`.
- [ ] Native A input adapts mobile marker arrival; actual marker506 pathfinding, animated marker lifecycle and exact original overlapping audio branches remain open. Verify full build and manual/automatic interaction in emulator. Do not mark complete.
- [ ] Remaining Part 1 generation reconciliation and Part 2 roof picker/tint/outline behavior remain open.

## Part 2 — deployed tent source sprites and collision checkpoint

- [x] Live deployed tent rendering uses original Sprite398 foundation and Sprite397 roof, animation 0/frame 0, using frame-indexed atlas rendering rather than item456's inventory icon (commit `0e99b1ee`; deployment UIDs 297398672474814 / 1391504325547996).
- [x] Roof397 overlap uses the original 49×65 frame with hotspot (25,33) and authored rectangle x=-25..12/y=-33..32, with opacity 25% on overlap and 100% outside (commit `3c99a618`; root UIDs 3450973739819458 / 8566394378489923).
- [x] Tent foundation398's source concave wall is consulted in live movement collision (commit `1a769643`); interior and entrance remain open. The original roof is not a Solid.
- [ ] Verify these changes in native tests, latest 3DS build and actual emulator. Exact original tent deployment restrictions and storage limits remain separate unverified behaviors.
- [ ] Finish manual door prompt/selector, roof picking and tint/outline depth effects; reconcile remaining Part 1 generation categories.

## Part 1 — original building placement subtype correction

- [x] Re-read root `data.deobfuscated.js` using the blob API. Category 7 has 51 direct primary variant branches; category 4 variant 40 contains seven nested var6=0..6 branches. The existing native table already selects the correct primary Sprite1345–1351, but the companion extractor discarded these branches because it only recognized var2 in the local condition. Fixed the extractor to inherit variant 40 under original event 431776969482924 and preserve the exact event UID. Each original structure now has its source Solid348 frame 33–39 at numeric Imagepoint1, with its original parent UID variable. Commit `0781e1ba`.
- [x] Added native regression for all seven subtypes, including collider frame and ownership, commit `cca0dfd2`.
- [ ] Confirm current CI and remaining original building placement categories; no claim of complete emulator parity.

## 2026-11-21 — Building gameplay implementation continuation

- [x] Extract original per-frame building interaction collision masks, including non-Solid roof and door frames; use hotspot-relative concave polygons rather than treating the entire PNG as a rectangle. Player collision object 181 has the original 30x30 default rectangle. Correct zero-length polygon edge false positives. Six isolated C++17 polygon overlap cases compiled and passed in the local test harness.
- [x] Wire original roof Imagepoint 1 (opacity) and Imagepoint 2 (drawing depth) to live C++ roof updates, the separate 0.1s/0.5s source timers, foundation animation/layer changes, wall-zone checks and linked door opacity. Preserve Solid behavior across drawing-layer changes. This is not a proof of exact picking/tint/outline parity.
- [x] Implement original Construct Sprite frame-change width/height ratios and hotspot updates (root c2runtime.js r.Zi) in world state transitions and snapshot restoration. Preserve the door's closed-frame companion anchor and do not accidentally recreate it on an opacity-only update during the original 0.01s wait.
- [x] Connect original indoor rain sample rainroof.wav to house shelter transitions and restore outdoor rain. Native implementation switches one weather channel rather than reproducing concurrent Construct audio tags.
- [x] Recover the separate original building loot branch 4949969985883762 from root data.deobfuscated.js: 49 action-bearing branches, 153 object-218 marker spawns, 151 immediate Trigger_spawn calls and two markers without the call. Source-authored category, imagepoint, case-insensitive layer resolution, special variant-1 Y offset, exact parent link and trigger flag are now represented in world generation. Native regression checks all 153 source transforms. Extraction independently checked against the root event blob; all referenced parent/variant pairs are present in the original primary-house spawn branch.
- [ ] Run latest full native/3DS CI and test Azahar/real hardware. Current connector does not expose main push workflow discovery, so the latest build is **not verified**.
- [ ] Implement and verify actual initial Trigger_spawn item generation for all building loot categories, including original conditions/weighted choices, capacity, persistence and container gameplay. Marker placement alone is not item spawning.
- [ ] Finish exact original roof picker, Tint and player outline effects, standalone roof 397 behavior, movement-marker door route and overlapping audio cue branches. Verify old saves after adding generated placements (runtime IDs are generation-order dependent).

Implementation commits include b536b265, 3a57c8cf, fee2235e, f90d68e, 2e46c700, bd4c0d57, 9b51733a. Overall Buildings/Roofs/Interiors/Doors/Furniture remains unchecked until integrated gameplay verification and remaining behavior pass.

# MiniDAYZ 3DS — Implementation Verification Tracker

**Branch:** `main`  
**Created:** 2026-11-20  
**State:** Source indexing and several narrow implementation milestones verified; all broad gameplay/release completion gates remain open pending original-game parity and Azahar testing.

This is the persistent checklist for the MiniDAYZ / MiniDAYZ+ Nintendo 3DS port. Update it as development proceeds. It is not a list of features already implemented, nor a claim that the original game contains every listed mechanic. Confirm feature presence and exact behavior in the original source first.

### Rotated building overlap correction

- [x] Native roof/interior/wall-trigger rectangle overlap now tests all four separating axes, including world-aligned player axes, avoiding false contact at rotated corners. Implementation commit 520f002; mirrored and unmirrored regression cases commit b47c8b5.
- [ ] Verify native/3DS CI for this correction. This is rectangle overlap accuracy, not proof of original Construct polygon picking or the remaining furniture, loot, prompt and emulator gates.

### Full original house source-branch coverage and missing numeric imagepoint fix

- [x] Expanded native world test to execute **every one of the 69 original literal primary house Spawn records** rather than sampling five city-house designs or only the 48 legacy cached rules. For each original parent, the test checks the actual source child object, source animation and frame, including descendants. This found a genuine missing house-generation collision Sprite: var2=36/base1323 originally spawns Solid348 frame30 at numeric Imagepoint 1 on zed_resp.
- [x] Root original var2=36 event UID 304859375570401 and image frame records inspected: parent1323 `b_gudui-sheet0.png` frame rectangle 57×41 has **no custom imagepoints**; source nevertheless calls Spawn(348, Imagepoint1). Source numeric missing-imagepoint behavior resolves to parent hotspot. The source house imagepoint generator now uses that default only for actual authored numeric point-1 actions with no custom points. It never invents texture crops or named imagepoints, and preserves the original source object348 frame30 with its atlas rectangle and collision polygon.
- [x] Added a separate var2=36 regression asserting Sprite1323 and its object348 frame30 exist at the exact authoring hotspot. Source audit [#44945083415](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44945083415) passed and confirms the original object/imagepoint fallback geometry was generated.
- [x] Full revised native world [#44945121183](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44945121183) completed successfully, including the exhaustive **69 primary house source records** and missing original 1323→348 imagepoint fallback regression. Original-game-versus-Azahar rendering/handling remains unverified.
- [x] [3DS build #44945083431](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44945083431) completed successfully for original Sprite1323 numeric point-1 hotspot fallback and native 69-house branch implementation. [Native regression #44945121183](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44945121183) subsequently passed with the separate Sprite348 frame30 position assertion. The revised full [3DS build #44945121231](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44945121231) also completed successfully, including the explicit original Sprite1323→348 frame30 position test.
- [ ] Other house-building milestone gates remain open: full original conditional object interaction groups, furniture/container contents and loot behavior, prompt-object506 effects, weather/fire effects on houses, and emulator/physical 3DS evidence.

### Exact original door instance variable zero, additional source event checks

- [x] Root door close UID 243854400993751 explicitly performs 349.SetInstanceVariable(0,348.UID) after creating its Solid. The native linked 348 companion now writes its own stable source UID into the correct door variable on initial closed-spawn and every close transition; opening leaves the former UID recorded as the original source Destroy branch does. Source variable 1 remains the original Spawn parent's UID. Changes in source/world.cpp and tests/world_system_tests.cpp commits 58083ac2 and 1b0be8ef.
- [x] Root original alternative door frame events 7258332154939593 / 353730214514351 are now supported in the guarded source door executor, including separate linked 348 activation and the original frame IDs. Normal automatic events remain unchanged. Native regression covers both source event UID pairs. Exact prompt-object 506 lifecycle is **not** yet fully dispatched by the 3DS controls.
- [ ] Most recent 3DS and native full-build results still need their own confirmation; the last observed fully passed 3DS revision was e297f7e6, with new companion UID and prompt event changes added afterward.
- [ ] Root original extra conditional house-primary branches are being distinguished from direct unguarded Mapgen.var2 conditions. Do not synthesize those primary houses based on one matching source variable while other original conditions remain unevaluated.

### Additional original house event-family coverage and verified build

- [x] [Original building runtime event trace #44944170187](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44944170187) found **25** original non-creation event sites directly referencing known house/roof/wall/door objects and source families. This includes TWO original door open/close event families, not just the automatic one.
- [x] Original UI-prompt door UIDs **7258332154939593** (open) and **353730214514351** (close) now select their original door animation frames in the native source event executor and update the linked 348 Solid, just as automatic UIDs **6061166023614368** and **243854400993751** do. World regressions cover both families and reject repeated transitions. The alternate source events also destroy object 506, an associated interaction prompt; that particular UI side effect is NOT yet a full native prompt lifecycle.
- [x] [Native system regression #44943895190](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44943895190) and [3DS build #44943895366](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44943895366) succeeded at revision e297f7e6, covering source house companion slot parsing, original UID linking, source-transformed building sprites and their native regression tests. Later new source trace/alternate door executor commits require own CI results.
- [ ] Remains unverified: native gameplay selector for prompt-driven original door events including object 506, other original building/fire/perk/looting conditions, all container/furniture gameplay and source 218 category-driven item creation, and Azahar hardware visual parity. Do not mark broad houses/buildings complete based on passing the build alone.

### Original house child instance variables and category-marked world objects

- [x] Root-original house post-Spawn action trace identified **458 authored actions**, including 168 object-346 wall instance-variable assignments, 48 shadow-347 UID assignments, 27 door-349 UID assignments and source object-218 loot category assignments, as well as original SetSize/SetHeight/SetAnimation actions. [Original trace #44943694658](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44943694658).
- [x] Source-generated house companion table now captures original same-event Sprite SetInstanceVariable (command 88) values in slots 0/1, both authored literals and actual UID-of-a-selected-source-parent expressions (expression 151). WorldPlacement preserves those per-instance source variables; source/world.cpp resolves the correct parent runtime identity in the same ordered Spawn chain instead of assigning unrelated nearby object IDs.
- [x] Original generated object-218 house loot markers now retain their exact source category (including source categories 9 and 19) as source_loot_category rather than losing the value. This is **metadata preservation, not a claim that category 9/19 timed loot logic is implemented**; original_timed_loot.hpp is source-guarded on category 15.
- [x] Native world regression asserts each mapped source companion's literal slot assignments and UID-linked object's actual source type, and that a generated loot marker category matches its original instance variable. Fixed the empty generated C++ array by decoding Construct numeric slot parameters. Latest code commit e297f7e6.
- [ ] Latest native/3DS CI completion for this revision must be verified; gameplay audio, actual container interactions, remaining conditional source event chains and Azahar parity remain open.

### Continuing unfinished houses and buildings — original post-Spawn transforms

- [x] Root `create_map_element` house Spawn extractor now carries original literal same-event **SetAngle (84), SetOpacity (163), and MoveToLayer (137)** changes on each particular newly spawned building sprite, without inventing values for unsupported expression-based actions. The runtime applies these to the actual C++ placement, including source frame imagepoint positioning, collision and draw layer. Explicit angle 0 is distinguished from the absence of an original SetAngle.
- [x] Added native exhaustive house-branch regression assertions for the authored per-child opacity and explicit angles; the native/3DS test workflows regenerate the source tables. Changes: `tools/stage1/build_source_house_companions.py`, `source/world.cpp`, `tests/world_system_tests.cpp`, commits `8fc325ba`–`cee6acce`.
- [ ] Verify native [CI #44943559378](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44943559378) and [3DS build #44943559287](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44943559287) for this revision; do not mark full gameplay parity without their results.
- [ ] Houses/buildings remains **not complete**: source-authored container/furniture interactions and loot state, unhandled conditional Spawn action paths, exact family 1048/346/1049 original picking and special draw-order cases, all original interaction audio and actions, dynamic object lifetime parity, and original-game-vs-Azahar/physical 3DS visual/gameplay comparison.

### Verified recovery of missing original house bases

- [x] Root-source extractor identified **69 literal source primary house Spawn branch records**, in addition to 213 nested Sprite Spawn actions. World regression [#44941485475](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44941485475) completed successfully and verified **48 mapped primary building branches plus 19 additional uncached original building branches recovered by the new runtime**, all directly backed by the original source var1=7/Mapgen.var2 conditions; all five key exterior/interior pairs and exact numeric Imagepoint3 door placement still pass. No invented source place names or guessed roof coordinates.
- [x] Fixed-seed worlds after this integration contained 64 source roof placements and 140 family-1041 interior placements in the native test; prior map generation with only base sprites produced fewer complete houses. These are observed native counts, not original-game screen or RNG parity.
- [ ] Remaining discrepancy: 69 source primary Spawn records versus 48+19 exercised mapped/restored branches includes conditional/repeated variants whose full source instance selection still requires original condition and runtime-level verification. Confirm all live corresponding Sprite animations, their shadow order, furniture contents and player overlap in Azahar.

### Family 1048 original animation verification and nested source IDs

- [x] Original-root [native CI #44941584258](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44941584258) passed and showed only **one** family1048 member has a source animation literally named `out`; the five city-house bases 302/306/311/312/313 only contain `in`. The source Construct 1048 SetAnimation(out) therefore **must not be simulated by inventing a second atlas frame on those houses**. The 3DS gameplay looks up and selects `out` only on a source member that truly supports it; the check also tests original 346 wall-trigger ownership.
- [x] Added explicit `parent_runtime_uid` and `house_base_runtime_uid` to each nested source building placement. A door spawned by the original roof 321 retains both its immediate roof parent and original foundation 302 UID, separately from its linked Solid 348. Native regression asserts the exact source base→roof→door and door→Solid relationship without matching unrelated nearby sprites.
- [ ] Confirm the latest full 3DS build/native suite after source house UID fields; earlier complete builds cover most source features but not every newest commit.

### Root source primary house recovery and roof family animation event

- [x] The original root CreateMapElement var1=7/508.var2 branches can contain a **primary house base Spawn** not present in the cached 158-rule world table. The source house extractor now records these literal Spawn actions only when the event is a direct, single-condition source branch; `WorldSystem::expand_map_element` safely emits otherwise-missing original base Sprites and their nested roof, door, wall and shadow Spawn actions, using the same original per-frame atlas and imagepoint handler as existing bases. Unclear or multi-condition branches are not fabricated. Native `test_every_source_mapped_house_spawn_chain` also validates restoration of those uncached original primary houses.
- [x] Exact source roof event UID **6159361858903351** attempts family 1048 Sprite.SetAnimation("out") when the player no longer overlaps its own wall-trigger object 346. A root-generated family1048 animation lookup now refuses to invent an animation index when none of the source members have a matching animation. The live handler only applies a source-confirmed out animation to that roof's linked 1048 foundation; it never changes a different house's sprite. An earlier codegen build failure from a missing closing brace was corrected in commit `3a046b01`.
- [ ] Current source-primary fallback and roof animation changes still need successful whole native/3DS workflow runs; do not mark broad house fidelity complete without confirming native branch coverage and visual comparison.

### Verified native house Spawn coverage and 3DS build checkpoints

- [x] Root-original Spawn generator reported **213 literal house companion Spawn actions, 66 source events, 87 parent Sprite kinds, 624 sprite-frame imagepoints**, with no unsupported Spawn action in the extracted set. Native [world regression #44940777597](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44940777597) **passed**, including five roof/door/wall/shadow city house pairs, **48 distinct primary map-rule building branches** and 243 created test Sprites; fixed-seed world generated 64 roof members and 80 family-1041 interior placements, and player automatic doors exercised five variants. These counts are native regression observations, not proof of every original source branch or emulator appearance.
- [x] 3DS CI [#44940503158](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44940503158) **succeeded** with native original five-pair source Spawn implementation; later [#44940595361](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44940595361) succeeded after local source house roof visibility improvements. Newer changes must still pass their own builds before claiming that full revision compiled.
- [ ] Outstanding: root 66 child-event branches versus only 48 covered primary rule variants must be mapped with exact conditions; original 1048/346/1049 roof selection, furniture/containers, special animations, save state for contents, and Azahar rendering remain unverified.

### Full original house Spawn parent identity and linked door ownership

- [x] The original nested house Spawn hierarchy now records `WorldPlacement::parent_runtime_uid` for each generated roof/door/wall/shadow separately from `linked_runtime_uid` used by the original 349-door→348-Solid relationship. Thus a generated door retains its exterior-roof parent even after gaining its own door Solid companion. World tests assert source pairings `foundation→exterior roof→door` and companion links rather than guessing by proximity.
- [x] The original 1048 foundation/1049 roof relationship is now used for the source-generated roof opacity decision. Original 1041 membership was independently confirmed to be a different family; the previous assumption would never fade generated city roofs.
- [ ] Verify latest broad native regression + compiled 3DS run for this revised original-source Spawn hierarchy and test all 66+ original nested building variants. Original source picker, furniture/containers, roof/wall triggers, actions beyond source Spawn and emulator rendering remain incomplete.

### Per-building roof ancestry, source Spawn coverage and 3DS performance

- [x] Corrected a dangerous roof-family assumption while comparing against the original project[4]. Original family **1048** (50 members) contains house foundations 302/306/311/312/313; family **1049** (48 members) contains their exteriors 321/322/323/324/333; family **1041** (17 members) is a DIFFERENT group. Source-spawned roofs now gate opacity on *their own linked 1048 foundation* rather than a global interior flag or matching their parent UID against family 1041 (which could never match). The authored/unlinked roofs retain a conservative fallback. Native tests assert family membership for all five house pairs.
- [x] Added `test_every_source_mapped_house_spawn_chain` to exercise source-extracted nested Sprite creation for **every mapped building branch** found in the old primary map table, checking original child creation is not silently lost. The test does not assert unexecuted external source conditions are satisfied; broader roof/furniture gameplay still requires complete picking.
- [x] Fast-path source `expand_map_element`: only original building category `var1=7` creates/iterates a house companion list; the thousands of generated original vegetation objects no longer allocate such a list per spawned tree. This avoids a preventable Nintendo 3DS heap/performance regression.
- [x] Five original paired house variants, their original imagepoints and child Sprite presence pass native [world regression run #44940503104](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44940503104). The later exhaustive branch-coverage test and 3DS builds require their own run evidence.
- [ ] End-to-end building completion remains blocked by furniture/loot/container original event dispatch, exact 1048/346/1049 picker and roof layer animation conditions, linked-348 Construct allocation/UID timing equivalence, audio, save of object contents, original-graphics emulator comparison, and full latest CI success.

## Original nested house creation, doors and sprites — 2026-11-21

- [x] **Important missing source behavior addressed in real C++ world generation:** original `create_map_element` building bases spawn more Sprites using original Construct command 73. Added `tools/stage1/build_source_house_companions.py` to derive source-specific nested Spawn rules from **root `data.deobfuscated.js`**, including per-child event UID, original var2/parent branch condition, child object ID/layer, source numeric or named imagepoint, SetFrame and SetAnimation. `source/world.cpp::expand_map_element` now creates roof exteriors, doors, wall-trigger object 346, and shadow object 347 at those *original imagepoints*, with source per-frame dimensions/atlas origins rather than only emitting the base artwork.
- [x] Original base/exterior pairs are now covered by live native assertions: base `306→333` (var2=15), `302→321` (27), `311→322` (28), `312→323` (29), `313→324` (30), including selected exterior frame, door at its original `Imagepoint 3`, per-house linked runtime IDs, wall trigger and shadows. Test `test_original_generated_house_companions` fails if any of these are absent or mispositioned.
- [x] Extracted original object-346 SetSize from foundation's `w1→w2` X/Y imagepoint differences, plus source object-347 SetHeight from `s1→s2` imagepoint differences. Those sprites are **1×1 and 76×1 by default**; keeping those defaults visibly/physically breaks original building interactions. Original Sprite SetAnimation('city3' etc.) is applied to the correct named atlas animation for each door.
- [x] Resolved the apparent base312/roof323 missing-child failure against a **direct original Spawn action trace**. Original source event UID for base312 var2=29 is `9738497725914856`, not `9738497725914855`: an earlier UID patch was incorrect and has been reverted on `main`. The actual missing-roof bug was caused by original imagepoint extraction indexing the newly added source variant field as a parent Sprite ID; this was fixed. Runtime child matching uses original base object + var2, with source UID retained as evidence; never alter UIDs to make a test pass.
- [ ] **Latest native/3DS CI and emulator parity still required.** Early house-test builds failed due a generator bug (parent indexing shifted after adding original variant; now corrected in commit `1bff1ce1`). Do not assert passing tests or mark houses complete until current run passes.
- [ ] **Remaining original source-parity gates:** all 68+ original building Spawn branches and specialized wall/shadow/door settings (including source UID variable linking, hidden triggers, original source layer order), every furniture/storage container and building loot interaction, exact original 1048/346/1049 roof-picking events, dynamic object creation/destruction timing, visual/audio source parity and on-device verification.

## Full house milestone continuation: linked door Solid and versioned roof persistence (2026-11-21)

- [x] Generated world now reserves a separate original object 348 placement for each eligible source door 349, linked bidirectionally by runtime UID. Its position comes from the original door frame-0 Imagepoint 1; original object-348 animation 0 frame 15 is rendered with source width, height, hotspot, and Door.Angle+6. The former decorative door rectangle no longer acts as the primary collision when a linked 348 exists.
- [x] Opening a door now changes only its own linked 348 opacity to zero (absent from collision/rendering); closing restores the linked Solid frame at the original imagepoint and angle. Generated placement indices/UIDs stay stable for save files and 3DS broadphase; no arbitrary relocation or shared door collider. This is a fixed native instance with activation/deactivation, not a literal Construct Destroy+Spawn allocation/UID lifecycle. Do not imply the latter.
- [x] Source door links are regenerated from the same seed at world load; the 348 placement state is automatically resynchronized from its source door 349 on world visual restore. Native door tests now assert linked 348 unique identity, frame 15, source angle, open/close visibility and UID isolation.
- [x] Source roof background-layer changes are now part of the version-2 world visual journal, with version-1 compatibility (missing layer keeps source-generated layer), checksum and atomic restore validation. Tests cover opacity/layer save, restore, reset and individual room isolation.
- [ ] Latest 3DS/native CI success for these commits must be confirmed. A previously observed native test compile failure from an unqualified namespace was fixed in commit 2586d0a6. Keep the feature gate open until its own tests pass.
- [ ] Still missing full parity: source family 1048/346 overlap and roof layer reorder ancestry, complete interactive building furnishings/containers and original location loot, polygon/picker equivalence including the native fixed-UID companion approximation, all world audio triggers, Azahar visuals/FPS and physical 3DS verification.

## Building completion work: original roof event, frame geometry and authored instance actions (2026-11-21)

- [x] Root House_transparenting event subtree traced and verified in [house source run #44937770379](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44937770379). The source roof overlap branch UID 9366118161137804 uses Every(0.1), and exit UID 9848716569903957 sets roof family 1049 to opacity 100 and moves it to Buildings_bg. This corrects the earlier guessed 0.5s opacity check. Event branches also involve family 1048 and object 346; complete picking semantics remain open.
- [x] Actual live 3DS code now evaluates source roof opacity on the 0.1s clock and moves only an exited roof's own placement onto the original Buildings_bg layer. WorldSystem::set_placement_layer validates the original Map layer index. Existing native world tests now check move-to-layer affects only the selected roof.
- [x] The native source location interpreter now understands original selected-Sprite SetFrame, SetSize, SetAngle, SetPosition, SetOpacity, MoveToLayer and SetInstanceVariable actions for generated instances, preserving original frame/angle/opacity/layer rather than silently losing those authored building effects. WorldSystem now propagates source-created angles and opacity into live placements.
- [x] The source-building polygon generator also derives the exact original animation/frame pixel size and hotspot for building sprite variants, including non-Solid roofs and house interiors; generated world placements use the selected frame's dimensions and hotspot when no SetSize overrides exist. Native polygon tests validate every generated custom polygon references a valid source frame.
- [x] Original location runtime regression extended: each of the 62 original location branches across the six source levels now checks finite frame/transforms, layer and source alpha for its spawned objects.
- [ ] Verify most recent native and 3DS builds and resolve all errors. These checkbox ticks indicate committed implementations and test additions, not a final original-game-equivalence certificate.
- [ ] Full house/building milestone still OPEN: exact original object-348 dynamic spawned collider UID lifecycle rather than an effective proxy, inherited roof 1048/346 picking and all shadow/draw order side effects, furniture/container interaction chains, original location creation completeness, collision/player-mask parity, world dynamic persistence, source sound coverage, and emulator/real-device gameplay comparison.

## Houses and buildings: additional live source implementation (2026-11-21)

- [x] Source automatic-door timing: source_door_auto.hpp now separates non-mutating ImagepointY("in") event selection from the real frame change. The 3DS game queues each original door event, waits 0.01 seconds as prescribed by original open/close event UIDs 6061166023614368 and 243854400993751, then updates the source frame, effective collider, and matching original wood/metal sound. Queued transitions carry a distinct door runtime UID, and pending waits reset on world generation.
- [x] Original Solid behavior extraction: build_source_family_members.py reads each original Sprite Solid behavior from root object data. Building collision broadphase and narrowphase no longer treat non-Solid decoration and floors as blocking walls. Door 349 retains its special linked Solid-348 behavior.
- [x] Original per-frame building collision polygons: build_source_building_polygons.py derives actual custom normalized vertices from original house/building frame[10]. source_building_polygon_collision.hpp uses the correct animation/frame, original instance dimensions, hotspot, rotation and mirroring in live player collision. Without a custom polygon the source full-frame rectangle remains; the existing object-348 door Solid is treated separately.
- [x] Tests committed: world_system_tests.cpp checks door intention does not mutate the world and rejects wrong UIDs. test_source_building_update_contract.py checks source wait/order/sound flow. source_building_polygon_tests.cpp tests original polygon boundary vertices across rotations and mirrors. Both GitHub CI jobs and the local devkitPro Makefile regenerate source polygon data.
- [ ] Verify all latest CI tests and the 3DS executable build. The above are committed code and test changes, not a final passing-build claim.
- [ ] Houses and buildings still require original object-348 actual Spawn/Destroy/link lifecycle (current code is a source-shaped effective collider), full original roof 1048/1049 and object 346 selection/layer actions, furniture/container interactions, complete original generated placements and interaction sounds, nonvisual persistence, and Azahar/real-device parity.

## Houses and buildings — live collision/door/roof integration — 2026-11-21

- [x] **Gameplay code:** `source/main.cpp` evaluates the existing source-driven automatic door interaction **before and after movement collision**. Previously a closed doorway could stop the player before `Doors_auto` evaluated; successful frame changes still dispatch their original wood/metal open/close WAV exactly once per transition.
- [x] **Actual mirrored object rendering parity correction:** source door overlap and `in` imagepoint, roof/interior overlap and roof imagepoint Y, and solid obstacle collision now reflect the original Construct sprite when `WorldPlacement::scale_x<0`. Authored width/height are already the displayed dimensions and no longer get a second scale multiplier in world interaction broadphase.
- [x] **Regression cases committed:** `tests/source_world_mirrored_collision_tests.cpp` checks flipped/unflipped door, roof, and wall geometry, including 90-degree rotated imagepoint positions; `tests/source_world_proximity_tests.cpp` checks actual sized bounds; `tests/test_source_building_update_contract.py` checks pre/post-movement door update and original sound dispatch contract. All are referenced by `.github/workflows/build-3ds.yml`.
- [x] **Initial mirrored building and door CI compile verified:** [3DS build #44935243607](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44935243607) completed successfully on commit `0b7cc9c` including `source_world_mirrored_collision_tests.cpp`, `source_world_proximity_tests.cpp`, the original gameplay-order source check and compiled `.3dsx`. This run predates the subsequent once-per-tick door guard and world-catalog syntax fixes; verify those separately.
- [ ] **CI results not directly observed yet for latest pushed changes.** Do not infer that tests pass merely because included in the workflow. Validate current Build 3DS Executable run and fix failures.
- [ ] **Gameplay parity still open:** original object-348 door companion behavior, source roof picking (1048/1049), true original collision polygons, all original furniture/container interactions, object placement completeness, Azahar video inspection and hardware performance remain unverified.

### Original closed-door Solid collision: native integration checkpoint

- [x] Native `WorldPreviewScene::blocked` now tests a source-derived **object-348 frame-15 Solid footprint**, positioned at object-349 `Imagepoint 1` and rotated `Door.Angle+6`, rather than treating the entire visible door artwork as a rectangular collider. Open door frame 1 omits this closed-door footprint. Unmapped source door variants retain a safety fallback rather than becoming walk-through gaps.
- [x] `tools/stage1/build_source_door_points.py` now exports both original `in` and `Imagepoint 1` for every door frame, plus original object-348 frame-15 Solid geometry. Frame 15 has **no usable authored custom polygon** in the root export; the extractor uses its own source dimensions and hotspot as the default Construct full-frame rectangle. `terrain_obstacle_index.hpp` includes the additional companion extent so movement broadphase cannot omit its collision footprint.
- [x] New `tests/source_door_solid_tests.cpp` validates source-driven frame-15 corners, rotation, mirroring and open-door no-collider semantics. **Geometry test passed** in [3DS build #44936199429](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44936199429), along with mirrored-building and door-order checks. Overall `.3dsx` compilation and emulator proof are separate gates.
- [ ] This is a **source-shaped collision proxy, not complete Construct object lifetime parity**. Dynamic 348 Spawn/Destroy instance UIDs, 0.01s waits, original 348 visual rendering, source object picking, collision/timing equivalence and save/reload need exact world integration and emulator verification.

### Original door-collider semantic checkpoint (root-source trace)

- [x] [Root-source world audit #44935601526](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44935601526) completed successfully. It extracted original object **348 = `t348`**: a **42-frame sprite with Construct Solid behavior**; it is **not exclusively a door shadow**. Other original structures, tree companions and maps also create/use different frames of object 348.
- [x] Original automatic door open source event **6061166023614368** contains object-348 **Destroy (command 53)**, original **0.01s Wait (System 54)** and sets object-349 frame **1**. Original close event **243854400993751** waits **0.01s**, sets door frame **0**, spawns original object **348** at door **Imagepoint 1** on layer `Buildings`, links the instance UID, sets companion frame **15**, and sets the companion angle using original source expression **Door.Angle+6**. This is source evidence, not yet an implemented gameplay claim.
- [ ] Complete dynamic original 348 create/destroy and per-door instance association, source frame-15 collider polygon and `Imagepoint 1` placement, wait timing, save/reload, and collision index state. The current native code remains an **approximation** for the closed 349 door; it must not be marked complete until these are validated against runtime source.

## World building interaction gameplay correction — 2026-11-21

- [x] **Actual 3DS gameplay C++ changed:** `source/main.cpp` now uses source sprite dimensions and scale for the broadphase of generated houses/interiors, roof opacity changes and automatic doors. Previous guessed limits `96px`, `256px`, `512px` could silently skip large/off-center original sprites even when the source narrow-phase would accept them.
- [x] New `include/minidayz/source_world_proximity.hpp` uses a conservative full-sprite-diagonal bound plus the original 30×30 player collider's half diagonal. The actual original door imagepoint, wood/metal event, roof family, opacity logic and associated audio still control what occurs; this change **does not invent new sounds or animations**.
- [x] Regression test `tests/source_world_proximity_tests.cpp` added to `.github/workflows/build-3ds.yml`, covering wide doors, large interiors, rotated/off-center sprites, mirrored sprites, far-object rejection, returning a faded roof to 100%, NaNs and invalid sizes. The same C++ geometry cases compiled and passed with a local C++17 host compiler.
- [ ] Confirm full CI 3DS build and generated world checks after this commit and test in Azahar/real hardware; local geometry unit tests alone are **not** source-gameplay parity.
- [ ] Complete source object 348 door companion, original Construct polygon collision, and full original 1048/1049 roof picking/layer semantics. Original world building visuals and audio remain a multi-step task.

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

## Part 2 collision extraction follow-up — 2026-11-21

- Commit `883fcfa32e4df3e43a2b29578d467f8175036b5f` expands the original frame polygon and geometry extraction to every object identified with the source Solid behavior, including procedural building spawns absent from static Map instances. No synthetic vertices added.
- Native CI and 3DS build verification pending; emulator parity remains open. Do not mark building collisions complete yet.

- Commit `40a4f9826bb4077f2d2039ee48cb8fbc991b4b0c` verifies all extracted polygon frames using their original frame dimensions and hotspots, with rotation and mirroring. The existing push-triggered 3DS workflow runs this regression before compilation. CI result not yet confirmed; Azahar and physical-hardware tests remain open.

## Part 2 Step 7 — interior visibility follow-up

- User reported the preceding collision CI workflows passed; emulator verification remains open independently.
- Read the authoritative root `data.deobfuscated.js` blob `bd6fc44626d2f4ede48f9a6b83dedbb23ce06a9d`, `House_transparenting` event 5249390718058362. Commit `119f33df157705657b9114148873f4f0c30f9ff1` implements spatial foundation picking (6583602733022543) and reciprocal roof selection before foundation layer changes (4153213040695326 / 7058744064425045), instead of assuming the generated parent identity is the source picker. Commit `495586a61828e8f2aca2ab55c81a2d44fc57ff` adds a regression contract for this dispatch. No changes to original PNG coordinates.
- CI not yet independently confirmed. Full wall-zone sprite overlap picking, Tint/Outline effects and emulator verification remain open; do not mark Step 7 complete.

- Step 7: original root t193 PlayerBase frame[10] contains 16 vertices and t346 warmzone has an empty custom mask (rectangular frame). Commits `d6c44e4c`, `2b0482d8`, `3b3228fc` implement exact PlayerBase-mask versus transformed wall rectangle overlap, connect it to live roof events and add corner/rotation regression tests. Original event 1166878656779541. CI pending independent confirmation; foundation/door spatial overlap picking and Tint/Outline rendering still open. Do not mark Step 7 complete.

### Part 2 / Step 7 — source event and effect implementation

- Original root `House_transparenting` UID `5249390718058362`: transformed active-frame 1048/346, 349/1049 and 349/346 sprite-mask overlap, including rotated/mirrored masks (events 8794469330826744, 6437471853562815, 7764770537466591): `ab2bbaa5`, `90f0463f`, regression `3eb5eb7a`. Removed synthetic parent-UID door/foundation matching in these branches.
- Source Tint effect `7294047492931295` / `8381343549210222`, source 1048 initial parameters RGB=(0.5,0.5,0.5), and player 181 variable 42 / family 1064 Outline events `995854697906168` / `642126689677118`: `2481de16`, `07ceab57`, `cc68d257`. Native renderer multiplies original foundation RGB; only actual source 1064 skin frames use the derived outline texture when enabled. Original `c2runtime.js` eight-neighbour shader, white 1px width, point sampling and premultiplied-alpha formula translated to Tex3DS frame pages (`93ede86a`, `03fa60d4`). Assets/gfx PNGs unchanged, exact original frame coordinates, index and hotspot retained. Added regression checks for shader pixels, source parameters, generated effect page existence, and live runtime dispatch.
- Object 397 tentative roof visibility: existing live tent renderer already evaluates original 49x65 frame mask and opacity 25%/100% (events 3450973739819458 / 8566394378489923). No duplicate tent rendering added.
- CI and actual Azahar/3DS gameplay verification mandatory; implementation not marked 100% verified until evidence is available. Step 8 remains blocked on verification.
