# MiniDAYZ original-source vs Nintendo 3DS parity review

**Scope:** repository-root `data.deobfuscated.js`, `mp_itemtable.js`, `l_eng_*.xml`, `assets/gfx`, and the native `source/`, `include/minidayz/`, `tools/` port on `main`. The separate `Minidayz-Multiplayer-main/` folder is **not** the authority for this review. The 3DS adapts presentation to a 400×240 top screen and a 320×240 touch screen; pixel-for-pixel mobile UI is not the goal.

**Integrity checks:** `make stage1-assets` runs `validate_stage1.py` to compare the parsed Construct project against the root original and validate image/event inventories. The new `tools/audit_original_parity.py` checks the source snapshot, original menu/floor art, relevant runtime paths, staged world-generation calls, and source-derived inventory art. GitHub Actions runs it after a 3DS build.

**Important:** These checks do **not** prove gameplay is fully identical. A compiled port, a valid source extract, and correct art names are different from implementing every Construct condition/action and verifying in an actual emulator/device.

| Area | What the repository currently shows | Parity status / action |
|---|---|---|
| Original source extraction | The Stage 1 validation explicitly compares the root original and reassembled parsed Construct data. | Source-integrity implementation present; CI will verify at build time. |
| Startup menu | Uses original `main_menu_logo`, `menu_btn_wood`, `tiledbgmenu_city_up` images; reflows to both native screen sizes. | Adapted UI, not the exact mobile menu flow. Verify hitboxes, visual crop, and readability on hardware. |
| New Game | Calls `begin_source_generation`; advances across frames; exits loading only after `generation_complete` and NPC initialization. | Five implemented stages wired; confirm execution, error recovery, and load/save behavior on 3DS. |
| World grid and POIs | `source_world_profile` quota tests exist; grid, map markers, location template runtime, bunkers, bridges implemented. | Original-derived pipeline, not certified event-for-event identical. Existing `tests/world_system_tests.cpp` should execute in CI. |
| Terrain | The renderer now loads an exported original `ground_lvl1_tilemap` grid and draws the authored tile IDs; source flip flags are not yet applied. | Authored tile placement code committed; build/emulator verification and tile flip handling remain outstanding. |
| Player spawn | New games now search source-generated map location cells and check collision at candidate coordinates; 160,120 remains the initial pre-generation state. | Improved safety, not original-equivalent: exact source player spawn rules remain to be traced. |
| Movement and collisions | `WorldPreviewScene` explicitly sets 72/108 speeds and 5×7 half-size collision box as preview tuning. | **Known non-parity:** replace only after tracing the source events. |
| Inventory and equipment | Original inventory art lookups and source item catalogs exist; native lower-screen touch layout differs from phone layout. | Data mappings present; UI shape, scaling, item effects, capacity and drag/drop require behavior+visual tests. |
| Crafting, condition, item use | Source-extraction scripts and native transaction tests exist. | Partial source-backed implementation; do not infer full equivalence from passing item tests alone. |
| NPCs | Native spawn actors map original skin/eyes sources, and NPCs initialize after world generation. | Spawn mapping covered in tests; full original AI, attacks, pursuit, death and respawn need behavior comparison. |
| Combat | Native game scene still centers on the preview world and Stage 2 controls. | **Incomplete parity:** do not claim full original firearms/melee/grenades. |
| Navigation and multi-layout | Scene comments explicitly state the preview cannot enter the Bridge layout. | **Known gap:** original multi-layout transitions and traversal unported. |
| Audio | Some original WAV resources used by Stage 2 are validated for 3DS PCM playback. | Presence/format check only, not original event-to-sound coverage. |
| Saves | New world is saved after successful generation; loading reconstructs world from saved seed. | Round-trip and interruption/failure tests required; don't replace an old save until generation succeeds. |
| Performance and hardware | Top=world and bottom=touch UI; source atlas loaded from RomFS. | Real 3DS GPU memory, draw order, screen scaling, crash behavior and load duration remain unverified. |

## Hard requirements before declaring 100% source equivalence

1. Derive every remaining gameplay rule from **root original** Construct event sheets, preserving exact conditions, values, random choices, and ordering. Avoid invented rules.
2. Reconstruct original world tilemap placement, ground details and spawn selection instead of repeating one floor tile and spawning at a fixed coordinate.
3. Run original-vs-native behavior comparisons across seeded world creation, item/equipment/crafting transactions, survival ticks, NPC AI, combat, audio triggers and persistence.
4. Render and interactively test both 3DS screens: 400×240 upper-world image, 320×240 lower touch inventory. Remove redundant phone input UI only while preserving gameplay actions on physical controls.
5. Run the 3DS GitHub Actions build, host unit tests, and an actual 3DS/Citra-compatible emulator test. This review alone does not prove any of those succeeded.

Run the parity audit after asset generation:

```sh
make stage1-assets
python3 tools/audit_original_parity.py
```

The audit intentionally distinguishes **PASS** (specific source invariant), **GAP** (known/untested parity), and **FAIL** (broken source/asset wiring). A build with GAP lines must never be labeled 100% faithful.
