#include "minidayz/inventory.hpp"
#include "minidayz/generated/item_catalog.hpp"
#include "minidayz/generated/item_source_catalog.hpp"
#include "minidayz/generated/item_sprite_catalog.hpp"

#include <cassert>
#include <cstring>
#include <cstdint>

int main() {
    using namespace minidayz::items;
    static_assert(kInventorySlotCount == 15);
    static_assert(kItemDefinitionCount == 405);
    static_assert(kSourceItemDefinitionCount == 405);
    static_assert(kItemSpriteDefinitionCount > 0);
    assert(kItemDefinitions[0].id == 0);
    assert(kItemDefinitions[1].id == 1);
    assert(kItemDefinitions[1].name[0] != '\0');
    assert(kItemSpriteDefinitionCount == 329);
    assert(source_construct_object(1) != 65535);
    assert(source_sprite_object(1) != 65535);
    assert(source_construct_object(999) == 65535);
    assert(source_sprite_object(0) == 65535);
    for (std::size_t i = 1; i < kSourceItemDefinitionCount; ++i) {
        assert(kSourceItemDefinitions[i - 1].id < kSourceItemDefinitions[i].id);
    }
    for (std::size_t i = 0; i < kItemSpriteDefinitionCount; ++i) {
        const auto item_id = kItemSpriteDefinitions[i].item_id;
        assert(source_sprite_object(item_id) == source_construct_object(item_id));
        assert(source_sprite_object(item_id) != 65535);
        if (i > 0)
            assert(kItemSpriteDefinitions[i - 1].item_id < item_id);
    }

    // Source-backed baseline limits from the original Check_stack_size event.
    assert(source_max_stack(11) == 50);
    assert(source_max_stack(18) == 3);
    assert(source_max_stack(73) == 5);
    assert(source_max_stack(138) == 40);
    assert(source_max_stack(12) == 60);
    assert(source_max_stack(24) == 10);
    assert(source_max_stack(17) == 30);
    assert(source_max_stack(58) == 80);
    assert(source_max_stack(1) == 1);
    assert(source_max_stack(18, true) == UINT16_MAX);
    assert(source_max_stack(1, true) == UINT16_MAX);

    // The original perk raises Check_stack_size to 31,415,926. Ground
    // transfers must accept the same effective cap rather than silently
    // falling back to the non-perk baseline stack limit.
    InventorySystem perk_ground;
    assert(perk_ground.add(18, 4, source_max_stack(18, true)));
    GroundItem perk_drop{};
    assert(!perk_ground.drop(0, 0, perk_drop));
    assert(perk_ground.drop(0, 0, perk_drop, source_max_stack(18, true)));
    assert(perk_drop.count == 4);
    InventorySystem baseline_pickup;
    GroundItem baseline_pile{18, 4, 100};
    assert(baseline_pickup.pickup(baseline_pile));
    assert(baseline_pickup.count(18) == 4);
    assert(baseline_pickup.slot(0)->count == 3);
    assert(baseline_pickup.slot(1)->count == 1);

    InventorySystem perk_pickup;
    GroundItem perk_pile{18, 4, 100};
    assert(perk_pickup.pickup(perk_pile, source_max_stack(18, true)));
    assert(perk_pickup.count(18) == 4);
    assert(perk_pickup.slot(0)->count == 4);

    // Item labels always come from the original root English localization.
    assert(source_item_name(1) != nullptr);
    assert(std::strcmp(source_item_name(1), "Canned Beans") == 0);
    assert(std::strcmp(source_item_name(454), "Improvised Sling Bag") == 0);
    assert(std::strcmp(source_item_name(455), "Improvised Backpack") == 0);
    assert(source_item_name(0) == nullptr);
    assert(source_item_name(65535) == nullptr);

    InventorySystem inventory;
    assert(inventory.occupied_slots() == 0);
    assert(inventory.capacity() == kInventorySlotCount);
    assert(!inventory.set_capacity(kMaximumInventorySlotCount + 1));

    // Original bandages cap at three; four items use two slots.
    assert(inventory.add(18, 4));
    assert(inventory.count(18) == 4);
    assert(inventory.slot(0)->count == 3);
    assert(inventory.slot(1)->count == 1);
    assert(inventory.occupied_slots() == 2);

    // Explicit stack limit can be used by a source-specific override.
    assert(inventory.add(1, 3, 10));
    assert(inventory.add(1, 9, 10));
    assert(inventory.count(1) == 12);
    assert(inventory.slot(2)->count == 10);
    assert(inventory.slot(3)->count == 2);

    assert(inventory.remove(0, 2));
    assert(inventory.slot(0)->count == 1);
    assert(!inventory.remove(0, 2));
    assert(!inventory.add(84, 1));
    assert(!inventory.add(0, 1));
    assert(inventory.add(1, 1)); // source default is one per slot

    // Empty destinations must also respect the source stack limit,
    // including stacks imported with an explicit non-source override.
    InventorySystem stack_guard;
    assert(stack_guard.add(18, 4, 10));
    assert(stack_guard.slot(0)->count == 4);
    assert(!stack_guard.move(0, 1)); // default source cap is three
    assert(stack_guard.slot(0)->count == 4);
    assert(stack_guard.slot(1)->empty());

    // Explicit split, bounded merge, and full-stack swap.
    assert(inventory.move(3, 5, 1, 10));
    assert(inventory.slot(3)->count == 1);
    assert(inventory.slot(5)->count == 1);
    assert(!inventory.move(3, 5, 1, 1));
    assert(inventory.move(3, 5, 1, 10));
    assert(inventory.slot(3)->empty());
    assert(inventory.slot(5)->count == 2);

    assert(inventory.add(18, 1));
    assert(!inventory.move(2, 0, 1, 10));
    assert(inventory.move(2, 0));
    assert(inventory.slot(2)->item_id == 18);
    assert(inventory.slot(0)->item_id == 1);

    assert(!inventory.move(15, 0));
    assert(!inventory.move(0, 0));
    inventory.clear();
    assert(inventory.occupied_slots() == 0);
    assert(inventory.count(1) == 0);

    InventorySystem full;
    for (std::uint16_t id = 1; id <= 15; ++id)
        assert(full.add(id, 1));
    assert(!full.add(18, 1));
    assert(full.occupied_slots() == 15);
    assert(full.count(18) == 0);

    // Original item variable 6 has item-specific defaults in the source map:
    // these three non-wearable IDs start at zero; ordinary items start at 100.
    assert(source_default_condition(1) == 0);
    assert(source_default_condition(75) == 0);
    assert(source_default_condition(320) == 100); // Not an original localized item ID.
    assert(source_default_condition(18) == 100);
    InventorySystem source_condition_defaults;
    assert(source_condition_defaults.add(1, 1));
    assert(source_condition_defaults.slot(0)->condition == 0);
    GroundItem default_condition_ground{1, 1};
    assert(source_condition_defaults.pickup(default_condition_ground));
    assert(source_condition_defaults.slot(1)->condition == 0);

    // Original item variable 6 is condition; it must survive stacking,
    // moving, ground transfers, backpack swaps and save snapshots.
    InventorySystem condition_inventory;
    assert(condition_inventory.add(18, 2, 0, 40));
    assert(condition_inventory.slot(0)->condition == 40);
    assert(condition_inventory.add(18, 1, 0, 100));
    assert(condition_inventory.slot(1)->condition == 100);
    assert(condition_inventory.slot(0)->count == 2);
    assert(!condition_inventory.add_to_slot(0, 18, 1, 0, 100));
    assert(condition_inventory.move(0, 2));
    assert(condition_inventory.slot(2)->condition == 40);
    GroundItem conditioned_ground{};
    assert(condition_inventory.drop(2, 1, conditioned_ground));
    assert(conditioned_ground.condition == 40);
    assert(!condition_inventory.drop(1, 1, conditioned_ground));
    assert(condition_inventory.pickup(conditioned_ground));
    assert(conditioned_ground.empty());
    assert(condition_inventory.slot(2)->condition == 40);

    const auto conditioned_snapshot = condition_inventory.snapshot();
    InventorySystem condition_restored;
    assert(condition_restored.restore(conditioned_snapshot));
    assert(condition_restored.slot(2)->condition == 40);
    assert(condition_restored.slot(1)->condition == 100);
    auto bad_condition_snapshot = conditioned_snapshot;
    bad_condition_snapshot.slots[2].condition = 101;
    assert(!condition_restored.restore(bad_condition_snapshot));

    InventorySystem conditioned_pack;
    assert(conditioned_pack.add(450, 1, 0, 55));
    assert(conditioned_pack.equip_backpack_from_slot(0));
    assert(conditioned_pack.equipped_backpack_condition() == 55);
    auto pack_snapshot = conditioned_pack.snapshot();
    InventorySystem pack_restored;
    assert(pack_restored.restore(pack_snapshot));
    assert(pack_restored.equipped_backpack_condition() == 55);
    assert(pack_restored.unequip_backpack_to_slot(0));
    assert(pack_restored.slot(0)->condition == 55);

    // Exact wearable backpack IDs from the original MDZItemTable.
    assert(is_backpack_item(450));
    assert(is_backpack_item(754));
    assert(!is_backpack_item(456)); // Civilian Tent
    assert(!is_backpack_item(461)); // Supplies Stash
    assert(is_backpack_item(462)); // Wearable fuel reservoir, zero storage slots
    assert(!is_backpack_item(1));
    // The original Items_update function exposes Backpack storage slots 1-11.
    assert(source_backpack_capacity(450) == 20);
    assert(source_backpack_capacity(451) == 22);
    assert(source_backpack_capacity(452) == 26);
    assert(source_backpack_capacity(453) == 18);
    assert(source_backpack_capacity(454) == 18); // 3 original improvised bag slots
    assert(source_backpack_capacity(455) == 20); // 5 original improved bag slots
    assert(source_backpack_capacity(457) == 24);
    assert(source_backpack_capacity(458) == 17);
    assert(source_backpack_capacity(459) == 16);
    assert(source_backpack_capacity(460) == 16);
    assert(source_backpack_capacity(462) == 15);
    assert(source_backpack_capacity(752) == 17);
    assert(source_backpack_capacity(753) == 16);
    assert(source_backpack_capacity(754) == 16);
    assert(source_backpack_capacity(456) == 0);
    assert(source_backpack_capacity(461) == 0);
    assert(source_backpack_capacity(1) == 0);
    const std::uint16_t backpack_ids[] = {
        450, 451, 452, 453, 454, 455, 457, 458, 459, 460, 462, 752, 753, 754
    };
    for (const auto id : backpack_ids) {
        assert(source_construct_object(id) != 65535);
        assert(source_sprite_object(id) != 65535);
    }

    InventorySystem fuel_pack;
    assert(fuel_pack.add(462, 1));
    assert(fuel_pack.equip_backpack_from_slot(0));
    assert(fuel_pack.capacity() == kInventorySlotCount);
    assert(fuel_pack.equipped_backpack() == 462);
    assert(fuel_pack.unequip_backpack_to_slot(0));
    assert(fuel_pack.slot(0)->item_id == 462);

    InventorySystem backpack;
    assert(backpack.add(1, 1));
    assert(backpack.add(450, 1));
    assert(!backpack.equip_backpack_from_slot(0));
    assert(backpack.add(456, 1)); // Civilian Tent is not wearable.
    assert(!backpack.equip_backpack_from_slot(2));
    assert(backpack.remove(2, 1));
    assert(backpack.equip_backpack_from_slot(1));
    assert(backpack.slot(1)->empty());
    assert(backpack.equipped_backpack() == 450);
    assert(backpack.capacity() == 20);
    assert(!backpack.unequip_backpack_to_slot(0)); // target occupied

    // A backpack is one equipment instance, never a multi-count stack.
    InventorySystem stacked_backpack;
    assert(stacked_backpack.add(450, 2, 2));
    assert(!stacked_backpack.equip_backpack_from_slot(0));
    assert(stacked_backpack.count(450) == 2);
    assert(stacked_backpack.capacity() == kInventorySlotCount);
    assert(!stacked_backpack.equip_backpack_from_slot(32));

    // Replacing a pack returns the old pack to the same inventory slot.
    assert(backpack.add(451, 1));
    assert(backpack.equip_backpack_from_slot(1));
    assert(backpack.equipped_backpack() == 451);
    assert(backpack.capacity() == 22);
    assert(backpack.slot(1)->item_id == 450);
    assert(backpack.slot(1)->count == 1);

    // Extended slots block unequip until their contents have been moved out.
    InventorySystem extended;
    assert(extended.add(450, 1));
    assert(extended.equip_backpack_from_slot(0));
    for (std::uint16_t id = 1; id <= 15; ++id)
        assert(extended.add(id, 1));
    assert(extended.add(16, 1));
    assert(extended.slot(15)->item_id == 16);
    assert(!extended.unequip_backpack_to_slot(15)); // backpack storage is not a base destination
    assert(extended.remove(14, 1));
    assert(!extended.unequip_backpack_to_slot(14));
    assert(extended.remove(15, 1));
    assert(extended.unequip_backpack_to_slot(14));
    assert(extended.capacity() == kInventorySlotCount);
    assert(extended.equipped_backpack() == 0);
    assert(extended.slot(14)->item_id == 450);

    // Source tutorial lists smaller capacities for improvised packs. Check
    // expansion, downsizing rejection, rollback and save/restore precisely.
    InventorySystem small_pack;
    assert(small_pack.add(454, 1));
    assert(small_pack.equip_backpack_from_slot(0));
    assert(small_pack.capacity() == 18);
    assert(!small_pack.add_to_slot(18, 1, 1));
    assert(small_pack.add(455, 1));
    assert(small_pack.equip_backpack_from_slot(0));
    assert(small_pack.capacity() == 20);
    assert(small_pack.slot(0)->item_id == 454);
    assert(small_pack.add_to_slot(19, 1, 1));
    assert(!small_pack.equip_backpack_from_slot(0));
    assert(small_pack.capacity() == 20);
    assert(small_pack.equipped_backpack() == 455);
    assert(small_pack.slot(19)->item_id == 1);
    assert(small_pack.remove(19, 1));
    assert(small_pack.equip_backpack_from_slot(0));
    assert(small_pack.capacity() == 18);
    assert(small_pack.equipped_backpack() == 454);
    assert(small_pack.slot(0)->item_id == 455);
    InventorySystem small_restored;
    assert(small_restored.restore(small_pack.snapshot()));
    assert(small_restored.capacity() == 18);

    // Replacing a pack from an extended slot preserves the old pack atomically.
    InventorySystem replacement;
    assert(replacement.add(450, 1, 0, 55));
    assert(replacement.equip_backpack_from_slot(0));
    for (std::uint16_t id = 1; id <= 16; ++id) assert(replacement.add(id, 1));
    assert(replacement.add(451, 1, 0, 65));
    assert(replacement.slot(16)->item_id == 451);
    assert(replacement.equip_backpack_from_slot(16));
    assert(replacement.capacity() == 22);
    assert(replacement.equipped_backpack() == 451);
    assert(replacement.equipped_backpack_condition() == 65);
    assert(replacement.slot(16)->item_id == 450);
    assert(replacement.slot(16)->condition == 55);
    assert(replacement.count(450) == 1);

    // Replacing a backpack from the last extended slot must work even when
    // every other inventory slot is occupied: the outgoing backpack can use
    // the incoming backpack's vacated slot. This is an important no-free-slot
    // atomic replacement edge case.
    InventorySystem full_replacement;
    assert(full_replacement.add(452, 1, 0, 55));
    assert(full_replacement.equip_backpack_from_slot(0));
    for (std::uint16_t id = 1; id <= 25; ++id)
        assert(full_replacement.add(id, 1));
    assert(full_replacement.occupied_slots() == 25);
    assert(full_replacement.add(452, 1, 0, 65));
    assert(full_replacement.slot(25)->item_id == 452);
    assert(full_replacement.occupied_slots() == 26);
    assert(full_replacement.equip_backpack_from_slot(25));
    assert(full_replacement.equipped_backpack() == 452);
    assert(full_replacement.equipped_backpack_condition() == 65);
    assert(full_replacement.slot(25)->item_id == 452);
    assert(full_replacement.slot(25)->condition == 55);
    assert(full_replacement.occupied_slots() == 26);

    // Save snapshots round-trip all slots, capacity, and equipped backpack.
    InventorySystem persisted;
    assert(persisted.add(450, 1));
    assert(persisted.equip_backpack_from_slot(0));
    assert(persisted.add(18, 2));
    const auto saved_inventory = persisted.snapshot();
    InventorySystem restored;
    assert(restored.restore(saved_inventory));
    assert(restored.capacity() == 20);
    assert(restored.equipped_backpack() == 450);
    assert(restored.count(18) == 2);
    auto corrupt_snapshot = saved_inventory;
    corrupt_snapshot.slots[26] = {1, 1};
    assert(!restored.restore(corrupt_snapshot));
    assert(restored.capacity() == 20);
    corrupt_snapshot = saved_inventory;
    corrupt_snapshot.slots[0] = {9999, 1};
    assert(!restored.restore(corrupt_snapshot));
    corrupt_snapshot = saved_inventory;
    corrupt_snapshot.slots[0].count =
        static_cast<std::uint16_t>(source_max_stack(18) + 1);
    assert(!restored.restore(corrupt_snapshot));
    assert(restored.capacity() == 20);
    assert(restored.count(18) == 2);

    // Legacy guessed capacities migrate to the exact source-defined backpack grid.
    auto legacy_pack_save = saved_inventory;
    legacy_pack_save.capacity = 26;
    InventorySystem migrated_pack;
    assert(migrated_pack.restore(legacy_pack_save));
    assert(migrated_pack.capacity() == 20);
    legacy_pack_save.slots[20] = {1, 1}; // old oversized storage position
    assert(migrated_pack.restore(legacy_pack_save));
    assert(migrated_pack.capacity() == 20);
    assert(migrated_pack.count(1) == 1);
    assert(migrated_pack.snapshot().slots[20].empty());
    // The original item-condition grouping must be respected when merging
    // overflow into a surviving partially filled stack.
    auto merge_legacy = legacy_pack_save;
    merge_legacy.slots[0] = {20, 3, 67};
    merge_legacy.slots[20] = {20, 2, 67};
    assert(migrated_pack.restore(merge_legacy));
    assert(migrated_pack.slot(0)->count == 5);
    assert(migrated_pack.slot(0)->condition == 67);
    assert(migrated_pack.snapshot().slots[20].empty());
    assert(migrated_pack.restore(legacy_pack_save));
    // A full surviving grid must reject the old oversized save rather
    // than deleting or overwriting any original item.
    auto overflowing = legacy_pack_save;
    for (std::size_t i = 0; i < 20; ++i)
        overflowing.slots[i] = {1, 1, 100};
    const auto before_failed_migration = migrated_pack.snapshot();
    assert(!migrated_pack.restore(overflowing));
    assert(migrated_pack.snapshot().capacity == before_failed_migration.capacity);
    assert(migrated_pack.count(1) == 1);

    InventorySnapshot stale_empty_expansion{};
    stale_empty_expansion.capacity = 20;
    InventorySystem migrated_empty;
    assert(migrated_empty.restore(stale_empty_expansion));
    assert(migrated_empty.capacity() == kInventorySlotCount);
    stale_empty_expansion.slots[16] = {20, 2, 67};
    assert(migrated_empty.restore(stale_empty_expansion));
    assert(migrated_empty.capacity() == kInventorySlotCount);
    assert(migrated_empty.slot(0)->item_id == 20);
    assert(migrated_empty.slot(0)->count == 2);
    assert(migrated_empty.slot(0)->condition == 67);
    assert(migrated_empty.snapshot().slots[16].empty());

    InventorySystem wear_inventory;
    assert(wear_inventory.add(251, 1, 0, 65));
    assert(wear_inventory.damage_condition(0, 20));
    assert(wear_inventory.slot(0)->condition == 45);
    assert(wear_inventory.damage_condition(0, 45));
    assert(wear_inventory.slot(0)->empty());
    assert(!wear_inventory.damage_condition(0, 1));

    // Native lower-touchscreen half-stack split preserves the exact source
    // item condition and does not mutate the inventory on a failed split.
    InventorySystem split_inventory;
    assert(split_inventory.add_to_slot(0, 20, 5, 0, 72));
    assert(split_inventory.split_half(0, 7));
    assert(split_inventory.slot(0)->count == 3);
    assert(split_inventory.slot(7)->count == 2);
    assert(split_inventory.slot(7)->condition == 72);
    const auto split_before = split_inventory.snapshot();
    assert(!split_inventory.split_half(0, 7));
    assert(!split_inventory.split_half(7, 7));
    assert(!split_inventory.split_half(0, 99));
    assert(split_inventory.slot(0)->count == split_before.slots[0].count);
    assert(split_inventory.slot(7)->count == split_before.slots[7].count);
    assert(split_inventory.split_half(0, 8));
    assert(split_inventory.slot(0)->count == 2);
    assert(split_inventory.slot(8)->count == 1);
    assert(!split_inventory.split_half(8, 9));

    // Ground pickup/drop mirrors the original inventory transfer flow.
    InventorySystem ground_inventory;
    GroundItem ground_pile{18, 4};
    assert(ground_inventory.pickup(ground_pile));
    assert(ground_pile.empty());
    assert(ground_inventory.count(18) == 4);
    assert(ground_inventory.drop(0, 2, ground_pile));
    assert(ground_pile.item_id == 18 && ground_pile.count == 2);
    assert(ground_inventory.slot(0)->count == 1);
    assert(ground_inventory.drop(1, 1, ground_pile));
    assert(ground_pile.count == 3);
    assert(!ground_inventory.drop(0, 1, ground_pile)); // original stack cap
    assert(ground_inventory.slot(0)->count == 1);
    assert(ground_inventory.pickup(ground_pile));
    assert(ground_pile.empty());
    assert(ground_inventory.count(18) == 4);

    InventorySystem no_room;
    for (std::uint16_t id = 1; id <= 15; ++id)
        assert(no_room.add(id, 1));
    GroundItem uncollected{18, 1};
    assert(!no_room.pickup(uncollected));
    assert(uncollected.item_id == 18 && uncollected.count == 1);
    assert(!no_room.drop(0, 1, uncollected)); // different item type
    assert(no_room.slot(0)->item_id == 1);

    // Capacity cannot be fabricated by a caller: only equipping an
    // original wearable backpack grants its source-defined extra slots.
    InventorySystem expanded;
    assert(!expanded.set_capacity(20));
    assert(expanded.capacity() == kInventorySlotCount);
    for (std::uint16_t id = 1; id <= kInventorySlotCount; ++id)
        assert(expanded.add(id, 1));
    assert(!expanded.add(16, 1));
    assert(expanded.occupied_slots() == kInventorySlotCount);
    assert(expanded.set_capacity(kInventorySlotCount));
    expanded.clear();
    assert(expanded.capacity() == kInventorySlotCount);
    assert(expanded.add(450, 1));
    assert(expanded.equip_backpack_from_slot(0));
    assert(!expanded.set_capacity(kInventorySlotCount));
    assert(expanded.set_capacity(20));
    assert(expanded.capacity() == 20);

    // Exact-slot placement is used when returning equipment to the backpack.
    InventorySystem exact_slot;
    assert(exact_slot.add_to_slot(4, 251, 1));
    assert(exact_slot.slot(4)->item_id == 251);
    assert(!exact_slot.add_to_slot(4, 303, 1));
    assert(!exact_slot.add_to_slot(15, 251, 1));
    assert(!exact_slot.add_to_slot(4, 251, 1)); // non-stackable headgear
    assert(exact_slot.add_to_slot(5, 18, 1));
    assert(exact_slot.add_to_slot(5, 18, 1));
    assert(exact_slot.slot(5)->count == 2);
    // Source-driven durability repair accepts only a validated higher value.
    InventorySystem repair_inventory;
    assert(repair_inventory.add(251, 1, 0, 40));
    assert(repair_inventory.restore_condition(0, 75));
    assert(repair_inventory.slot(0)->condition == 75);
    assert(!repair_inventory.restore_condition(0, 75));
    assert(!repair_inventory.restore_condition(0, 39));
    assert(!repair_inventory.restore_condition(0, 101));
    assert(!repair_inventory.restore_condition(15, 100));
    assert(repair_inventory.count(251) == 1);
    assert(repair_inventory.damage_condition(0, 74));
    assert(repair_inventory.slot(0)->condition == 1);
    assert(repair_inventory.damage_condition(0, 1));
    assert(repair_inventory.count(251) == 0);
    assert(!repair_inventory.restore_condition(0, 100));

    // The original backpack repair menu supports duct tape (+15) and epoxy glue (+25).
    InventorySystem backpack_repair;
    assert(backpack_repair.add(450, 1, 0, 40));
    assert(backpack_repair.add(39, 1));
    assert(backpack_repair.add(122, 1));
    assert(backpack_repair.equip_backpack_from_slot(0));
    assert(backpack_repair.repair_equipped_backpack_from_inventory_slot(1));
    assert(backpack_repair.equipped_backpack_condition() == 55);
    assert(backpack_repair.count(39) == 0);
    assert(backpack_repair.repair_equipped_backpack_from_inventory_slot(2));
    assert(backpack_repair.equipped_backpack_condition() == 80);
    assert(backpack_repair.count(122) == 0);
    assert(!backpack_repair.repair_equipped_backpack_from_inventory_slot(2));

    InventorySystem full_backpack_repair;
    assert(full_backpack_repair.add(450, 1, 0, 100));
    assert(full_backpack_repair.add(39, 1));
    assert(full_backpack_repair.equip_backpack_from_slot(0));
    assert(!full_backpack_repair.repair_equipped_backpack_from_inventory_slot(1));
    assert(full_backpack_repair.count(39) == 1);

    InventorySystem wrong_backpack_repair;
    assert(wrong_backpack_repair.add(450, 1, 0, 40));
    assert(wrong_backpack_repair.add(35, 1));
    assert(wrong_backpack_repair.equip_backpack_from_slot(0));
    assert(!wrong_backpack_repair.repair_equipped_backpack_from_inventory_slot(1));
    assert(wrong_backpack_repair.count(35) == 1);

    InventorySystem broken_backpack_repair;
    assert(broken_backpack_repair.add(450, 1, 0, 0));
    assert(broken_backpack_repair.add(39, 1));
    assert(broken_backpack_repair.equip_backpack_from_slot(0));
    assert(!broken_backpack_repair.repair_equipped_backpack_from_inventory_slot(1));
    assert(broken_backpack_repair.count(39) == 1);

    // Original satchel and ordinary backpack coexist. The Army/Medical
    // three exclusive cells enforce exact Construct source item categories.
    InventorySystem dual_pack;
    assert(dual_pack.add(450, 1, 0, 74));
    assert(dual_pack.equip_backpack_from_slot(0));
    assert(dual_pack.capacity() == 20);
    assert(dual_pack.add(458, 1, 0, 81));
    assert(dual_pack.equip_satchel_from_slot(0));
    assert(dual_pack.capacity() == 22);
    assert(dual_pack.equipped_backpack() == 450);
    assert(dual_pack.equipped_satchel() == 458);
    assert(dual_pack.equipped_satchel_condition() == 81);
    assert(dual_pack.add(11, 1));
    auto both_saved = dual_pack.snapshot();
    auto satchel_saved = dual_pack.satchel_snapshot();
    InventorySystem both_restored;
    assert(both_restored.restore_with_satchel(both_saved, satchel_saved));
    assert(both_restored.capacity() == 22);
    assert(both_restored.equipped_backpack() == 450);
    assert(both_restored.equipped_satchel() == 458);
    assert(both_restored.count(11) == 1);
    const auto crafting_rollback = both_restored.snapshot();
    assert(both_restored.restore(crafting_rollback));
    assert(both_restored.equipped_satchel() == 458);
    assert(both_restored.equipped_satchel_condition() == 81);
    assert(both_restored.capacity() == 22);

    // General Satchel slot 21 contains an item invalid for Army's first
    // restricted cell: swapping must reject, never delete cargo.
    assert(both_restored.add(459, 1, 0, 76));
    assert(both_restored.add_to_slot(21, 12, 1));
    assert(!both_restored.equip_satchel_from_slot(1));
    assert(both_restored.equipped_satchel() == 458);
    assert(both_restored.slot(1)->item_id == 459);
    assert(both_restored.slot(21)->item_id == 12);
    assert(both_restored.remove(21, 1));
    assert(both_restored.equip_satchel_from_slot(1));
    assert(both_restored.equipped_satchel() == 459);
    assert(both_restored.capacity() == 24);
    assert(both_restored.slot(1)->item_id == 458);
    assert(!both_restored.is_exclusive_satchel_slot(20));
    for (std::size_t slot = 21; slot < 24; ++slot)
        assert(both_restored.is_exclusive_satchel_slot(slot));
    assert(!both_restored.slot_accepts(21, 18)); // medical only
    assert(both_restored.slot_accepts(21, 24));  // matches, original Army EDC
    assert(!both_restored.add_to_slot(21, 18, 1));
    assert(both_restored.add_to_slot(21, 24, 1));
    assert(both_restored.move(21, 22));
    assert(both_restored.move(22, 21));
    assert(both_restored.slot(21)->item_id == 24);
    const auto army_saved = both_restored.snapshot();
    InventorySystem army_reloaded;
    assert(army_reloaded.restore_with_satchel(
        army_saved, both_restored.satchel_snapshot()));
    assert(army_reloaded.slot(21)->item_id == 24);
    auto corrupt_slots = army_saved;
    corrupt_slots.slots[21] = {18, 1, 100};
    assert(!army_reloaded.restore_with_satchel(
        corrupt_slots, both_restored.satchel_snapshot()));
    assert(army_reloaded.slot(21)->item_id == 24);

    // A Medical Satchel accepts Bandages etc, not Army-only matches.
    assert(both_restored.add(460, 1, 0, 66));
    assert(!both_restored.equip_satchel_from_slot(2));
    assert(both_restored.equipped_satchel() == 459);
    assert(both_restored.slot(21)->item_id == 24);
    assert(both_restored.remove(21, 1));
    assert(both_restored.equip_satchel_from_slot(2));
    assert(both_restored.equipped_satchel() == 460);
    assert(both_restored.capacity() == 24);
    assert(both_restored.slot(2)->item_id == 459);
    assert(both_restored.slot_accepts(21, 18));
    assert(!both_restored.slot_accepts(21, 24));
    assert(both_restored.add_to_slot(21, 18, 1));
    assert(both_restored.move(21, 22));
    assert(both_restored.move(22, 21));
    assert(!both_restored.unequip_satchel_to_slot(3));
    assert(both_restored.remove(21, 1));
    assert(both_restored.unequip_satchel_to_slot(3));
    assert(both_restored.equipped_satchel() == 0);
    assert(both_restored.capacity() == 20);
    assert(both_restored.slot(2)->item_id == 459);
    assert(both_restored.slot(3)->item_id == 460);
    auto invalid_satchel = satchel_saved;
    invalid_satchel.item_id = 461;
    assert(!both_restored.restore_with_satchel(both_saved, invalid_satchel));
    assert(both_restored.equipped_satchel() == 0);

    return 0;
}
