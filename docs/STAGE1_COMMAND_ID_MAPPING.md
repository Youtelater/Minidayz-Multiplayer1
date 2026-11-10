# Stage 1 — Construct command-ID resolution blocker

## What the export preserves

The parsed event trees preserve numeric condition and action command IDs, the referenced object-type index, parameters, expressions, and subevent structure. Across all event sheets the audit found **115 distinct condition command IDs** and **206 distinct action command IDs**.

These IDs are not safe to rename by guesswork. The same numeric ID must be interpreted in the context of the referenced Construct object/plugin and, where applicable, its behavior.

## Required metadata

Construct 2's editor-side plugin definitions declare the available actions, conditions, expressions, their editor names, parameter definitions, and display strings. The runtime-side scripts implement the corresponding methods. See the official Construct 2 SDK documentation:

- [Actions, Conditions and Expressions](https://www.construct.net/en/construct-2/manuals/construct-2-javascript-sdk/sdk-documentation/actions-conditions-expressi)
- [ACE implementations](https://www.construct.net/en/construct-2/manuals/construct-2-javascript-sdk/sdk-documentation/ace-implementations)
- [Runtime functions](https://www.construct.net/en/construct-2/manuals/construct-2-javascript-sdk/sdk-documentation/runtime-functions)

The current repository has the generated/minified `c2runtime.js`, but does not contain the original Construct editor-side `edittime.js` definitions for the built-in plugins and behaviors. The project export's numeric IDs alone do not provide the readable ACE labels and parameter schemas.

## Resolution procedure

1. Inventory every distinct `(object/plugin type, command ID)` pair used by the event trees.
2. Obtain the exact matching Construct 2 editor-side plugin and behavior definitions for the runtime version used by this project.
3. Resolve each command ID to its condition/action name, parameter schema, and display string.
4. Preserve the original raw command, parameters, expression tree, object index, and event UID beside the resolved label.
5. Add an automated check that every command reference resolves exactly once; report unknown and ambiguous references instead of guessing.
6. Only then translate the command into C++ behavior and write source-to-C++ traceability tests.

## Status

**Blocked pending exact ACE metadata.** The source event trees and raw IDs are preserved and indexed. No numeric ID is being presented as a human-readable command name until its mapping is verified against the matching editor-side definitions.
