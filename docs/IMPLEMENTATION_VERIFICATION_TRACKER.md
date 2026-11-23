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

### Step 7 — confirmed automated verification (2026-11-21, EST)

- Exact main commit `7bb4eeb04557d8957d8ec80293f36828f3835c65`: [Native System Regression Tests #44977686221](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44977686221) and [Build 3DS Executable #44977686289](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44977686289) both completed successfully. Verified job conclusions for world-tests, player-tests and build, including house polygons, interior/door bounds, roof update order, atlas mappings, compilation and .3dsx upload. [Graphics audit #44977583412](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/44977583412) also passed on the preceding effect implementation commit.
- Step 7 source-backed implementation and automated verification: PASS; preceding Step 6 collision tests also pass on this revision. Actual Azahar and physical 3DS visual/interaction parity remain untested; do not mark those verification boxes complete. Step 8 may proceed only after the required gameplay verification gate is satisfied (or explicitly deferred to final integration by the project owner).

### Part 2 / Step 8 — original Map drawing order and runtime layer transitions (2026-11-22)

- Authoritative source: root `data.deobfuscated.js` extracted into original Map layout 2 `stage1::kLayerTable`; 1,250 original Map placements in `data/stage1/instances.csv` have zero explicit Z offsets, so within-layer source instance order matters. Map layer indices: `Buildings_shadows=9`, `Buildings_base=17`, `Buildings_base_top=20`, `Buildings_bg=31`, `player=32`, `Buildings=41`, `Trees=43`.
- Native changes on `main`: `include/minidayz/source_world_layer_depth.hpp` and `source/main.cpp` stable-sort camera-visible placements by their **current** Construct source layer, preserving original insertion order within a layer. Source layer intervals are disjoint in the verified Citro2D greater-Z-is-foreground convention; same-layer instances receive strictly ascending fractional depths. The original `House_transparenting` 31/41 and 17/20 transitions now directly affect draw ordering without requiring a new visibility-index rebuild. No original PNG, imagepoint or gameplay mechanics are changed.
- Automated regressions: `tests/terrain_collision_tests.cpp` asserts the extracted Map layer identities, foundation/shadow/roof/player relative depths, original stable same-layer order, and a simulated roof layer transition; `tests/test_3ds_screen_depth.py` guards live sort/depth dispatch. The first compilation failed on a missing source layer header, corrected in `d7a0fe5aeba9cbd7d90f6b6903c25a85b1c869af`.
- **PASS** on code commit `d7a0fe5aeba9cbd7d90f6b6903c25a85b1c869af`: [Native System Regression Tests #45048485101](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45048485101) and [Build 3DS Executable #45048485212](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45048485212), including original layer regression compilation and `.3dsx` output. Both workflow conclusions were independently checked.
- Step 8 **implementation and automated verification pass**. Original-game visual parity on Azahar and physical Nintendo 3DS remain **not verified**; shadows, furniture, containers, loot, persistence and final Part 2 certification are still outstanding. Do not mark Part 2 100% complete based on these automated checks.


### Part 2 / Step 9 — original building shadow depth compatibility (2026-11-22)

- [x] Audited original root Construct Map layer indices and authored Sprite347 shadow placement against the renderer. `Buildings_shadows` is layer **9**, Tilemap211 on `Buildings_base` is layer **17**, and Tilemap208 on `Water` is layer **19**. The native `world.cpp` companion extractor already emits source Sprite347 with source imagepoint, original opacity, size and source-layer identity; do not replace it with guessed rectangles or edit original PNGs.
- [x] Corrected `source/engine.cpp` to use the same source-layer depth function for Tilemap208/211/418/230 as for original Sprite347/other world instances. Previous `0.12+0.008*map.layer` used an incompatible depth scale that incorrectly drew layer-17 Tilemap211 behind layer-9 shadows. Same-layer tilemap cells use the base of their original layer, behind subsequently created source sprites in that layer.
- [x] Added regression assertions for shadow9 < tilemap17 < sprite17 < water19, exact source object347 frame/opacity in five original house spawn chains, and a Python guard preventing return of the independent tilemap depth scale.
- [x] **Automated verification PASS** for code commit `066fc8bd62c9116308fdfc562fdf77e974e81f5e`: [Native System Regression Tests #45051311143](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45051311143) (world-tests and player-tests success), [Build 3DS Executable #45051311113](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45051311113) (compiled and uploaded `.3dsx` successfully), and [Original Graphics Audit #45051311162](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45051311162) (success). Step 9 depth reconciliation is complete at the source-code and automated-test level.
- [ ] **Emulator / hardware gate:** compare original Sprite347 shadows in Azahar and on a physical Nintendo 3DS, including overlaps with layer-17 Tilemap211, water layer19, opacity and named imagepoint-derived stretch. Do not claim emulator parity or complete Part 2 until visually checked.


### World Part 2 / Step 10 — source furniture and fixture implementation (2026-11-22)

- [x] Root original Construct object951 (`bed-sheet0.png`, frames0/1), source bed Build-mode event 125247805541322; original object442 (`waterpump-sheet0.png`), map generation UID 8279941300293499. Actual furniture instances are on Map layer18. Rain layer44 bed is a source editor template, not active furniture.
- [x] Source pump rules for Mapgen.var3=0 initialize water random(100,200); original events 7034863253908448 / 2626558775459672, 2-second wait and 25-unit thirst replenishment with depleted stock. Native state validates water0..200.
- [x] World visual sidecar v3 persists pump variable0 alongside original instance frame/opacity/layer; v1/v2 read remains supported; original PNG atlas unchanged. Native fixture tests and 3DS workflow test step added.
- [x] Source 3DS A control dispatch, pump wait timer and source-bed atlas frame selection implemented on main commit `d067b5c40df17247f4da847c4b687721049f0422`. The source pump native waits exactly 2 seconds; it is not an immediate refill.
- [x] **Automated verification passed on the implementation commit:** [Native System Regression Tests #45053654405](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45053654405), [Build 3DS Executable #45053654425](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45053654425) including `.3dsx` upload, and [Original Graphics Audit #45053654415](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45053654415).
- [ ] Complete source Building-mode bed crafting/placement, original SleepApproved/GotoSleep hierarchy, deconstruction/material returns, and Azahar/physical 3DS gameplay verification. Source-correct Step 10 is **partially implemented and must not be marked fully complete**.


### World Part 2 / Step 10 — live source pump and bed interaction wiring

- [x] Source A input resolves actual Map-layer 18 Sprite442/Sprite951 with the original 181 collider against active frame polygons; requires no selected inventory item, preserving the original manual door's existing priority. Invisible Rain layer 44 editor bed is rejected.
- [x] Real water pumps: native A starts root event UID 7034863253908448, waits original 2 seconds, then consumes up to 25 water and replenishes player thirst without exceeding 100, using existing WorldSystem source object mutation and v3 save. Exhaustion and full-thirst cases do not consume water. Pending event validates the same runtime UID and resets when traveling/new game.
- [x] Bed Sprite951 selects original atlas animation0/frame1 upon native interaction and resets to frame0 when leaving original sprite overlap. This is original root interaction event UID 948724703580895 / reset UID 903835777805920, not guessed bed texture coordinates.
- [ ] Complete original `SleepApproved`, time-skipping `GotoSleep`, Building-mode bed placement and deconstruction before full Step 10 signoff. Do NOT invent a sleep reward or time skip while source conditions remain unported.


### Part 2 / Step 10 — original bed sleep preconditions (2026-11-22)

- [x] Read and executed the root `data.deobfuscated.js` sleep event trace via `tools/stage1/trace_furniture_sleep_source.py` and its GitHub workflow; original `Bed_activated` event UID `760795525771502` calls `Count_sleep_hours`, then `SleepApproved` and `GotoSleep`. Verified rejection events: daytime `373077181447985` (`360 < Timer_ALL < 1440`), temperature below zero `648183463842982`, player var13 hunger below 10 `993930551645035`, and player var14 thirst below 10 `413036232407346`.
- [x] Added native source-derived bed precheck and controller feedback with regression threshold cases. The UI does not give a sleep reward or move game time on passing only these conditions.
- [ ] Full root `SleepApproved` includes separate raid, shelter, status and nearby-enemy exclusions; `GotoSleep` advances many gameplay systems through a source event family. These are NOT yet implemented. Do not call Step 10 fully complete or claim original-source sleep parity.


#### Step 10 — original bed crafting, construction, and sleep source reference (NOT implemented)

- Source original `Sub_menu` offers `Craft_bed` from Woodpiles item **50** plus Deerskin item **714**, or `Craft_bed1` using Woodpiles **50** plus Bear Hide **784** (source menu events 824213715547664, 311727557962900). Both source paths call `Building_mode(12)` at action UID 400129545571316 / original event 393115516051013; the latter sets `bear_pi=1`. Do not misrepresent crafting a bed as producing an inventory item.
- Original `Building mode` function (event 1025534786344989) makes object **573** ghost at player X+50, with source frame **12** from `build_ghost_obstacle-sheet0.png`, authored **37×21** and hotspot **(0.6,0.7)**. Validity event **811103055423252** requires the ghost overlap with original house wall zone object **346** and distance to player less than **100**, before builder image frame0 confirms deployment. Event **125247805541322** has an original **3-second delay**, then spawns bed object **951** on `items_on_ground`, sets bed instance variable0 from `bear_pi`, and destroys the ghost. Bed source `bed-sheet0.png` contains two exact frames: frame0 rect **(1,1,37,21)**, frame1 rect **(1,24,37,21)**, hotspot (0.5,0.5).
- Original bed dismantle event **622329722190205** spawns object317, waits **1 second**, returns item **50** and item **714 or 784** according to bed variable0, and destroys bed951. This has other equipment/perk and original inventory overflow guards; do not grant rewards without them.
- Original source `Bed_activated` event **760795525771502** checks survival/night/temperature/perk/nearby enemies before `GotoSleep` event **242979671087187**; count available sleep hours is `Count_sleep_hours` event **974086311952240** and advances all dependent survival/time effects. A shortcut that only changes clock or health would not be source-equivalent. Source `perk_sleep` 10-second interaction event **422434553865245** is distinct from actual `GotoSleep`, and cannot be treated as a complete sleep implementation.
- These source events are the remaining Step 10 porting contract. The currently passing code only implements source bed detection/original frame and source water pump use/persistence. Do not check the entire Furniture Interactions step until construction, sleep, dismantling, regression tests and emulator verification are completed.


### Part 2 / Step 10 — original automatic bed selection frame (2026-11-22)

- [x] Root Game_events 903835777805920 resets bed951 to frame0; overlap event 948724703580895 changes original bed sprite to frame1 when the player181 overlaps. Native frame highlight previously required manually pressing A and could not show the original proximity selection.
- [x] Live source furniture update now synchronizes every active bed's original frame0/frame1 with player collision each tick, rejects editor bed on Rain layer44, and preserves existing A input interaction. Native test cases cover original frame selection and disabled/invisible layers. Uses original bed atlas, no guessed sprite coordinates.
- [ ] Test original visuals in Azahar/physical 3DS and complete full source sleep/deconstruction/placement events before Step 10 100% completion.

- [x] **Automated verification PASS for latest Step 10 code commit `8668714e80a55db37cd27539ac3d770c45f506bb`:** [Native World + Player Regression #45054384355](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45054384355) both succeeded, [3DS executable build #45054384311](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45054384311) compiled and uploaded its `.3dsx`, and [original graphics audit #45054384301](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45054384301) succeeded. CI is not proof of visual emulator parity or the missing GotoSleep/building mode/deconstruction pathways.

### Part 2 / Step 10 — original-source bed build, sleep and dismantle implementation (2026-11-22)

- [x] Authoritative root source data.deobfuscated.js extracted events, not this tracker. Craft_bed / Craft_bed1 use original item50 with item714 Deerskin or item784 Bear Hide. Original Building_mode UID 1025534786344989 generates Sprite573 frame12 at player X+50; exact 37x21 crop within build_ghost_obstacle-sheet0.png is X52,Y1 with hotspot (0.6,0.7), and custom source 4-vertex collision. UID 811103055423252 requires wall346 overlap and player distance <100; UID 125247805541322 waits 3 seconds then SpawnAnotherObject(951,items_on_ground,0), sets material var0 and destroys ghost. Native crafting touch UI exposes two recipes, circle-pad moves exact sprite frame, A confirms in-wall placement, B cancels, source delay and atomic ingredient debit, source sprite951 generation; original collision masks and preindexed wall slots used.
- [x] Original bed951 source bed-sheet0.png has exact frame0 crop (1,1,37,21) and frame1 crop (1,24,37,21); original selection UIDs 903835777805920 and 948724703580895 drive proximity highlight; no guessed images, Rain editor bed44 rejected.
- [x] Root UID 622329722190205 dismantles source bed, selects original chop sample and source Sprite317 from test_bullet_hit-sheet0.png, Wait(1), refunds original item50 plus original material714/784 and destroys original bed. Native X while overlapping bed invokes the delay; atomic inventory/ground refund, stable-runtime-UID tombstone and source visuals/audio are wired.
- [x] Original Bed_activated / SleepApproved / Count_sleep_hours UIDs 760795525771502 and 974086311952240 and GotoSleep 242979671087187 translated to native controls. Native rejects original daytime restriction, negative source temperature, hunger or thirst <10, missing shelter346, native wound/illness, and nearby tracked threats. Count_sleep_hours source limits use starvation/thirst rates plus source perk values, 5s hunger cadence, 1.5s source-minute and sunrise at original Timer_ALL=360; original Timer_ALL=1440 has no sleep time. Following original Wait(1), native performs full integer source stat-loop iterations, camel/hamster/sweethome adjustments, body-warmth ticks, original minute tick clock and ambience refresh. Original global side effects in unported game stages remain open.
- [x] Original source world visual sidecar upgraded backward-compatibly to version4: checksummed dynamic constructed beds (UID,x,y,hide variant,active/tombstone), retains v1/v2/v3 readers and original per-Sprite change journal. Native world regression tests source bed spawn/destroy, save roundtrip and UID stability.
- [x] Tests compare authored Construct source 573/951/317 frame coordinates and original event UID provenance, ghost custom collision polygon, original material debit and return, sleep hours thresholds, and v4 world sidecar roundtrip. CI fixture link moved to after generated source item catalogs.
- [x] **Automated Step 10 source-code verification PASS:** commit `0b9aa6268dc369c020094b7915d4b45ddb0d01e4` passed [Native System Regression Tests #45057689193](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45057689193) (world-tests and player-tests) and [Build 3DS Executable #45057689112](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45057689112) (compiled `.3dsx`, source sprite extraction, native original bed/ghost frame/mask checks). [Original Graphics Audit #45057564804](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45057564804) passed on preceding commit. These are real GitHub Action conclusions, not assumed. **Step 10 core gameplay code is complete and CI-verified** for the current single-player engine; full Construct cross-system and emulator equivalence are separate remaining acceptance gates.
- [ ] Full Azahar/physical Nintendo 3DS visual verification and advanced original GotoSleep side-effect branches depending on presently unported raid/perk/quest/fire systems remain for later integrated game parity. No claim of full game 100% visual/gameplay parity.


### World Part 2 / Step 11 — original interior container interaction (2026-11-22)

- [x] Audited authoritative root `data.deobfuscated.js` for independent container sprites and direct Construct event references. House interior furnishings are painted into the original per-house `*_int-sheet0.png` sprites, not free-standing searchable storage objects; do not invent wardrobes/cupboards or assign them six-slot inventories.
- [x] Identified actual source interior loot container: bunker Sprite796 `bunker_lootbox-sheet0.png` / `max_box-sheet0.png`, Map `items_on_ground` layer18. The original 796 animation indices0–3 all have frame0 and frame1. Root UID `389950912936355` triggers on Player181 collision when unopened, switches Sprite796 to frame1, calls Fade.StartFade and marks original bool1=true. Original source also has independent airdrop710, car loot241 and player-deployed stash995; those are not house storage cabinets.
- [x] Added source-specific 796 collision-triggered open handling to native 3DS scene, with original per-frame collision and original frame1, no invented interaction distance, no invented cabinets or arbitrary contents. The original frame transition is retained in the existing v4 world visual save sidecar; source frame1 prevents a duplicate open trigger after Continue. Added native eligibility tests for all 4 animations, opened frame, invisible sprites, editor/wrong layer and wrong object.
- [ ] Original lootbox Fade behavior and actual category-specific drop actions (source 796.var0 categories 51/52/53/113/456789/999/1020/1021/1022), bunker-room setup/transitions, and Azahar/physical 3DS confirmation are not covered by this narrow original container interaction handler. Item generation belongs to Step12; dungeon access is a separate original world travel dependency. Do not certify full source gameplay parity from the opening trigger alone.


### World Part 2 / Step 11 — verified original container-opening interaction (2026-11-22)

- [x] **Source code and automated tests PASS:** commit `076d1484214f907041ea511e2d952354adf386d6` passed [Native System Regression Tests #45060271183](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45060271183) (both world-tests and player-tests success, including original root Construct event and all four Sprite796 two-frame atlas contracts), and [Build 3DS Executable #45060271137](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45060271137) (success, compiled and uploaded `.3dsx`). [Original Graphics Audit #45059969952](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45059969952) passed on preceding source-interaction commit. Initial native regression failed because the new shell test used two literal backslashes; fixed before the successful run. No untested build has been marked passing.
- [x] **Original-house interior container audit resolved:** no independent searchable wardrobes/cupboards are evidenced by the original source house interior sprites. Reuse the existing native tent and six-slot Supplies Stash storage, and original bunker Sprite796 collision/open events; do not invent a per-house inventory container or pretend an integrated bunker room exists yet.
- [x] **Step 11 container trigger/state implementation complete for source-defined instances:** original Sprite796 collision event `389950912936355` opens only unopened original lootboxes, selects authentic frame1 in all four source animations, and the world visual sidecar retains that opened frame across save/Continue. The engine receives the event automatically on overlap rather than inventing a button prompt. This closes the narrow interaction-event implementation milestone, not the cross-system bunker/loot pipeline.
- [ ] **Separate future integration dependencies:** original bunker entry and category-specific loot drops must be connected by world/bunker and Step 12 item-spawn work; original C2 Fade animation is part of animation fidelity. Azahar and physical 3DS testing are intentionally deferred by the project owner until final integration. Do not claim complete bunker gameplay or visual parity from the Step 11 event tests.


### Map generation / Azahar reports no visible buildings — investigation and source fix (2026-11-22)

- [x] Reproduced **real native house-parent placement geometry bug** in source `WorldSystem::expand_map_element`: root original object-level cached sizes/hotspots are 0×0, while the actual original Construct Sprite animation-0/frame-0 carries positive width, height and hotspot. A zero-sized house foundation collapses the ImagePoint offset used by its attached roof, wall, doorway and interior sprites. Fixed root-generated houses to read `source_building_frame_geometry(object,0,0)` before fallback to cached ObjectDef metadata ([commit 852e29788914](https://github.com/Youtelater/Minidayz-Multiplayer1/commit/852e29788914a1b3b7e3d86e88ebad4e3bed4b07)). No house positions, recipes or artwork were invented.
- [x] New native regression verifies **all 51 original category-7 house variants** preserve original Sprite width, height, hotspot, and positive sizes ([commit 1c16341dd81b](https://github.com/Youtelater/Minidayz-Multiplayer1/commit/1c16341dd81b)). Every source-generation level verifies actual runtime town markers, house bases, nonzero dimensions, source Map visible layer, source placement culling and roofs; native tests logged 118/108/77/57/174/181 house bases on fixed six-map seeds. Buildings in a generated world are confirmed; a freshly spawned survivor may begin in wilderness, so houses need not be immediately in the 400×240 viewport.
- [x] Automated world/player tests for geometry fix passed: [#45065698371](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45065698371). 3DS executable build and original graphical audit passed for that fix: [#45065698315](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45065698315), [#45065698372](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45065698372).
- [ ] **Azahar gameplay rendering verification remains open**. These source-code tests and the 3DS build cannot establish visually that a new `.3dsx` displays house pixels on the user's specific emulator. Do not conflate original house counts with an emulator screenshot. If no houses render on the corrected build after reaching a town, inspect runtime T3X loading and Citro2D rendering rather than changing original map spawning probabilities.


### Azahar report — missing visible buildings; original frame geometry repair (2026-11-22)

- [x] **Investigated the reported 3DS/Azahar no-building symptom before continuing loot.** Generated original houses are demonstrably present: source world seed regression on all six native worlds found respectively 118, 108, 77, 57, 174 and 181 authentic category-7 house foundations. Each has a valid visible Map layer, positive opacity, native sprite object and GPU visibility-index inclusion. See [native world/player tests #45064598930](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45064598930) and [#45065698371](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45065698371).
- [x] **Fixed an actual original-building placement bug**: the compact `stage1_data.hpp` object defaults have zero width, height and origin, and `WorldSystem::expand_map_element` incorrectly used these for generated original house foundations. Original spawned roofs, walls, interiors and doors inherit named Sprite image points relative to the foundation dimensions: zero parent size collapsed those original offsets. `source/world.cpp` now reads exact original animation0/frame0 width/height/hotspot from the root-derived `source_building_frame_geometry` table, with the existing fallback only for non-covered objects. This is a source fidelity fix, not arbitrary placement. Commit `852e29788914a1b3b7e3d86e88ebad4e3bed4b07` passed [native tests #45065698371](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45065698371) and [3DS build #45065698315](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45065698315).
- [x] **Expanded regression to all 51 original house primary variants**, requiring exact first-frame dimensions and source hotspots instead of just checking that a raw object was spawned. [Native tests #45065774189](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45065774189) passed both player and world suites on this strengthened test.
- [x] **Checked actual 3DS house art assets**: 51 original primary house types and 107 unique source companion frames resolve to packed T3X files (verified in [3DS build #45065479355](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45065479355)). The renderer now also frees no-longer-needed menu sprite sheets before world art allocation; this avoids retained startup texture VRAM competing with generated-building atlases.
- [ ] **Azahar visual verification still open**, per the player's report: successful native model, source packing and 3DS compilation tests do not prove textures appear in the emulator. The original source player starts at X=600 on a world much wider than a 400px viewport. Confirm whether buildings are missing at a marked town on the Y map with the newest rebuilt `.3dsx` (not just the wilderness spawn) before treating the symptom as fully resolved. Do not invent a new player spawn, relocate houses, or claim 100% emulator parity.


### Original source versus native 3DS building-loading parity audit (2026-11-22)

- [x] Compared root `data.deobfuscated.js` house-generation event extractor, native `WorldSystem::expand_map_element`, original atlas builder and Citro2D render. Original category-7 house creation has 51 directly verified literal Sprite variants; root companion Spawn-another-object actions, per-frame image points, roofs/walls/doors/shadows and 153 house loot markers are extracted directly. Native Map generation loads the authored Map instances, generates original-inspired grid locations, reconstructs house/companion objects and draws the original individually cropped animation/frame textures in source Map layer order.
- [x] Fixed a real visible-render mismatch: original Construct sprites rotate about their active image hotspots, while native Citro2D previously rotated uncentered quads about the sprite's top-left. This also produced wrong rotated-sprite viewport culling. Added shared original-hotspot draw origin and tested displaced 90-degree pivot, mirrored zero-angle and viewport scenarios; original source art/crop coordinates untouched.
- [x] Protected original Sprite and `draw_original_image` T3X atlas pages from replacement inside a single GPU frame: 32-slot cache now recycles only unpinned pages, then releases them safely in a future frame. If all 32 pages were submitted in one frame, additional textures cannot be loaded without risking invalidating previous draw commands. This hardware limit is not identical to the browser texture manager.
- [x] **Latest audit head `0339e22809f505139b517cdf82a4acbc5a2b93b4` PASSED both native test jobs** ([#45067295696](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45067295696)) **and 3DS executable build** ([#45067295697](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45067295697)). Code commit `8d4283f66add34ac787e7a92c33b1332675ada00` also passed [native #45067081222](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45067081222) and [3DS #45067081211](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45067081211). Source graphics audit passed [#45067045933](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45067045933). The first added native job had a double-backslash shell line continuation, corrected before these successful runs. This is passing code/build verification, **not** a claim of pixel-perfect emulator equivalence.
- [ ] Azahar visual rendering of original houses remains deferred. Browser Construct RNG, browser texture loading, and 3DS GPU page constraints are not pixel/seed identical; when visual verification resumes, inspect towns and runtime atlas capacity rather than mistaking an empty wilderness spawn for failed map generation.


### Azahar / Manic Emu screenshot regression — large purple city region (2026-11-22)

- [x] Source-backed root cause isolated from seven reported emulator screenshots: original Sprite404 `ach_city_counter-sheet0.png` is only **2×2 solid purple pixels**, but the native generated world creates **1000×1000** region instances on original Buildings layer41 with 100% opacity. Previously the generic sprite renderer drew them directly, creating giant purple panels and hiding world/player visuals. Original root PNGs and tilemaps contain no comparable intended large purple terrain.
- [x] Preserve each original Sprite404 world placement, event/instance UID and region geometry for gameplay/achievement logic, but skip **only the Sprite404 pixel draw** in the world renderer. No guessed asset coordinates, no alteration of source PNGs, and no hiding of genuine house/door/tree sprites.
- [x] Add deterministic native per-map regression for all six levels asserting the actual 1000×1000 object404 placements are recognized as nonvisual, while ordinary building/door/tree objects remain drawable.
- [ ] Visual confirmation of absence of the purple area in Manic Emu/Azahar after this patch is **deferred**; passing native/3DS CI alone does not certify gameplay visuals. Other reported issues (tree/player occlusion, blocky terrain seams and bottom-screen layout) still require separate source comparison and fixes.


### Screenshot world regression verification — native and 3DS CI (2026-11-22)

- [x] **Identified source overlay cause:** original `ach_city_counter-sheet0.png`, Sprite404, is a 2×2 magenta marker scaled to 1000×1000 in real generated map instances on Buildings layer41. Six original world-level samples contain these opaque region objects (world-test log run #45070872649). The original `ground_lvl1..7` tilemaps do not contain the purple rectangle.
- [x] **Code fix:** preserve source marker instances for event/achievement logic and remove their erroneous GPU draw only; leave actual house, tree and door sprites visible. Commit `d8eb8fc050b017099e2042d75180bccae3c63d4e`.
- [x] **Bottom HUD truncation:** shorten temperature/bleeding/infection labels while keeping actual numerical values so their right edge no longer collides with the touch display boundary. Commit `80c54d2709bb20378ea8d1e71b582eeb95d80f0f`.
- [x] **Passing automated verification on latest code `80c54d270`:** [native world and player regression #45071384493](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45071384493), [Original Graphics Audit #45071384534](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45071384534), and [3DS executable build #45071384519](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45071384519) all completed successfully. Includes separate original pixel/color audit workflow #45071065630 and tree-event source trace #45071247915.
- [x] **Source parity note:** Trees layer43 paints above player layer32 in the original Map; player occlusion by a large tree is not proof of a layer bug. Original tree family1035 actions were traced without finding a direct tree-fade/opacity event. Do not add fabricated translucency without an original source event.
- [ ] **Pending visual verification:** load the build containing the Sprite404 fix in Azahar/Manic Emu to confirm purple artifact and HUD clipping are visually gone. The screenshots predate the fix; CI builds alone do not prove emulator display parity. Also audit the 30px original environmental tile squares and other issues which were not documented in screenshots. Never mark unobserved graphics corrected.


### Original audio event-parity audit — rain/blizzard sound dispatch (2026-11-22)

- [x] Identified native sample-selection error: `kSourceDefaultRainsLevel==2` is a constant true expression, causing the blizzard loop to play on every raining map even outside the snowy level. That constant defines rain eligibility, **not** which weather audio sample to play.
- [x] Changed the three weather-start/resume/unmute call sites to check `world_level_==4`, consistent with the same source winter-world branch used for winter ambience. Original `rain_loop.wav`, `blizzard.wav`, and sheltered `rainroof.wav` assets are retained unchanged. Added a native CI source-dispatch contract test.
- [ ] Audio is **not 100% verified**: original Construct has 461 direct sound-event action sites; the port must compare the exact trigger conditions, sound selection, volume, loop, stop, cooldown, and event timing for all. Other reported out-of-time playback issues require separate reproduction and parity checks; emulator audio verification intentionally deferred.


### Full-port Step 1 — root graphics and runtime pixel-visibility review (2026-11-22)

- [x] Audited original `data.deobfuscated.js` object/frame references and `assets/gfx` using `tools/stage1/audit_original_visual_roles.py`; independent source action ancestry distinguishes actual scenery from editor templates, event markers and building previews. The submitted lime-green image is root `build_ghost-sheet0.png`, Sprite571; related `build_ghost_obstacle-sheet0/1.png` is Sprite573. Both source Map template instances are staged at negative Y with alpha zero, and source actions belong to GUI → Building mode. **They must remain available for live placement previews, not ordinary world rendering.**
- [x] Found and corrected generic top-world rendering of original source masks: player collision/anchor 181/193, loot marker218, red obstacle collider345, warmzone trigger346, lime-green big-obstacle collider348, achievement region404, map-element508, mapgen514, polygon target857, collision checker984. Source IDs and canonical PNGs are checked against root Construct data. Colliders, world placement records, interaction and persistence remain unchanged, as does visible door349, shadow347, tree184 and house321 art.
- [x] Prevent zero-opacity source sprites from consuming 3DS atlas slots; report separate per-frame `DROPS` counter beside `TEX LOADS` in the optional on-screen performance overlay. This diagnoses sprite loss due to unsupported frame, failed T3X load, or exhausting the 32 active GPU atlas pages without silently claiming every graphic loaded.
- [x] Added source-backed original green ghost activation / sprite-mask contract in world CI plus native world-system exclusion tests; do not alter any original PNG or manufacture coordinates.
- [ ] Original graphics source role inventory is **not full event parity**: some of the 1,854 PNG assets are UI/effects or only conditionally visible. Every original activation/event branch, runtime animation frame transition, original texture tint, 3DS memory loading failure, and Azahar output must still be checked before Step 1 can be certified 100%. Treat source-frame packaging as distinct from correct gameplay visuals.


### Step 1 graphics parity — verified native/GitHub build results (2026-11-22)

- [x] Root original source visual role identification passed [Audit Original Visual Sprite Roles #45073042851](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45073042851). The user-supplied neon-green `build_ghost-sheet0.png` is original Sprite571; `build_ghost_obstacle-sheet0/1.png` Sprite573. The game's original Map templates are out of bounds with opacity zero; the only direct original action groups are GUI → Building mode.
- [x] Corrected generic renderer so original green/red collision masks, warmzone/loot/mapgen/player markers and city achievement regions are *not* painted as ordinary scenery while preserving object state and collisions. See code commit `3c47cba7cde4bdbbd1e1cc36d771b32e1ad520ef`; source event-activated ghost previews remain separately drawn.
- [x] Native System Regression Tests [#45073236585](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45073236585) — both world and player jobs passed, including source-root visual role regression. Original Graphics Audit [#45073236623](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45073236623) — 1,854 original PNGs and T3Xs, 14,505 original source frame references, no missing original source PNG references. Build 3DS Executable [#45073236616](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45073236616) — PASS and `.3dsx` uploaded; separate validator confirms 14,505 source frames, 1,812 atlas pages, 1,735 frame sheets, 32 off-edge frames padded transparently.
- [ ] Step 1 remains **not certified as identical gameplay output**: Azahar running the latest build has not been checked for no unexpected graphics, 3DS texture DROPS, construction preview activation, correct item/UI frames, active original animation branches, or residual terrain seams. Keep these as open verification issues rather than treating CI success as proof of full source visual parity.


### Graphics / Step 1 — complete canonical source-role inventory and verification gate (2026-11-22)

- [x] Every **1,854** canonical original PNG now appears in one complete per-file `full_original_visual_role_inventory.json` artifact, with a conservative source role, original Construct object IDs, authored frame count, Map template count, zero-opacity/hidden-template flags, and original event action-reference counts. Audit explicitly distinguishes **1,736 Construct sprite-animation PNGs**, **43 other source tilemap/texture PNGs**, and **75 original PNGs without a directly mapped source frame or texture**. Those 75 are **not automatically unused or always hidden**.
- [x] Exact **14,505 original source sprite frames** and **0 missing referenced PNGs** asserted against root `data.deobfuscated.js`; construction ghost571/573 and nonvisual source collision/achievement helper sprites are checked against real original object IDs. Excluded pixels do **not** delete the placement, collision, event UID or source PNG.
- [x] [Complete original graphical-role audit #45078718183](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45078718183) passed, including new `tests/test_complete_original_visual_roles.py`. The earlier [native player/world tests #45073441872](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45073441872) and [3DS build #45073442085](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45073442085) passed on the same previously unchanged renderer code. The new commit only expands the source auditor, adds tests, and documentation.
- [ ] **Visual equivalence remains unverified by emulator.** The 32-active-page GPU cache can report per-frame `DROPS`; animation states and object-specific event activation are part of their respective gameplay systems. Root-source asset completeness does **not** prove every animation/frame is displayed correctly in Azahar. Per user instruction defer Azahar/physical-3DS visual checks until full integration; do not claim 100% pixel-perfect parity.


### Step 2 source-driven Map array POI range correction — 2026-11-22

- [x] Compared root `generate_array_locations` event UID `6938246402369311`, including nested level branch `214943915460313`, to native `WorldSystem::place_poi_count`. Exact root expression is `floor(random(1, Map_size_X/Y))`, then Array write at `check_X - 1` / `check_Y - 1`. Original Construct `random` upper endpoint is **exclusive**; native previously included it and could place primary POI centers on the last column or last unsplit row, shifting roads and location coordinates.
- [x] Corrected original last-column/row exclusions, the first/split half max bounds, and the same gas-station POI center sampling while preserving original five-cell footprint writes. Added root-source expression parity Python test and native multi-level/multi-seed boundary coverage.
- [ ] Original JS RNG seed/state and draw ordering, mapgen-to-world cell coordinates, exact array-to-location activation, all original terrain SetTile effects, and Azahar pixel output are independent parity gates; do not confuse source-bound correctness with 100% map equivalence.


### Step 2 exact source road generation row ranges (2026-11-22)

- [x] Traced the original root `generate_array_roads_1` (UID `921061167763666`) and `generate_array_roads_2` (UID `484111970403648`) event trees. Their five Array518 road-anchor codes exactly match native `{6,9,11,15,103}` in both horizontal and vertical source guards. Original `Map_size_X`/`Map_size_Y` are 16 by default (a challenge mode temporarily sets 3, then 16).
- [x] Found native late-level error: horizontal generator looped across **all 33 grid rows**, while original horizontal `Repeat(Map_size_X)` stops at row 15. Only the vertical source function adds a second pass at `Y+17`. Stopped inventing second-half horizontal roads, kept the authentic vertical second-half pass, updated world road adjacency regression and root source guard check.
- [ ] Original browser PRNG/call ordering and the exact Mapgen sprite world positions remain separate parity items; source road-code/loop parity does not prove final Azahar road rendering matches.


### Step 2 — source world generation corrections and CI evidence (2026-11-22)

- [x] Root-original **POI grid boundaries** corrected: Construct `floor(random(1, Map_size_X/Y))` is half-open and writes to `check_X-1/check_Y-1`, unlike the earlier native inclusive end which shifted villages, military/city sites, hospitals, fire stations, gas stations and roadside cafes toward the final map column/row. Both main and split world halves follow original half-open bounds.
- [x] Root-original **road generation loops** corrected: `generate_array_roads_1` UID 921061167763666 repeats `Map_size_X=16` horizontal rows, while `generate_array_roads_2` UID 484111970403648 uses `Map_size_Y=16` vertical pass and `Y+17` for the second split-half. Previously the native 16x33 world wrongly created full second-half horizontal roads. Both directions guard exact root array anchors 6, 9, 11, 15, 103.
- [x] Source-based road/function/Mapgen tracer scripts committed; complete native source/road/POI regression and player tests **passed**: [#45082339021](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45082339021). Rebuilt 3DS executable **passed**: [#45082339041](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45082339041). Original graphics audit **passed**: [#45082339066](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45082339066), root road/event trace **passed**: [#45082339107](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45082339107), focused Mapgen514 action trace **passed**: [#45082541320](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45082541320).
- [ ] **Step 2 full original-world parity is not closed.** The original runtime Mapgen514 world-coordinate placement, exact global Construct/browser RNG semantics and event order, all procedural SetTile side effects, generated forest density/terrain seams, dynamic location lifecycle, and final in-Azahar rendered structure/road/terrain correctness remain unproven. Do not mark source trace, a successful 3DS build, or current native output as pixel-identical to the original game. Step 3 is gated on these source-comparison tasks per the user's reset plan; loot work stays paused.


### Step 2 original Mapgen position-action audit (2026-11-22)

- [x] New root source trace [#45082804980](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45082804980) enumerates the original `Mapgen` Sprite514's runtime actions: **42** SetInstanceVariable actions (Construct opcode88), **two** SetPosition actions (opcode47) and one SpawnAnotherObject action (opcode73) in unrelated Quest logic. The two original SetPosition events are UID `1131020651387518` and UID `156278692855564`, both assigning `Mapgen.X+500, Mapgen.Y+500` under Map generation. Sprite508, not 514, is the source of dozens of original tree/building SpawnAnotherObject events. Distinguish sprite movement from other world-array marker creation; do not invent a fixed world anchor.
- [ ] Native `generate_source_locations` still uses C++ bounds/grid-derived cell centers, not demonstrated execution of the original Sprite514 position-setting event sequence. No source-exact replacement can be certified from the isolated action trace alone; root ancestor conditions and original runtime object iteration need execution-level verification. Keep this a Step 2 blocker, rather than claiming the source map is identical just because generated house counts are nonzero.


### Procedural terrain source condition correction (2026-11-22)

- [x] The original `build_procedural_environment.py` incorrectly interpreted *every* Sprite514 variable compare as marker code. Corrected extraction to only accept Sprite514 **slot 0** literal equality, excluding source grid-X/Y slots 1/2. This prevents unrelated 3DS terrain artwork on the wrong location type.
- [x] Preserved original per-level `CurrentLevel` source guards as `TerrainScatterRule.level_mask`. The renderer only paints those source branches on levels where their original ancestor comparisons permit them. Native and root-source tests added.
- [ ] Original browser random call order, procedural water/ground-level SetTile effects, withheld perk branches and visual-in-Azahar verification remain unresolved; do not call overall Step 2 pixel-identical.


### Step 2 root Mapgen dispatch provenance — corrected after complete leaf audit (2026-11-22)

- [x] Verified all **22,343** root Construct events and **14,560** seven-field event leaves. Earlier notes reporting zero actions and zero Sprite514 creations were INVALID: those older audit walkers incorrectly dropped seven-field leaves. Original `generate_array_tomap` UID `1694439930581699` has **102** descendant actions, including **70** Sprite514 SetInstanceVariable actions; original `generate_location` has **5,747**.
- [x] The original **Map** boot event UID `3669795404796534` explicitly creates one Sprite514 per `Repeat(Map_size_X*Map_size_Y)` iteration, on `zed_resp` layer. It uses exact **X=700+1020*int(loopindex/Map_size_X)**, **Y=320+1020*(loopindex modulo Map_size_X)**. This corresponds to physical world X following the Array Y axis. The source Move +500 action in UID `1131020651387518` occurs AFTER `generate_array_tomap` and `generate_mapgen_toelements` calls; therefore pre-move 700/320 coordinates are the correct active generation hotpots, not 1200/820.
- [x] Native mapping, physical road orientation, terrain scatter and direct Mapgen creation AST assertions match this source sequence. Original `c2runtime.js` expression opcode17 means **>=**; native equality implementation was corrected to the original >= operator with regression tests (also supports source comparison ops12–16).
- [ ] Runtime source-driven POI counts, category-conditioned markers and building placements are tested natively, but full JavaScript RNG draw-order parity and emulator pixels remain unverified. Those are separate sign-off gates, not reasons to reintroduce synthetic normalized cell centres.

### Generated tree collider integration — 2026-11-22

- [x] Found source-visible regression that old native test missed: `ORIGINAL COLLIDER BASES indexed=0` despite 1,525 generated tree sprites. Root `create_map_element` spawns invisible `obst_base` object **345** on `zed_resp`, not `Obstacle_enviroment`. The old native broadphase and narrowphase wrongly filtered it away, letting the player pass through trunks.
- [x] Shared guard `source_linked_tree_collision_hitbox` recognizes only linked original Sprite345 on source `zed_resp`. Wired BOTH `SourceObstacleIndex` and `source/main.cpp` player collision; preserved hidden graphics and kept unrelated zombie/loot marker instances nonsolid. Native tests require dozens of in-bounds indexed tree hitboxes.
- [ ] Native fixes do not prove original runtime trunk collision polygon or fallen-tree collision state parity; those require source `Hitting_obstacles` event and original mask comparison before 100% gameplay certification.


### Step 2 original source pump choice semantics (2026-11-22)

- [x] Root-original `create_map_element` event **3903561981802149** assigns pump Sprite442 var0 with `choose(100,200)`, which is **not** `random(100,200)`. Its source AST is `[19,82,[[0,100],[0,200]]]`. The old native code rolled any integer from 100 to 200, which was incorrect. The replacement explicitly uses `source_random_bounded(rng_,2)` to select exactly 100 or 200 and ensures deterministic replay on 3DS and host tests. Contract tests verify the exact original AST, not guessed event names. This does not prove the entire game's PRNG call order matches the browser.

### Step 2 critical source-audit parser correction — seven-field original Construct events (2026-11-22)

- [x] Audited exact root `data.deobfuscated.js`: **22,343** Construct event nodes, of which **14,560** are valid **7-field leaf events**. Several newer map diagnostics incorrectly required 8 fields, so their claims that `generate_array_tomap` had **0 actions** were FALSE. Its root UID `1694439930581699` actually has **102 descendant actions**, including 70 original Mapgen514 instance-variable writes. Root `generate_location` UID `5017992314331459` contains **5,747 descendant actions**. Source pump init UID `3903561981802149` is also a 7-field action event and sets Sprite442 var0 to `choose(100,200)` — not a fractional random expression.
- [x] Fixed the `extract_location_templates.py` source spawn collection to accept 7-field leaves; it already treated them correctly in the full executable location event stream. Corrected three separate source audits and pump source contract so future traces cannot lose real source world actions. Added regression asserting exact original action counts and seven-field-event preservation; rebuilt source location tables via native and 3DS CI.
- [ ] Prior `Mapgen` no-action / no-source-creation gap statements must be re-evaluated from corrected event traces before concluding an essential runtime feature is unimplemented. This correction is an original source-audit/manifest fix; whether original browser RNG and Azahar output are identical remains unproven.


### Step 2 original split-map terrain theme orientation (2026-11-22)

- [x] Source Sprite514 position uses `Mapgen.X=700+1020*ArrayY`. Thus the native 16x33 Map's second terrain half begins to the **right on world X**, not downward on world Y. Native base terrain sheet and original Tilemap RLE selector previously switched themes by screen Y and source road rendering likewise used Y, even though generated building/road/scatter placements now correctly transpose Array coordinates. Aligned both terrain render passes and the original asphalt layer to the source grid: second-half row18 starts at worldX=**19060**; added CI source-axis checks. This does not alter the source PNG sprites or artificially move existing authored tilemap locations.


### Step 2 source-generation engineering gate — final automated validation (2026-11-22)

- [x] **Source-array-to-world transform and rendering**: original Sprite514 Create event UID `3669795404796534` maps ArrayX/ArrayY to physical `(700+1020*ArrayY,320+1020*ArrayX)`. Native location actions, generated terrain 230 patches, source road corridor geometry and zero-based slot2 now agree. Source Map's 33-row second biome starts at physical **world X=19,060**, not screen Y.
- [x] **Map Array generation and scenery**: fixed exclusive Construct POI bounds, removed nonexistent second-half horizontal roads, corrected original source Mapgen event extraction of **14,560 valid seven-field leaves** (22,343 total events), fixed source tree hitbox indexing, original waterpump `choose(100,200)`, and rendered source asphalt and ground_enviroment on their original source Z layers. Source-generated terrain art and authored spritesheets were preserved, not replaced with approximated icons.
- [x] **Final latest HEAD checks**: commit `921f63885ffd` passed [Native System Regression Tests #45088064921](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45088064921) (both player and world jobs), [Original Graphics Audit #45088064961](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45088064961), and [Build 3DS Executable #45088064808](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45088064808) (`.3dsx` artifact uploaded). An intermediate build failed due to missing namespace qualification of the new source helper; commit `921f638` corrected it and the rerun passed.
- [ ] **Deferred final emulator/original-browser gates (not falsely marked tested)**: the game must be visually compared against original runtime in Azahar/3DS to validate building appearances, terrain seams, road appearance, dynamic procedural SetTile timing, and saved-game compatibility. Browser-specific `Math.random` draws and exact invocation order are not bit-for-bit identical to native C++ PRNG; perk-dependent scenery on specific progression levels is connected to the later perk system. Gameplay regression remains part of final integration, as agreed with the user; no emulator run was claimed.

**Step 2 core world-generation implementation and automated engineering gate: PASSED.** Proceed to Step 3 (original buildings, doors, roofs, interiors) using the new source-verified geometry; retain the deferred visual checks as hard final-integration signoff requirements rather than silently treating them as completed.


### Step 3 — original building/roof/wall selection parity (2026-11-22)

- [x] Compared original House_transparenting event UID `8794469330826744`, child `1340260606749434`, and door-child `7764770537466591` (root source trace #45090045639) against the live 3DS roof update. Confirmed the old port treated Player193 overlapping **any wall346** and foundation1048 overlapping **any other wall** as grounds to fade a roof. This incorrectly lets another building's wall drive a nearby roof's opacity/layer, door translucency and rain shelter.
- [x] 3DS now restricts source wall picking to the **same nearest source 1048 foundation selected for that roof**, using original per-frame collision-mask overlap with the picked 346 wall. Related door fade and rain shelter branches use that matched pick, not an unrelated wall. Added rotated and disjoint-house geometry tests, plus original-event live contract assertions.
- [ ] Original runtime visual comparison (Azahar/physical hardware), full Construct audio timing, and other gameplay system progressions remain separate final gates. This is the original Step 3 engineering audit; no emulator run has been claimed.


### Step 3 source-accurate parent guard for roof versus rain (2026-11-22)

- [x] Reviewed the original full House_transparenting event tree after fixing the seven-field source-leaf trace ([source trace #45090045639](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45090045639)). **Roof opacity 0 and inner door opacity 25 are guarded by UID 8794469330826744 (picked foundation1048 overlapping wall346)**, but rain shelter UID `1706727368417029` is a **sibling** under UID `1166878656779541` (Player193 overlaps wall346), not a descendant of the 1048 pick. Kept source-accurate global wall parent condition for shelter and inside/outside branch selection, while constraining the *actual* roof/door visual effects to a matched foundation; added native dispatch contract.


### Step 3 — original interior bunker container open-state parity (2026-11-22)

- [x] Root Sprite796 collision event UID `389950912936355` sets **both** animation frame1 and boolean instance-variable1 to true; the 3DS implementation formerly changed only the frame. Introduced a source-guarded, one-time `WorldSystem::open_source_bunker_lootbox` action and source var1 latch; reload derives this boolean from restored frame state without changing the binary save layout. The caller now uses the atomic original action instead of raw SetFrame. Added root-source contract verification.
- [ ] Sprite796's Fade.StartFade timing and original category-conditioned Spawn_drop rewards are reserved for the Step 5 items/loot implementation, not invented in Step 3; do not claim bunker loot has already been granted. This limitation and the Azahar visual test remain final-integration prerequisites.


### Step 3 — original buildings and interiors: source/engineering validation gate (2026-11-22)

- [x] Source 51 original category-7 house primary variants, all linked source Spawn-another-object exteriors/interiors, frame/image-point wall346/shadow347/roof1049/door349 and original closed-door Solid348 have exact source extraction and source-geometry regression checks. Do not substitute plain rectangles for authored original PNGs. The renderer retains original source Map layer9 shadow, Buildings_bg layer31, and Buildings layer41.
- [x] Corrected Roof rule UID `8794469330826744` to check the **same picked 1048 foundation against the overlapped 346 wall**, not any remote house foundation; its nested roof-zero opacity UID `1340260606749434` and door-quarter opacity UID `7764770537466591` no longer leak across houses. Rain shelter remains under the original broader wall event UID `1706727368417029` (source sibling; not incorrectly restricted by the picked-base guard).
- [x] Original automatic doors continue to wait 0.01 seconds before changing Sprite349 source frame and playing original wood/metal cues; linked original Solid348 is removed before opening and restored on closing. The 3DS implementation distinguishes original player collider181 from PlayerBase193 and maintains per-instance door UID and snapshot logic.
- [x] Corrected bunker interior Sprite796 original collision-open event UID `389950912936355`: frame1 and instance boolean slot1 now latch together **once**, with native save/reload reconstruction; test opens an original authored Map fixture and confirms a second collision cannot open it again.
- [x] Fixed original-roof and interior source audit scripts dropping **seven-field leaf Construct events**. Corrected trace recovered 27 active roof rules, 34 source house/companion event sites, 791 source house post-Spawn actions, 210 direct source building interaction event references and all actual Sprite796 actions in the root export. [Roof/house source trace #45090347673](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45090347673) and [interior source trace #45090347694](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45090347694) both passed.
- [x] **Latest Step 3 tested code `44c602d0240a37f5fa3941c1fd17e98228ce9c57`:** [Native System Regression Tests #45091039011](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45091039011) **PASS** (both player and world jobs), [3DS executable build #45091039050](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45091039050) **PASS** with upload; original graphics audit earlier [#45090868014](https://github.com/Youtelater/Minidayz-Multiplayer1/actions/runs/45090868014) **PASS** (same unchanged original art).
- [ ] **Deferred source event/integration boundaries, not falsely certified:** exact Sprite796 Fade.StartFade runtime fade parameters and all lootbox category-dependent drops belong to Step 5 item/loot event integration; browser audio event parity, native/browser PRNG sequence and save compatibility are cross-cutting later checks. Original-game-vs-Azahar visible pixels, interior navigation and hardware performance cannot be verified by native or 3DS CI and remain final integration signoff gates.

**Step 3 house/buildings/interiors engineering and automated tests: PASSED.** Next Step 4: Player sprites, original animations, input/movement and collision; defer gameplay-on-emulator visual signoff as agreed, without claiming source-perfect rendering.


### Step 4 player sprite and equipment world layers (2026-11-22)
- [x] Removed hardcoded 3DS depths `0.57f`, `0.575f` .. `0.605f` for player character and wearables. Source Map defines player/equipment at layers **32..39**; now draw with `source_world_layer_depth` to preserve correct depth against Buildings_bg layer 31, Buildings layer 41, and neighboring world objects.
- [x] Added source-parsed Map layer-name contract, native layer-order assertions, and updated screen depth regression. The original sprites and atlas frame coordinates remain unchanged.
- [ ] Original event-based movement speed and animation switching, runtime collision fidelity, and actual Azahar visual verification are separate gates for completing Step 4.
