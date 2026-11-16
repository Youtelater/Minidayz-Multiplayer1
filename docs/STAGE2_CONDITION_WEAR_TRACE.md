# Stage 2 — Original item-condition event trace

The Stage 2 exporter now includes the original Construct event groups that can
write the held-item condition field, in addition to inventory, pickup, drop and
item-activation events. The inputs remain the root Mini DAYZ 1.6.0 export and
its parsed Construct snapshot; the newer `Minidayz-Multiplayer-main/` directory
is not used.

## Source groups retained

| Event group | Source UID | Condition-write event UID |
|---|---:|---:|
| Firearm shooting | 6904772320561696 | 875054325568454 |
| Alternate attacks | 337954396197555 | 875054325568454 |
| Melee fight | 3528035038609865 | 2098275646157213 |
| Damage dealing | 811368909310732 | 251602875520018 |
| Player damaging | 9520121190252054 | 682686209618980 |
| Hitting_obstacles | 5836740205504893 | 8036449108462062 |

The source actions include writes targeting object 1030, instance variable 6,
using Construct action IDs 87 and 88. Their expressions are not all simple
integer decrements: the original data includes fractional values (for example
0.2, 0.25, 0.4 and 0.5), item-ID-specific branches, and conditions involving
`perk_wxnj`. Other events derive values from source expressions rather than
literal constants.

## Verification

`tools/stage2/validate_item_condition_trace.py` checks that all six named
source groups and representative condition-write event UIDs are present in the
generated raw trace. The native regression workflow runs this validator
immediately after `extract_item_behavior_trace.py`.

This validator is a source-provenance guard only. It does **not** claim that the
condition-write actions have been implemented in gameplay. The native inventory, equipment, backpack, and ground-item condition fields now
use floating-point values, so fractional source wear is retained through stack
moves, equipment swaps, ground transfers, snapshots, and save version 6. Save
versions 1-5 are migrated; v5's byte-sized condition fields are converted
without changing their existing values. Regression tests cover fractional wear
and ground transfer. The event-to-runtime wiring remains open: each source
branch still needs its exact item-ID predicate, action timing, and perk_wxnj
evaluation connected to the matching firearm, melee, player-hit, and obstacle
actions. This change is the storage prerequisite, not a claim that those combat
events are now active in gameplay.
