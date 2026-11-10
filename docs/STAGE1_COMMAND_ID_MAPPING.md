# Stage 1 — Construct command-ID resolution

## What the original export contains

The exported Construct project preserves the event trees, numeric condition/action IDs, referenced object-type indices, parameters, expression trees, and nested-event structure. The current inventory records 115 distinct condition IDs and 206 distinct action IDs, across 1,120 condition object/command pairs and 4,237 action pairs.

The new original offline package also confirms the presence of the game runtime (`c2runtime.js`) and exported project data (`data.js`). The offline manifest (`offline.js`) is a JSON asset manifest, not gameplay source code. It lists 1,825 package resources, including the runtime, data export, graphics, and audio.

## Important distinction: editor metadata vs. runtime

The game runs in a browser without `edittime.js`. That file belongs to the Construct 2 editor SDK: it describes editor-facing ACE names, parameter types, and display strings. It is not required by the exported game's runtime and its absence is not a blocker to reverse-engineering or porting the game.

The runtime contains the executable implementations and event-dispatch machinery. Therefore, command resolution should proceed from the runtime and the serialized event data. Editor definitions, if ever found, can be used as a cross-check, not as a prerequisite.

## Resolution procedure

1. Preserve each raw event record exactly: sheet, event UID, object-type index, command ID, parameters, expression tree, and subevent nesting.
2. Inspect the exact original `c2runtime.js` shipped with the offline package. Identify the runtime's condition/action dispatch tables and how serialized command IDs select implementations.
3. Correlate runtime dispatch entries with the object/plugin type IDs and command IDs in `ace_reference_inventory.csv`.
4. Recover parameter ordering and expression evaluation from the serialized event records and runtime call sites.
5. Use behavior observed in the browser-exported game as a verification reference where a mapping is ambiguous.
6. Record each mapping with its evidence and confidence. Do not infer a human-readable name from a numeric ID alone.
7. Translate resolved behavior into C++ with source event UIDs and regression tests.

## Current findings

- The original offline package contains `c2runtime.js` (350,716 bytes) and `data.js` (12,442,669 bytes); the GitHub file reader can retrieve the runtime but currently returns empty text for the large `data.js` file.
- The offline manifest contains no `edittime.js` or obvious editor plugin definition entries.
- The offline runtime file and the earlier root runtime copy are not byte-identical (350,716 vs. 350,006 bytes). The offline-package copy should be investigated as the more directly associated runtime, with differences audited before choosing a canonical source.
- The 1,825-entry offline manifest is a resource list, not a substitute for the project data.
- The C++ port must not guess unresolved command semantics.

## Status

**Not blocked on `edittime.js`.** Runtime-based command resolution and behavioral translation remain unfinished. No claim is made that the command IDs have all been resolved or that all events have been ported.
