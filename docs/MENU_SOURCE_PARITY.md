# MiniDAYZ 3DS — original boot-menu source fidelity

**Source of truth:** root `data.deobfuscated.js` as extracted into `data/stage1/construct_project_parsed.json` (`Menu_Events`), original root `assets/gfx`, `l_eng_ui.xml`, and source `mp_menu.js` *only* as a reference for the multiplayer modification's additional DOM menu. No root `images/` duplicates or `Minidayz-Multiplayer-main/` substitutes.

## Implemented 3DS menu UI

| Source-backed feature | 3DS state |
| --- | --- |
| Boot background, source logo, wooden buttons | Drawn on upper/lower displays from original `romfs/gfx` textures. Source logo artwork has 427×108 aspect preserved. |
| Continue | Reloads the actual save file **at selection time**, before reconstructing its world. This also prevents a failed New Game attempt from causing Continue to use a stale in-memory seed. |
| New Game | Requires Novice/Regular/Veteran selection before world creation. Existing save is not overwritten until generation succeeds. |
| Character selection | 20 original source sprite animations, D-pad/shoulder/touch selection and persistent skin preference. The original unlock eligibility rules are **not** all executed yet. |
| Achievements | 33 original localized titles/descriptions. 4-row selection list and scrollable full text view. |
| Character unlocks | 12 original source requirement records: IDs 71–79, 140–142. |
| Weapon unlocks | 6 source requirement records: IDs 392, 393, 394, 416, 418, 420. X switches to weapon requirements in Unlocks. |
| Options | Original `options_menu-sheet0.png` panel with its 128×211 aspect preserved, persistent working sound on/off. |
| Multiplayer | Explicitly says native networking isn't implemented; the multiplayer browser mod is not misrepresented as working. |
| Input/navigation | Bottom touchscreen, A/B, D-pad, L/R, X/Y. Startup input gate rejects forwarded emulator launcher presses. |
| Failure recovery | World generation errors return to a usable menu and display an error. |
| CI | Host menu tests, original source sprite validations, catalog tests, original event-group audit and 3DS compile. |

## Original Construct `Menu_Events` coverage (not inferred from `mp_menu.js`)

The source event sheet has **153 top-level entries** including globals, groups and native actions. Important source groups and their original event UIDs:
- `new_menu` — 5021901739871839; nested `Menu_init`, `Menu_draws`, `Menu_auto_update`
- `character_menu` — 357525716070154
- `Achievement_menu` — 4625067508108619
- `Item unlocks` — 890899747398625
- `Weapon_unlocks` — 928340114546882
- `Language` — 694249778531812; source language tags EN/RU/DE/ES/FR/IT/CZ/PT
- `Animateds` — 6983171607608507

Native C++ reproduces the boot UI workflow as a 320×240 lower-screen adaptation, not as Construct's entire event tree.

## NOT YET SOURCE-IDENTICAL — do not call these features complete

- Original character unlock eligibility, achievements tiers/score gating and persistent progression events have not been ported across all gameplay systems. Displaying text is not an unlock engine.
- Original `Item unlocks` and `Weapon_unlocks` event actions affect gameplay and require their source variables and combat/loot triggers; the menu only displays available weapon unlock descriptions.
- Original eight-language menu/global UI switching isn't implemented. The current generated menu text is from `l_eng_ui.xml`.
- Exact Construct menu transitions, animated elements, drag-and-drop, options and sound effects aren't all reproduced. `mp_menu.js` is primarily a **multiplayer browser DOM override** and is not an equivalent source of vanilla transitions.
- Pixel-level menu identity and real-time performance still need screenshots/video from Manic Emu; CI builds alone can't prove that.

**Criteria for claiming 100% menu parity:** the above source event groups and conditions must produce equivalent state changes, artwork/timings must match reference frames with justified 3DS resizing, and all screens must be tested in an emulator. No unverified claim of full parity should be made.
