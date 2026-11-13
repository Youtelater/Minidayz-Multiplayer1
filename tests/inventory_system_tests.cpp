#include "minidayz/inventory.hpp"
#include "minidayz/generated/item_catalog.hpp"

#include <cassert>
#include <cstdint>

int main() {
    using namespace minidayz::items;
    static_assert(kInventorySlotCount == 15);
    static_assert(kItemDefinitionCount == 405);
    assert(kItemDefinitions[0].id == 0);
    assert(kItemDefinitions[1].id == 1);
    assert(kItemDefinitions[1].name[0] != '\0');

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

    // Empty destinations must also respect the source stack limit.
    assert(inventory.add(18, 1));
    assert(!inventory.move(4, 6, 4)); // four bandages exceed the source cap of three

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

    // Exact wearable backpack IDs from the original MDZItemTable.
    assert(is_backpack_item(450));
    assert(is_backpack_item(754));
    assert(!is_backpack_item(456)); // Civilian Tent
    assert(!is_backpack_item(461)); // Supplies Stash
    assert(!is_backpack_item(1));

    InventorySystem backpack;
    assert(backpack.add(1, 1));
    assert(backpack.add(450, 1));
    assert(!backpack.equip_backpack_from_slot(0, 20));
    assert(!backpack.equip_backpack_from_slot(1, 15));
    assert(backpack.equip_backpack_from_slot(1, 20));
    assert(backpack.slot(1)->empty());
    assert(backpack.equipped_backpack() == 450);
    assert(backpack.capacity() == 20);
    assert(!backpack.unequip_backpack_to_slot(0)); // target occupied

    // Replacing a pack returns the old pack to the same inventory slot.
    assert(backpack.add(451, 1));
    assert(backpack.equip_backpack_from_slot(1, 22));
    assert(backpack.equipped_backpack() == 451);
    assert(backpack.capacity() == 22);
    assert(backpack.slot(1)->item_id == 450);
    assert(backpack.slot(1)->count == 1);

    // Extended slots block unequip until their contents have been moved out.
    InventorySystem extended;
    assert(extended.add(450, 1));
    assert(extended.equip_backpack_from_slot(0, 22));
    for (std::uint16_t id = 1; id <= 15; ++id)
        assert(extended.add(id, 1));
    assert(extended.add(16, 1));
    assert(extended.slot(15)->item_id == 16);
    assert(extended.remove(14, 1));
    assert(!extended.unequip_backpack_to_slot(14));
    assert(extended.remove(15, 1));
    assert(extended.unequip_backpack_to_slot(14));
    assert(extended.capacity() == kInventorySlotCount);
    assert(extended.equipped_backpack() == 0);
    assert(extended.slot(14)->item_id == 450);

    // Replacing a pack from an extended slot can downsize if the old pack
    // has a safe free return slot in the new grid.
    InventorySystem downsize;
    assert(downsize.add(450, 1));
    assert(downsize.equip_backpack_from_slot(0, 22));
    for (std::uint16_t id = 1; id <= 16; ++id) assert(downsize.add(id, 1));
    assert(downsize.add(451, 1));
    assert(downsize.slot(16)->item_id == 451);
    assert(downsize.equip_backpack_from_slot(16, 20));
    assert(downsize.capacity() == 20);
    assert(downsize.equipped_backpack() == 451);
    assert(downsize.count(450) == 1);

    // Save snapshots round-trip all slots, capacity, and equipped backpack.
    InventorySystem persisted;
    assert(persisted.add(450, 1));
    assert(persisted.equip_backpack_from_slot(0, 20));
    assert(persisted.add(18, 2));
    const auto saved_inventory = persisted.snapshot();
    InventorySystem restored;
    assert(restored.restore(saved_inventory));
    assert(restored.capacity() == 20);
    assert(restored.equipped_backpack() == 450);
    assert(restored.count(18) == 2);
    auto corrupt_snapshot = saved_inventory;
    corrupt_snapshot.slots[20] = {1, 1};
    assert(!restored.restore(corrupt_snapshot));
    assert(restored.capacity() == 20);
    corrupt_snapshot = saved_inventory;
    corrupt_snapshot.slots[0] = {9999, 1};
    assert(!restored.restore(corrupt_snapshot));

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

    InventorySystem expanded;
    assert(expanded.set_capacity(20));
    for (std::uint16_t id = 1; id <= 20; ++id) assert(expanded.add(id, 1));
    assert(expanded.occupied_slots() == 20);
    assert(expanded.slot(19)->item_id == 20);
    assert(!expanded.set_capacity(15));
    expanded.clear();
    assert(expanded.set_capacity(15));
    assert(expanded.capacity() == 15);
    return 0;
}
