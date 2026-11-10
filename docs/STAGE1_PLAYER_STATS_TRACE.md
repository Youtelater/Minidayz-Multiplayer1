# Player stats — source event trace (Stage 1)

## Scope and provenance

Source: stored parsed Construct project snapshot, sheet `Game_events`, group `Player stats` (UID `7463747062628312`). No files from the restricted `Minidayz-Multiplayer-main/` folder were used. No `mp.json`, `raw/`, `romfs/`, or `c2injt.js` sources were used.

The group contains 732 nested event nodes, including child groups. Its `States` subgroup (UID `6096691100830449`) contains 199 nested event nodes; `colchecker` (UID `2317094210126576`) contains 17. The serialized event records preserve numeric condition/action IDs and raw parameter expressions; no command IDs are assigned guessed names.

## Original variables

The original `Game_events` sheet declares player-related variables including `Player_temperature`, `Weather_temperature`, `Rain_cold`, `Night_cold`, `Day_cold`, `Starving_current`, `Thirsty_current`, `Regeneration_current`, `Stats_blood_lost`, `Death_log_reason`, `Difficulty`, and perk flags such as `perk_camel`, `perk_hamster`, `perk_shengbing`, and `perk_regen`.

## Direct observations from the event tree

1. A branch writes `Thirsty_current` using an expression containing `0.1`; a nested branch checking `perk_camel` applies a further `0.1` adjustment.
2. A parallel branch writes `Starving_current` using an expression containing `0.1`; a nested branch checking `perk_hamster` applies a further `0.1` adjustment.
3. One event path writes `Player_temperature` using an expression that adds `2` to its current value.
4. Other branches reference `Stats_blood_lost`, `Regeneration_current`, player-stat comparisons, perk flags, and `rad_phone`.
5. Separate event paths set `Death_log_reason` to `Hunger`, `Thirst`, and `Cold`.

These are observations of serialized event expressions and variable references, **not yet per-second rates**. Trigger cadence, command semantics, caps, and full branch context must be resolved before using constants in C++.

## Current C++ comparison

`include/minidayz/player.hpp` and `source/player.cpp` provide a generic player/survival scaffold. Numeric `PlayerRules` values default to zero and are injected by callers. The scaffold does not yet implement this original event tree. This avoids inventing drain rates, thresholds, damage, or temperature rules.

## Next translation work

- Correlate raw condition/action IDs with original runtime dispatch and implementations.
- Resolve parameter/expression formats for `States` and `colchecker`.
- Translate each verified branch into named C++ rules, preserving source event UIDs.
- Add regression tests for perk modifiers, hunger/thirst, temperature, bleeding, regeneration, and death reasons.
- Populate `PlayerRules` only with source-verified values, then integrate with world and HUD.

## Status

This is a first source-grounded semantic trace of the Player stats group, not a completed player-system port or Stage 1 sign-off.
