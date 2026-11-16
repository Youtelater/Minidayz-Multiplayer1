#include "minidayz/inventory.hpp"
#include "minidayz/generated/item_catalog.hpp"
#include "minidayz/generated/item_source_catalog.hpp"
#include "minidayz/generated/item_sprite_catalog.hpp"

#include <cassert>
#include <cmath>
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
    assert(!is_backpack_item(1));
    // The original Items_update function exposes Backpack storage slots 1-11.
    assert(source_backpack_capacity(450) == 26);
    assert(source_backpack_capacity(451) == 26);
    assert(source_backpack_capacity(452) == 26);
    assert(source_backpack_capacity(453) == 26);
    assert(source_backpack_capacity(454) == 26);
    assert(source_backpack_capacity(455) == 26);
    assert(source_backpack_capacity(457) == 26);
    assert(source_backpack_capacity(458) == 26);
    assert(source_backpack_capacity(459) == 26);
    assert(source_backpack_capacity(460) == 26);
    assert(source_backpack_capacity(462) == 26);
    assert(source_backpack_capacity(752) == 26);
    assert(source_backpack_capacity(753) == 26);
    assert(source_backpack_capacity(754) == 26);
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
    assert(backpack.capacity() == 26);
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
    assert(backpack.capacity() == 26);
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

    // Every original wearable backpack uses the same 11-slot storage grid.
    // Replacing a pack from an extended slot preserves the old pack atomically.
    InventorySystem replacement;
    assert(replacement.add(450, 1, 0, 55));
    assert(replacement.equip_backpack_from_slot(0));
    for (std::uint16_t id = 1; id <= 16; ++id) assert(replacement.add(id, 1));
    assert(replacement.add(451, 1, 0, 65));
    assert(replacement.slot(16)->item_id == 451);
    assert(replacement.equip_backpack_from_slot(16));
    assert(replacement.capacity() == 26);
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
    assert(full_replacement.add(450, 1, 0, 55));
    assert(full_replacement.equip_backpack_from_slot(0));
    for (std::uint16_t id = 1; id <= 25; ++id)
        assert(full_replacement.add(id, 1));
    assert(full_replacement.occupied_slots() == 25);
    assert(full_replacement.add(451, 1, 0, 65));
    assert(full_replacement.slot(25)->item_id == 451);
    assert(full_replacement.occupied_slots() == 26);
    assert(full_replacement.equip_backpack_from_slot(25));
    assert(full_replacement.equipped_backpack() == 451);
    assert(full_replacement.equipped_backpack_condition() == 65);
    assert(full_replacement.slot(25)->item_id == 450);
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
    assert(restored.capacity() == 26);
    assert(restored.equipped_backpack() == 450);
    assert(restored.count(18) == 2);
    auto corrupt_snapshot = saved_inventory;
    corrupt_snapshot.slots[26] = {1, 1};
    assert(!restored.restore(corrupt_snapshot));
    assert(restored.capacity() == 26);
    corrupt_snapshot = saved_inventory;
    corrupt_snapshot.slots[0] = {9999, 1};
    assert(!restored.restore(corrupt_snapshot));
    corrupt_snapshot = saved_inventory;
    corrupt_snapshot.slots[0].count =
        static_cast<std::uint16_t>(source_max_stack(18) + 1);
    assert(!restored.restore(corrupt_snapshot));
    assert(restored.capacity() == 26);
    assert(restored.count(18) == 2);

    // Legacy guessed capacities migrate to the exact source-defined backpack grid.
    auto legacy_pack_save = saved_inventory;
    legacy_pack_save.capacity = 20;
    InventorySystem migrated_pack;
    assert(migrated_pack.restore(legacy_pack_save));
    assert(migrated_pack.capacity() == 26);
    legacy_pack_save.slots[20] = {1, 1}; // hidden outside the legacy saved capacity
    assert(!migrated_pack.restore(legacy_pack_save));

    InventorySnapshot stale_empty_expansion{};
    stale_empty_expansion.capacity = 20;
    InventorySystem migrated_empty;
    assert(migrated_empty.restore(stale_empty_expansion));
    assert(migrated_empty.capacity() == kInventorySlotCount);

    InventorySystem wear_inventory;
    assert(wear_inventory.add(251, 1, 0, 65));
    assert(wear_inventory.damage_condition(0, 20));
    assert(wear_inventory.slot(0)->condition == 45);
    assert(wear_inventory.damage_condition(0, 0.25f));
    assert(std::fabs(wear_inventory.slot(0)->condition - 44.75f) < 0.0001f);
    GroundItem fractional_ground{};
    assert(wear_inventory.drop(0, 1, fractional_ground));
    assert(std::fabs(fractional_ground.condition - 44.75f) < 0.0001f);
    assert(wear_inventory.pickup(fractional_ground));
    assert(std::fabs(wear_inventory.slot(0)->condition - 44.75f) < 0.0001f);
    assert(wear_inventory.damage_condition(0, 45));
    assert(wear_inventory.slot(0)->empty());
    assert(!wear_inventory.damage_condition(0, 1));

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
    // original wearable backpack grants the source-defined 11 extra slots.
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
    assert(!expanded.set_capacity(20));
    assert(expanded.capacity() == 26);

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

    return 0;
}
