# Stage 2 #1 — Original loot respawn: source-verified integration status

Authoritative sources: root `data.deobfuscated.js`, `mp_itemtable.js`, and original root localization. Do not use `Minidayz-Multiplayer-main/` or substitute guessed location types, marker positions, or item IDs.

## Implemented and CI-covered

- Original `Check_loot` recurring 1480-second event (UID `2026232979865106`) and save/load preservation (v17/v18).
- Root-source parser `tools/stage2/build_original_timed_loot.py` validates the event ancestry and generates both seed-specific, weighted marker-15 loot choice pools.
- Original `Global_respawn` event (UID `388188595587361`) requires the `Map` layout and `onBridge == 0`.
- Original timed `Trigger_spawn` event (UID `234332254075458`) requires object 218 marker variable 0 == 15 and **distance >1000** from the player.
- Normal `SEED_pubg == 0` original loot path additionally excludes source family 995 and requires `choose(1,2,3,4,5) != 1`; separate SEED_pubg=1 branch has neither restriction.
- The exact source `Spawn_drop` weighted pool and original rounded marker coordinates are used, validated, and atomically inserted into native ground-loot storage.
- Original `Map` editor-marker UIDs and categories are extracted instead of invented; invalid, unknown or out-of-bounds locations are rejected.
- `source/main.cpp` dispatches the original guarded handler only on a real `Check_loot` timer firing.
- Native and devkitARM CI run `tests/original_timed_loot_tests.cpp`, covering timer, distance, source branch gating, item selection, coordinates, failure paths and provenance.

## World integration still required before claiming end-to-end functionality

**The current native `WorldSystem::generate_from_source()` does not yet execute the original `create_map_location` runtime event tree that creates active object 218 loot markers on the playable map.** Existing object-218 placements in the root `Map` layout are 15 source-authoring *editor palette* markers located at negative Y, not active loot locations. Source tracing verified the original `create_map_location` function UID `8627807469654645` has 1193 event nodes, including action calls that create marker 218 via sprite image points, set original marker category through action 218/88, and invoke `Trigger_spawn`.

The native runtime correctly refuses to create fake loot from off-map authoring markers. This means the timer and guarded source drops now work with a verified in-world marker, but the current generated playable world supplies none, so the current build **does not yet produce recurring in-world loot**. Do not label #1 fully completed until the original location-loader action tree (including marker positions/categories and initial marker spawn) is integrated and tested through a full 1480-second cycle.

## Verification

The regression and 3DS build checks show compilation and guard correctness; they do not imply complete in-world source location marker placement or physical-console gameplay equivalence.
