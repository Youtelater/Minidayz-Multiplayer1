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

The native regression workflow now runs the original event-trace exporter on every branch build. The current Mini DAYZ 1.6.0 snapshot exports the raw inventory behavior records across seven source event groups, including both original item activation functions. The 40 direct item-use stat rows are generated separately and tested natively. This confirms partial behavior translation, not full gameplay completion: conditional perks, item-specific secondary effects, weapon actions, clothing protection, durability, and backpack capacity still need their original Construct semantics ported and tested.
