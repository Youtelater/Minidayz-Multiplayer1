# System 2 — Player system implementation

## Implemented native player module

- `include/minidayz/player.hpp` and `source/player.cpp` define player position, health, hunger, thirst, temperature, bleeding, infection, facing, life/death state, movement input, and configurable rules.
- Movement supports normalized 8-way input, separate walk/run speeds, configurable player collision half-extents, axis-separated collision checks, and wall sliding.
- Inputs and state values are guarded against non-finite values so NaN/Inf values cannot corrupt player coordinates or stats.
- Damage, healing, consumable stat adjustments, bleeding/infection setters, and survival ticks are bounded by supplied rules.
- Hunger, thirst, bleeding, infection, regeneration, and temperature effects run through the player update path. The rates are still supplied by the source-equivalence layer; unverified rates remain zero rather than being guessed.
- Death records the explicit cause supplied by the event dispatcher. Hunger, thirst, cold, and unclassified causes are represented.

## Original player animation integration

The extracted source snapshot identifies `t679` / `player_skin_def_2` (object index 679) as the default player skin. Its `data/stage1/animations.csv` animation order is:

- `idle_down/up/right/left`: animation indices 0–3.
- `run_down/up/right/left`: animation indices 4–7, four looping frames each.
- The default skin has no dedicated `on_ground` / death animation; the scene shows a diagnostic dead marker after death rather than assigning an unrelated animation.
- The source inventory has no `walk_*` animation for this actor. The current renderer therefore uses the matching `run_*` directional sprite for both movement speeds rather than inventing a walk animation.
- The source animation speed field is 1.0; the native player animation timer uses that extracted speed for the four-frame run cycle.

The startup preview now draws the original source player sprite through `Renderer::draw_source_sprite`, falls back to a diagnostic marker if an atlas is missing, and clears stale sprite-cache entries when a sheet is evicted.

## Scene and controls integration

- The active startup scene now updates the PlayerSystem every frame and displays the source player actor on the top-screen world preview.
- Circle Pad and D-pad movement are combined and normalized by PlayerSystem.
- The B button selects the preview run speed.
- The bottom screen displays live health, hunger, thirst, and temperature meters above the existing equipment/backpack placeholder grid.
- The scene currently uses a preview boundary collision callback. It is not yet connected to source polygon/building collision geometry.
- The preview movement speeds and 0–100 meter caps are explicitly temporary presentation values, not claims about original Mini DAYZ tuning.

## Source fidelity — still unresolved

The source trace identifies the Construct `Game_events` sheet's `Player stats` group (UID `7463747062628312`), its `colchecker` and `States` subgroups, and variables including `Player_temperature`, `Weather_temperature`, `Rain_cold`, `Night_cold`, `Day_cold`, `Starving_current`, `Thirsty_current`, `Regeneration_current`, and `Stats_blood_lost`.

The trace records difficulty modifiers:
- Starvation: Veteran 1, Regular 0.75, Novice 0.5.
- Thirst: Veteran 1, Regular 0.75, Novice 0.5.
- Regeneration: Veteran 1.5, Regular 1.75, Novice 2.
- Weather-cold modifiers: all three recorded as 0 in the export.

The event tree contains expressions with `0.1` for hunger/thirst changes and an expression adding `2` to player temperature. These are not yet proven per-second rates. Trigger cadence, ACE command semantics, branch conditions, caps, and full cold/death logic remain unresolved. Do not copy these expressions into a timed C++ simulation until their event cadence is established.

Still open:
1. Complete the source event/ACE trace for movement speed, collision checks, hunger/thirst cadence, temperature/weather/equipment, blood loss, infection, and regeneration.
2. Translate the original `colchecker` behavior and connect player collision to world polygons/building geometry.
3. Connect the remaining player equipment layers (pants, hands, clothing, vest, helmet, backpack, weapon) to the original layered render order.
4. Add source-derived regression fixtures for the remaining event branches and validate behavior against the original.
5. Run the 3DS build and test the player controls, sprite atlas loading, and HUD on emulator/hardware.

## Regression coverage

`tests/player_system_tests.cpp` covers:
- Hunger and thirst death causes.
- Generic and explicitly classified damage.
- Diagonal movement and wall sliding.
- Source animation index selection, frame progression, and death pose.
- Collision half-extents.
- Non-finite movement input.
- Survival tick math using injected test rates.
- Stat setter clamping.

The host test workflow in `.github/workflows/player-tests.yml` compiles with C++17, `-Wall -Wextra -Werror -pedantic`, then runs the test executable.

## Status

**Player runtime integration is implemented, but the Player system is not yet source-faithful or complete.** The remaining work is specifically the unresolved original event semantics, real world collision, and layered equipment rendering listed above. The preview-only movement values are not to be treated as final game tuning.
