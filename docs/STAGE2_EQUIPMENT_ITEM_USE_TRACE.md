# Stage 2 — Equipment, item use, and durability source trace

This implementation is based on the original root-level Mini DAYZ sources. It does not use the newer `Minidayz-Multiplayer-main/` directory.

## Item use

The item-effect generator reads the original Construct activation events and emits the native source catalog. It now preserves both direct stat changes and the two conditional perk branches:

- `perk_hamster`: the original true branch adds 10 hunger.
- `perk_camel`: the original true branch adds 10 thirst.

The C++ API accepts `ItemUsePerks` flags representing the already-evaluated original conditions. It does not guess perk levels; the perk progression layer supplies those flags. The generator validates the original conditional expression shape and rejects an unrecognized branch instead of silently omitting it.

## Equipment and protection

- The original equipment property catalog is extracted from item instance variables in the root Construct project.
- Equipped armor is now connected directly to player damage resolution through `EquipmentSystem::apply_damage`.
- Protection remains the original flat helmet/outerwear/vest/pants subtraction. Condition is stored independently; the original hit path does not scale protection by condition.

## Original repair actions

The source events modify the equipped item's condition variable by the listed amount and clamp the result to 100. The repair menu only offers these actions while the target condition is greater than 0 and less than 100. One matching repair material is consumed on a successful repair.

| Original menu action | Target | Material from `l_eng_items.xml` | Item ID | Condition gain |
|---|---|---|---:|---:|
| `pants_tape` | Pants | Duct Tape | 39 | +15 |
| `outerwear_tape` | Outerwear | Duct Tape | 39 | +15 |
| `backpack_tape` | Backpack | Duct Tape | 39 | +15 |
| `vest_tape` | Vest | Duct Tape | 39 | +15 |
| `pants_glue` | Pants | Epoxy Glue | 122 | +25 |
| `outerwear_glue` | Outerwear | Epoxy Glue | 122 | +25 |
| `backpack_glue` | Backpack | Epoxy Glue | 122 | +25 |
| `vest_glue` | Vest | Epoxy Glue | 122 | +25 |
| `Patch_helmet` | Helmet | Sewing Kit | 35 | +50 |
| `Patch_helmet_lite` | Light helmet | Sewing Kit | 35 | +25 |
| `Patch_pants` | Pants | Sewing Kit | 35 | +50 |
| `Patch_outerwear` | Outerwear | Sewing Kit | 35 | +50 |

The generator validates each original menu label, resolves the material ID by its exact English name in the original XML, and extracts the numeric condition gain from the matching Construct action. It emits `item_repair_catalog.hpp`; unsupported or changed source expressions fail the build.

The backpack repair actions update the equipped backpack's own condition in `InventorySystem`. Helmet, outerwear, vest, and pants repairs update the corresponding equipment slot in `EquipmentSystem`. The light-helmet menu action remains distinct because the original source gives it a different repair amount.

## Regression coverage

- Conditional food/drink perk bonuses and unchanged baseline item effects.
- Armor protection applied through the equipment-to-player damage path.
- Every original repair material/target combination.
- Backpack repair, item consumption, full-condition rejection, broken-condition rejection, and 100-condition clamping.
- Inventory/equipment condition preservation and invalid repair rejection.

Build verification is tracked by the GitHub Actions native regression and 3DS executable workflows for the implementation commit.
