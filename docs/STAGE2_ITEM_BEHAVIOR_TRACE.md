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
Unequipping is refused if shrinking the inventory would strand occupied slots.

This is a partial native behavior port, not a claim that every item-use effect
or every original inventory event has been translated. Food, medicine, weapon,
clothing, and environmental effects remain tied to the preserved original
event trace until their command semantics are ported.
