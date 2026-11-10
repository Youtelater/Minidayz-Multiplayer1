# System 2 — Player system source audit

## Source inspected

The authoritative source is the preserved Construct project snapshot under
`data/stage1/source_snapshot/`, specifically the `Game_events` event sheet.

- The top-level `Player stats` group has UID `7463747062628312` and 196 direct child entries.
- It contains the `colchecker` subgroup (UID `2317094210126576`, 3 direct children).
- It contains the `States` subgroup (UID `6096691100830449`, 22 direct children).
- The group contains hundreds of condition/action references. Their numeric IDs remain unresolved until the exact Construct 2 ACE metadata is available; see `docs/STAGE1_COMMAND_ID_MAPPING.md`.

## Source-backed player data

The Game_events sheet declares these player/survival variables (initial values shown in the source export):

| Variable | Initial value | Use indicated by source name |
|---|---:|---|
| `Player_temperature` | 0 | Player body temperature |
| `Weather_temperature` | 0 | Ambient temperature |
| `Rain_cold` | 0 | Rain cooling modifier |
| `Night_cold` | 0 | Night cooling modifier |
| `Day_cold` | 0 | Day cooling modifier |
| `Rain` | 0 | Rain state |
| `Starving_current` | 0 | Difficulty-adjusted starvation rate |
| `Thirsty_current` | 0 | Difficulty-adjusted thirst rate |
| `Regeneration_current` | 0 | Difficulty-adjusted regeneration rate |
| `Weathercold_current` | 0 | Difficulty-adjusted cold rate |
| `Veteran_starving` / `Regular_starving` / `Novice_starving` | 1 / 0.75 / 0.5 | Difficulty modifiers |
| `Veteran_thirsty` / `Regular_thirsty` / `Novice_thirsty` | 1 / 0.75 / 0.5 | Difficulty modifiers |
| `Veteran_weathercold` / `Regular_weathercold` / `Novice_weathercold` | 0 / 0 / 0 | Difficulty modifiers |
| `Veteran_regeneration` / `Regular_regeneration` / `Novice_regeneration` | 1.5 / 1.75 / 2 | Difficulty modifiers |
| `Stats_food_eaten` | 0 | Food statistic |
| `Stats_water_drinked` | 0 | Water statistic |
| `Stats_blood_lost` | 0 | Blood-loss statistic |
| `Death_log_reason` | `None` | Death cause |
| `GUI_control_type` | 0 | Control mode |
| `GUI_dpad_onscreen` | 0 | On-screen directional pad |
| `run_touch_id`, `stick_touch_id`, `force_stick`, `stick_on`, `stick_pos_X`, `stick_pos_Y` | 0 | Touch movement controls |

The source also has player-related objects/layers for the player body, clothing,
vests, helmets, backpacks, weapons and the survivor sprite sets.

## C++ work in this commit

- Adds `PlayerState` for position, health, hunger, thirst, temperature, bleeding,
  infection, facing and alive/dead state.
- Adds configurable movement with diagonal normalization and a collision-query
  callback, so the exact collision geometry can be supplied by the world system.
- Adds explicit operations for damage, healing and survival-stat changes.
- Keeps game-specific speeds, maxima and rates configurable rather than
  inventing values that are not yet resolved from the original event logic.

## Not yet source-equivalent

This is the first native player-system foundation, not a completed player port.
The following still require ACE resolution and event-by-event translation:

1. Exact movement modes, acceleration/speed values, animation transitions and touch-control behavior.
2. Exact collision shapes, solid-object filtering, collision response and interaction with vehicles/buildings.
3. Health limits, damage sources, regeneration and death transitions.
4. Hunger/thirst drain timing, consumption effects and difficulty/perk modifiers.
5. Temperature calculation from weather, rain, day/night, clothing and equipment.
6. Bleeding severity, blood loss, bandage/healing rules and infection mechanics.
7. Source-to-C++ UID traceability and regression tests against the original.

No multiplayer JSON is used. Original media assets are not modified.
