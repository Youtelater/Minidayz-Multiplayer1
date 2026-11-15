# Original item behavior trace

The source exporter reads the original Construct project snapshot, original
English item localization, and original `MDZItemTable`. It preserves the raw
conditions, actions, expression parameters, parent/child relationships, and
event UIDs for inventory use, pickup, transfer, and drop.

The native inventory port now implements the baseline stack-size groups from
the original `Check_stack_size` function in `data.deobfuscated.js`, including
the original item-ID groups and numeric limits. The `perk_cybb` expression
branch is kept separate until its Construct expression semantics are mapped;
the baseline limits are not presented as the perk-modified limits.

Backpack equipment uses the original dedicated `Backpack` slot and exact
wearable item IDs from `MDZItemTable`. Equipping consumes the backpack from
the selected inventory slot and returns the previous pack to that same slot.
Replacing a pack from a slot that disappears during a capacity reduction now returns the old pack to a free slot in the new grid, and fails atomically if no safe slot exists. Unequipping is refused if shrinking the inventory would strand occupied slots.

The native item-use port now extracts 40 direct, literal stat-effect rows
from the original `item_activate` and `item_activate2` functions. The runtime
applies their source values to hunger, thirst, health, and temperature, consumes
one item only when a matching original Use/Use2 action exists, and clamps the
result through the player rules. The generator intentionally omits conditional
perk bonuses and unrelated player variables until their expression semantics
are implemented.


## CI extraction coverage

The native regression workflow now runs the original event-trace exporter on every branch build. The current Mini DAYZ 1.6.0 snapshot exports the raw inventory behavior records across seven source event groups, including both original item activation functions. The 40 direct item-use stat rows are generated separately and tested natively. Original item condition (instance variable 6) is now carried through inventory, ground, equipment and save/load transfers; stacks with different condition values do not merge. Default values are extracted for 168 item IDs from original map instances (two IDs start at zero); unmapped IDs currently default to 100. The equipment exporter also produces 63 original item property rows, including flat protection and clothing warmth values. The original backpack storage grid is now extracted and validated from the Items_update function (slot indices 1-11); the native inventory model applies the resulting 11-slot extension. Remaining work includes conditional perk bonuses, item-specific secondary effects, source condition wear/repair, and wiring armor/temperature values into the full combat and weather runtime.


## Condition-wear source trace

The exporter also retains the six original combat/obstacle event groups that contain item-condition writes. Their exact group and event UIDs, source action IDs, and unresolved fractional/perk expression semantics are documented in [STAGE2_CONDITION_WEAR_TRACE.md](STAGE2_CONDITION_WEAR_TRACE.md). CI now runs `validate_item_condition_trace.py` after extraction to guard that trace coverage. This is provenance validation only; it does not mark runtime condition wear as implemented.
