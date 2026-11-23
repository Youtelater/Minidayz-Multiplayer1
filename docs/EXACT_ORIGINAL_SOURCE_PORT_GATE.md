# Original MiniDAYZ → Nintendo 3DS: evidence-driven completeness gate

The goal is to reconstruct **the original game**, not approximate its appearance. World generation, cars, trees, buildings, inventory, weather, combat, NPCs, audio, UI, and saving are all in scope.

A successful 3DS compilation or one regression test **does not** prove a system is complete. The root `data.deobfuscated.js` is authoritative for Construct behavior; `mp_worldseed.js` and `mp_itemtable.js` are supporting references, not replacements. Original `assets/gfx` and `media` remain the source for art, frame rectangles, collision polygons and sounds.

## Exact port-coverage register

Run `python3 tools/parity/audit_original_port_coverage.py` and `python3 tests/test_original_port_coverage.py`.

The report contains **every original event occurrence** (including duplicate UIDs and seven-field leaf events), its full path, parent IDs, inherited conditions, exact action AST, original sheet/group and fingerprint. Each event is identified by its **sheet ordinal and child-index path**, not UID alone.

Events remain `not_event_verified` until `data/parity/verified_original_events.json` explicitly identifies that precise event with its fingerprint, C++ implementation file, and passing regression test. An object appearing on screen is not evidence that its AI, collision, interactions, sound, loot, animation, or persistence works.

A verified implementation mapping must be reviewed against **all** inherited conditions and action ordering—not just direct children. This strict register starts with no falsely signed-off events. That does **not** mean no C++ systems exist: it means individual event-by-event parity has not been certified.

The CI artifact `exact-source-port-coverage-and-car-loot` contains:
- `every_original_event.jsonl` — exact source event index and implementation status
- `original_car_loot_event_manifest.json` — full original car-open source chains, individual weighted drop choices, probability/variant/perk guards, and world/touch-mode ancestry
- `parity_summary.json` — source hash, per-sheet totals, unverified system groups, and closure gate

## Confirmed missing system: cars

Original car Sprite241 is used in actual interaction events, not merely scenery. The source car-opening tree changes outline 1041 to frame1, awards experience, draws original `trunk_rng`, tests car animation variants (frames 0/1/2/3/4/5), checks original map/tutorial and perks, and can call `Spawn_drop` with source item pools. The source has separate touch and button interaction entry chains. The 3DS runtime currently lacks a certified equivalent car-search/drop sequence. **Do not claim car loot is implemented** because the car PNG or original drop table exists. Exact event manifests are extracted; native state transitions, collision/range guards, rewards and save/reload still require a complete source-grounded implementation and test.

## Exit criteria

No step is complete merely because some objects exist or a build passes. A system closes when all original events affecting it are mapped, with faithful conditions, actions, RNG order, frame/hotspot/collision, loot, audio and state, native regression evidence and later Azahar verification. Unsupported expressions and unported actions must be explicitly identified, never silently guessed or skipped.
