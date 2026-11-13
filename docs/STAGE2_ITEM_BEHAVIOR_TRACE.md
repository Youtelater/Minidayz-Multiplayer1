# Original item behavior trace

This Stage 2 source exporter reads only the original single-player Construct
project snapshot, the original English item localization table, and the
original `MDZItemTable` object mapping.

It preserves all source conditions, actions, parent/child relationships, and
event UIDs for these original event groups:

- Using inventory items
- Picking player items
- Picking inventory items
- Droppin items
- inventory_items

It also records every original item ID/name and its exact Construct object
name when present in `MDZItemTable`.

**This is source extraction, not a claim that every effect has been ported.**
The Construct records contain numeric ACE command IDs and raw expressions.
Those must be correlated with the original runtime/plugin command semantics
before converting values into C++ effects. Unknown values remain unknown; do
not add guessed hunger, thirst, healing, armor, temperature, stack, or
backpack-capacity values.

Run:

```sh
python3 tools/stage2/extract_item_behavior_trace.py
```

The output is `data/stage2/item_behavior_trace.json`.
