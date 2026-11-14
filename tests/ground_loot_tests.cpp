#include "minidayz/ground_loot.hpp"

#include <cassert>
#include <cmath>

using namespace minidayz::items;

int main() {
    InventorySystem inventory;
    GroundLootSystem ground;
    assert(inventory.add(1, 3));
    assert(ground.drop_from_inventory(inventory, 0, 2, 100.0f, 200.0f));
    assert(inventory.count(1) == 1);
    assert(ground.count() == 1);

    const auto* closest = ground.nearest(102.0f, 200.0f, 5.0f);
    assert(closest && closest->item.item_id == 1 && closest->item.count == 2);
    assert(!ground.pickup_nearest(inventory, 110.0f, 200.0f, 5.0f));
    assert(ground.pickup_nearest(inventory, 102.0f, 200.0f, 5.0f));
    assert(ground.count() == 0 && inventory.count(1) == 3);

    // Failed pickup leaves the ground item untouched when inventory is full.
    InventorySystem full;
    for (std::uint16_t id = 1; id <= kInventorySlotCount; ++id)
        assert(full.add(static_cast<std::uint16_t>(id + 100), 1));
    GroundItem medkit{1, 1, kUseSourceItemCondition};
    assert(ground.add(medkit, 4.0f, 5.0f));
    assert(!ground.pickup_nearest(full, 4.0f, 5.0f, 1.0f));
    assert(ground.count() == 1);

    const auto saved = ground.snapshot();
    GroundLootSystem restored;
    assert(restored.restore(saved));
    assert(restored.count() == 1);
    assert(restored.entry(0)->x == 4.0f);
    auto corrupt = saved;
    corrupt.entries[0].x = std::numeric_limits<float>::infinity();
    assert(!restored.restore(corrupt));
    assert(restored.count() == 1);

    // Capacity exhaustion must not remove an inventory item.
    InventorySystem source;
    assert(source.add(1, 64));
    GroundLootSystem crowded;
    for (std::size_t i = 0; i < kMaximumGroundLootItems; ++i)
        assert(crowded.add({1, 1, 100}, static_cast<float>(i), 0.0f));
    assert(!crowded.drop_from_inventory(source, 0, 1, 0.0f, 0.0f));
    assert(source.count(1) == 64);
    return 0;
}
