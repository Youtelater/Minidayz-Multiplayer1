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

    InventorySystem inventory;
    assert(inventory.occupied_slots() == 0);
    assert(inventory.capacity() == kInventorySlotCount);
    assert(!inventory.set_capacity(kMaximumInventorySlotCount + 1));
    assert(inventory.add(1, 3, 10));
    assert(inventory.count(1) == 3);
    assert(inventory.slot(0)->item_id == 1);
    assert(inventory.slot(0)->count == 3);

    // Existing stacks fill first; overflow occupies another slot.
    assert(inventory.add(1, 9, 10));
    assert(inventory.count(1) == 12);
    assert(inventory.slot(0)->count == 10);
    assert(inventory.slot(1)->count == 2);
    assert(inventory.occupied_slots() == 2);

    assert(inventory.add(18, 1));
    assert(inventory.slot(2)->item_id == 18);
    assert(inventory.remove(2, 1));
    assert(inventory.slot(2)->empty());
    assert(!inventory.remove(2, 1));
    assert(!inventory.add(84, 1));
    assert(!inventory.add(0, 1));
    assert(!inventory.add(1, 1, 0));

    assert(inventory.move(1, 3));
    assert(inventory.slot(1)->empty());
    assert(inventory.slot(3)->item_id == 1);
    assert(inventory.slot(3)->count == 2);
    assert(inventory.move(0, 3, 5));
    assert(inventory.slot(0)->count == 5);
    assert(inventory.slot(3)->count == 7);

    assert(inventory.add(18, 2, 10));
    // A partial stack cannot merge into a different item type; moving the
    // complete stack swaps the two occupied slots.
    assert(!inventory.move(0, 1, 1));
    assert(inventory.move(0, 1));
    assert(inventory.slot(0)->item_id == 18);
    assert(inventory.slot(0)->count == 2);
    assert(inventory.slot(1)->item_id == 1);
    assert(inventory.slot(1)->count == 5);

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

    InventorySystem expanded;
    assert(expanded.set_capacity(20));
    for (std::uint16_t id = 1; id <= 20; ++id) assert(expanded.add(id, 1));
    assert(expanded.occupied_slots() == 20);
    assert(expanded.slot(19)->item_id == 20);
    assert(!expanded.set_capacity(15)); // shrinking never hides occupied items
    expanded.clear();
    assert(expanded.set_capacity(15));
    assert(expanded.capacity() == 15);
    return 0;
}
