# Source-completeness gate for the Nintendo 3DS port

The target is to implement **all original MiniDAYZ behavior**, not to approximate the game. The authoritative source for original single-player event logic is the root Construct data file data.deobfuscated.js, with c2runtime.js for original runtime semantics; the root assets/gfx and media are image/audio sources. mp_worldseed.js is a multiplayer diagnostic wrapper and must not replace the original generation event rules.

## No original action can silently disappear

Run tools/parity/audit_original_port_coverage.py. It produces:

- every_original_event.jsonl: 22,343 original event nodes, their exact paths, inherited conditions, and original action arrays.
- every_original_action.jsonl.gz: all **63,289 original action occurrences** individually keyed by event path and action ordinal; it preserves source expressions and the inherited condition chains, including seven-element leaf events and duplicated UIDs.
- original_car_loot_event_manifest.json: exact original car-search and loot action ancestry, with variant-specific weighted item choices.
- parity_summary.json: review status and source hash.

The native-action field is **not individually verified** until that action's C++ code path and regression test demonstrate its conditions, timing, RNG, collision, item effects, images, sounds, and save-state behavior. Event discovery, a working generator or a successful 3DS build cannot make an action complete by themselves. The manually verified source-event mapping registry is data/parity/verified_original_events.json.

## Missing behavior already found

**Car trunks**: Original object241 interacts with the player, opening a trunk and setting a visual frame, playing an open sound, rolling trunk_rng, evaluating trunk_bonus and perk_trunkster, and calling frame-dependent weighted Spawn_drop. The original source contains multiple loot action occurrences with inherited random and layout guards. The port's live scene has not yet implemented all those interactions. The new source-backed car loot extractor is tools/stage2/build_original_car_trunk_loot.py; a generated item pool alone does not wire the original event.

**Vehicles**: Driveable cars require separate enter/exit, driving, fuel, repairs, damage, crashes, audio, and persistence. Searchable wrecks and driveable vehicles are distinct mechanics.

**Other source systems**: All original world generation and SetTile effects, vegetation/trees and loot, NPC/zombie/animal AI and drops, weapons/combat/projectiles, crafting, full item-use and inventory, quests/progression, weather/survival, sounds and animation branches, saving and multiplayer must each satisfy their original event chains. Previously corrected multi-building settlements and tree-block hitboxes are only parts of this list.

## Definition of complete

Only mark an original system complete after every source action occurrence has a mapped native handler (or an explicitly justified browser-to-3DS adaptation), source conditions and random choice weights, item and effect side effects, sound timing, exact frames and collision, and deterministic tests. Azahar testing is a separate required visual and interaction gate. Keep Step 4B on hold while original world and car interaction omissions are repaired.
