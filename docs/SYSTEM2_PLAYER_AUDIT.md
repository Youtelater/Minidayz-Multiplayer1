# System 2 — Player system implementation

## Implemented native module

- `include/minidayz/player.hpp` defines player state, directional facing, locomotion animation state, movement input, configurable rules, collision callback and survival tick results.
- `source/player.cpp` implements normalized 8-way movement, axis-separated collision checks (wall sliding), facing and idle/walk/run/dead state transitions.
- Survival ticking applies configurable hunger/thirst drain, starvation/dehydration damage, bleeding/infection damage, health regeneration and temperature change. Damage is clamped at zero and transitions the player to dead.
- Healing and consumable-stat adjustments are bounded by caller-supplied caps.
- The collision callback is the seam for the World system; the player module does not invent world geometry.

## Source fidelity and unresolved values

The source audit identifies the Construct `Game_events` sheet's `Player stats` group (UID `7463747062628312`), its `colchecker` and `States` subgroups, and variables including `Player_temperature`, `Weather_temperature`, `Rain_cold`, `Night_cold`, `Day_cold`, `Starving_current`, `Thirsty_current`, and `Regeneration_current`.

The source audit also records difficulty modifiers:
- Starvation: Veteran 1, Regular 0.75, Novice 0.5.
- Thirst: Veteran 1, Regular 0.75, Novice 0.5.
- Regeneration: Veteran 1.5, Regular 1.75, Novice 2.
- Weather-cold modifiers: all three recorded as 0 in the export.

The exact ACE command semantics and event timing are not yet resolved. Accordingly, this module does **not** hard-code guessed Mini DAYZ speeds, stat maxima, drain rates, temperature thresholds, damage rates, or regeneration values. Populate `PlayerRules` only from the completed event/ACE trace. The temperature rate is an injected net rate; the World/weather layer must calculate it from source-equivalent weather, time-of-day and equipment rules.

## Integration boundary

The module is compiled automatically by the repository Makefile because `source/player.cpp` is in `source/`. It is currently a standalone gameplay subsystem; the engine still has no active gameplay scene, so this commit does not claim a playable character on screen. The next integration pass connects the player to the World collision provider, game scene, HUD and source-mapped sprite animations.

## Verification status

- C++ source is written for the repository's C++17 / `-fno-rtti` build.
- Target build and hardware/emulator behavior must be confirmed by the GitHub Actions build and a 3DS runtime test; this document does not claim either has passed.
