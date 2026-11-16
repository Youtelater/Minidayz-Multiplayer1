#include "minidayz/tent_storage.hpp"
#include <cassert>
#include <limits>
using namespace minidayz::items;

static bool placement_blocked(void*, float x, float y) noexcept {
    return x < 0.0f || y < 0.0f || (x >= 90.0f && x < 110.0f);
}

int main() {
    assert(source_item_exists(kOriginalCivilianTentItemId));
    InventorySystem inv;
    TentStorageSystem tents;
    assert(inv.add(456, 2, 0, 73));
    assert(!tents.deploy_from_inventory(inv, 100, 10, 20));
    assert(!tents.deploy_from_inventory(inv, 0,
           std::numeric_limits<float>::quiet_NaN(), 20));
    assert(inv.count(456) == 2);
    assert(tents.deploy_from_inventory(inv, 0, 10, 20));
    assert(inv.count(456) == 1);
    assert(tents.tent_count() == 1);
    assert(tents.tent(0)->condition == 73);
    assert(tents.nearest(10, 21, 5) == 0);
    assert(tents.nearest(50, 20, 5) == 1);
    assert(inv.add(18, 3, 0, 62));
    assert(inv.add(18, 2, 0, 81));
    assert(!tents.store_from_inventory(inv, 100, 0));
    assert(!tents.store_from_inventory(inv, 1, 100));
    assert(tents.store_from_inventory(inv, 0, 0));
    assert(inv.count(18) == 2);
    assert(tents.store_from_inventory(inv, 2, 0));
    assert(tents.tent(0)->items[0].item_id == 18);
    assert(tents.tent(0)->items[0].count == 3);
    assert(tents.tent(0)->items[0].condition == 62);
    assert(tents.tent(0)->items[1].count == 2);
    assert(tents.tent(0)->items[1].condition == 81);
    assert(tents.take_to_inventory(inv, 0, 0));
    assert(inv.count(18) == 3);
    assert(!tents.take_to_inventory(inv, 0, 0));
    assert(tents.tent(0)->items[0].empty());

    auto saved = tents.snapshot();
    TentStorageSystem restored;
    assert(restored.restore(saved));
    assert(restored.tent_count() == 1);
    assert(restored.tent(0)->items[1].count == 2);
    assert(restored.tent(0)->items[1].condition == 81);
    auto corrupt = saved;
    corrupt.tents[0].items[1].count = 65535;
    assert(!restored.restore(corrupt));
    assert(restored.tent(0)->items[1].count == 2);
    corrupt = saved;
    corrupt.tents[1].items[0] = {1, 1, 100};
    assert(!restored.restore(corrupt));
    corrupt = saved;
    corrupt.tents[0].x = std::numeric_limits<float>::infinity();
    assert(!restored.restore(corrupt));

    // A full backpack must not destroy stored items when withdrawing.
    InventorySystem full;
    for (std::uint16_t id = 1; id <= kInventorySlotCount; ++id)
        assert(full.add(1, 1, 1, static_cast<std::uint8_t>(id)));
    assert(!restored.take_to_inventory(full, 0, 1));
    assert(restored.tent(0)->items[1].count == 2);

    // An item stack with a different condition must never be merged.
    InventorySystem more;
    assert(more.add(18, 3, 0, 40));
    assert(restored.store_from_inventory(more, 0, 0));
    assert(restored.tent(0)->items[0].condition == 40);
    assert(restored.tent(0)->items[1].condition == 81);

    // Deployment limit is a native resource bound, not a source gameplay cap.
    InventorySystem supply;
    assert(supply.add(456, static_cast<std::uint16_t>(kNativeMaximumDeployedTents)));
    for (std::size_t i = 1; i < kNativeMaximumDeployedTents; ++i)
        assert(restored.deploy_from_inventory(supply, i - 1, float(i), 0));
    assert(restored.tent_count() == kNativeMaximumDeployedTents);
    assert(!restored.deploy_from_inventory(supply, 0, 100, 0));
    // In the actual 3DS scene, tent placement uses a collision callback
    // and an overlap check. A failed check must preserve the inventory.
    InventorySystem placement_inventory;
    TentStorageSystem placement_tents;
    assert(placement_inventory.add(456, 3, 0, 57));
    assert(!placement_tents.deploy_from_inventory_checked(
        placement_inventory, 0, 98.0f, 40.0f,
        &placement_blocked, nullptr));
    assert(placement_inventory.count(456) == 3);
    assert(!placement_tents.deploy_from_inventory_checked(
        placement_inventory, 0, 2.0f, 3.0f,
        &placement_blocked, nullptr)); // corner probe crosses boundary
    assert(placement_inventory.count(456) == 3);
    assert(placement_tents.deploy_from_inventory_checked(
        placement_inventory, 0, 50.0f, 50.0f,
        &placement_blocked, nullptr));
    assert(placement_tents.tent(0)->condition == 57);
    // Original tent stacks have max count one. Slot 0 was consumed by the
    // first deployment; use the still-occupied second slot for this test.
    assert(!placement_tents.deploy_from_inventory_checked(
        placement_inventory, 1, 54.0f, 54.0f,
        &placement_blocked, nullptr)); // overlaps the first tent
    assert(placement_inventory.count(456) == 2);
    assert(placement_tents.deploy_from_inventory_checked(
        placement_inventory, 1, 70.0f, 70.0f,
        &placement_blocked, nullptr));
    assert(placement_inventory.count(456) == 1);
    return 0;
}
