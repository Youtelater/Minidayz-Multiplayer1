# Stage 1 — Construct command-ID resolution

## Authoritative original files

The authoritative Mini DAYZ 1.6.0 source set is the repository root. The structured Construct export used by the extraction pipeline is root-level `data.deobfuscated.js`; the original packaged data and runtime are root-level `data.js` and `c2runtime.js`. The root offline resource manifest is `offline.js`. Original item names, item sprite mappings, localization, and artwork are also taken only from root-level `l_eng_*.xml`, `mp_itemtable.js`, and asset directories.

The `Minidayz-Multiplayer-main/` directory contains a separate newer version and is explicitly excluded from command-ID research, source extraction, catalogs, runtime behavior, and assets. Its `data.js`, `c2runtime.js`, and `offline.js` must not be used to fill gaps in the root source.

## What the original export contains

The root Construct export preserves event trees, numeric condition/action IDs, referenced object-type indices, parameters, expression trees, and nested-event structure. The current inventory records 115 distinct condition IDs and 206 distinct action IDs, across 1,120 condition object/command pairs and 4,237 action pairs.

## Editor metadata vs. runtime

The game runs in a browser without `edittime.js`. That file belongs to the Construct 2 editor SDK: it describes editor-facing ACE names, parameter types, and display strings. It is not required by the exported game's runtime. The root `c2runtime.js` contains the runtime dispatch machinery; the serialized event data in root `data.deobfuscated.js` is the authoritative record of which commands and expressions the game uses.

## Resolution procedure

1. Preserve each raw event record exactly: sheet, event UID, object-type index, command ID, parameters, expression tree, and subevent nesting.
2. Inspect the root-level `c2runtime.js` and identify the runtime's condition/action dispatch tables and how serialized command IDs select implementations.
3. Correlate runtime dispatch entries with the object/plugin type IDs and command IDs in the generated ACE reference inventory.
4. Recover parameter ordering and expression evaluation from root runtime call sites and serialized event records.
5. Use browser behavior from this same original version as a verification reference where a mapping is ambiguous.
6. Record each mapping with its evidence and confidence. Do not infer a human-readable name from a numeric ID alone.
7. Translate resolved behavior into C++ with source event UIDs and regression tests.

## Status

**Not blocked on `edittime.js`.** Runtime-based command resolution and behavioral translation remain unfinished. No claim is made that all command IDs have been resolved or all events ported.
