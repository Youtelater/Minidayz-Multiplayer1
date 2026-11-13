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

    // Source-backed item stack limits from the original Check_stack_size event.
    assert(source_max_stack(11) == 50);
    assert(source_max_stack(18) == 3);
    assert(source_max_stack(73) == 5);
    assert(source_max_stack(138) == 40);
    assert(source_max_stack(12) == 60);
    assert(source_max_stack(24) == 10);
    assert(source_max_stack(17) == 30);
    assert(source_max_stack(58) == 80);
    assert(source_max_stack(1) == 1);

    InventorySystem inventory;
    assert(inventory.occupied_slots() == 0);
    assert(inventory.capacity() == kInventorySlotCount);
    assert(!inventory.set_capacity(kMaximumInventorySlotCount + 1));

    // Original bandage stacks cap at three; adding four uses two slots.
    assert(inventory.add(18, 4));
    assert(inventory.count(18) == 4);
    assert(inventory.slot(0)->count == 3);
    assert(inventory.slot(1)->count == 1);
    assert(inventory.occupied_slots() == 2);

    assert(inventory.add(1, 3, 10));
    assert(inventory.count(1) == 3);
    assert(inventory.slot(2)->item_id == 1);
    assert(inventory.slot(2)->count == 3);
    assert(inventory.add(1, 9, 10));
    assert(inventory.count(1) == 12);
    assert(inventory.slot(2)->count == 10);
    assert(inventory.slot(3)->count == 2);

    assert(inventory.remove(0, 2));
    assert(inventory.slot(0)->count == 1);
    assert(!inventory.remove(0, 2));
    assert(!inventory.add(84, 1));
    assert(!inventory.add(0, 1));
    assert(!inventory.add(1, 1, 0) == false); // zero selects the source rule

    // Explicit split, bounded merge, and full-stack swap.
    assert(inventory.move(3, 4, 1, 10));
    assert(inventory.slot(3)->count == 1);
    assert(inventory.slot(4)->count == 1);
    assert(!inventory.move(3, 4, 1, 1));
    assert(inventory.move(3, 4, 1, 10));
    assert(inventory.slot(3)->empty());
    assert(inventory.slot(4)->count == 2);
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

    // Only original wearable backpack IDs can occupy the Backpack equipment slot.
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

    // Replacing a pack returns the old one to the exact source slot.
    assert(backpack.add(451, 1));
    assert(backpack.equip_backpack_from_slot(2, 22));
    assert(backpack.equipped_backpack() == 451);
    assert(backpack.capacity() == 22);
    assert(backpack.slot(2)->item_id == 450);
    assert(backpack.slot(2)->count == 1);

    // Extra occupied slots prevent unequipping; no items are silently hidden.
    assert(backpack.add(18, 1));
    assert(backpack.add(18, 1));
    assert(backpack.add(18, 1));
    assert(backpack.add(18, 1));
    assert(backpack.add(18, 1));
    assert(backpack.add(18, 1));
    assert(backpack.add(18, 1));
    assert(backpack.add(18, 1));
    assert(backpack.add(18, 1));
    assert(backpack.add(18, 1));
    assert(backpack.add(18, 1));
    assert(backpack.add(18, 1));
    assert(backpack.add(18, 1));
    assert(backpack.add(18, 1));
    assert(backpack.add(18, 1));
    assert(backpack.add(18, 1));
    assert(!backpack.unequip_backpack_to_slot(3));
    assert(backpack.capacity() == 22);
    backpack.clear();
    assert(backpack.occupied_slots() == 0);
    assert(backpack.capacity() == kInventorySlotCount);

    // Save snapshots round-trip all slots, capacity, and the equipped backpack.
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

    // Unequipping succeeds once every extended slot is empty.
    InventorySystem removable;
    assert(removable.add(450, 1));
    assert(removable.equip_backpack_from_slot(0, 20));
    assert(removable.add(18, 1));
    assert(removable.unequip_backpack_to_slot(1));
    assert(removable.capacity() == kInventorySlotCount);
    assert(removable.equipped_backpack() == 0);
    assert(removable.slot(1)->item_id == 450);

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
