# Stage 1 — Map generation event-tree audit

Source: `Game_events` in the supplied `data.deobfuscated.js` Construct project export.

This audit records the original event structure and raw Construct command references. It does **not** claim that the map-generation logic has been translated into C++.

## Verified scope

- Group name: `Map generation`
- Group UID: `6835885414231505`
- Event nodes including the group marker: **3,062**
- Direct child group declarations: **5**
- Distinct condition object/command pairs in this subtree: **150**
- Distinct action object/command pairs in this subtree: **652**

## Direct child groups

| Child group | Event nodes including child group marker |
|---|---:|
| `Bunker_generate` | 443 |
| `Bridge level` | 63 |
| `Z_spawners` | 54 |
| `MiniMap_legend` | 7 |
| `Location loader` | 1,660 |

## Translation boundary

The source tree includes nested conditions, actions, expressions, object references, and subevents. The object names and numeric command IDs must be resolved against the corresponding Construct plugin/type metadata before their behavior can be faithfully rewritten. The numeric IDs are intentionally preserved rather than guessed into human-readable meanings.

The next semantic pass must trace, at minimum:

- world/map seed and initialization behavior;
- map tile and location loading;
- bunker construction and bunker entry/exit behavior;
- bridge state and bridge-level transitions;
- zombie spawn placement and spawn conditions;
- map/minimap legend and location presentation.

**Status: source group indexed; gameplay semantics and C++ behavior are not yet signed off.**
