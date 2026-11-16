#include "minidayz/ground_loot.hpp"
#include "minidayz/loot_source.hpp"

#include <cassert>
#include <cmath>
#include <limits>

using namespace minidayz::items;

int main() {
    InventorySystem inventory;
    GroundLootSystem ground;
    assert(inventory.add(11, 3, 50));
    assert(ground.drop_from_inventory(inventory, 0, 2, 100.0f, 200.0f));
    assert(inventory.count(11) == 1);
    assert(ground.count() == 1);

    const auto* closest = ground.nearest(102.0f, 200.0f, 5.0f);
    assert(closest && closest->item.item_id == 11 && closest->item.count == 2);
    assert(!ground.pickup_nearest(inventory, 110.0f, 200.0f, 5.0f));
    assert(ground.pickup_nearest(inventory, 102.0f, 200.0f, 5.0f));
    assert(ground.count() == 0 && inventory.count(11) == 3);

    // Failed pickup leaves the ground item untouched when inventory is full.
    InventorySystem full;
    for (std::uint16_t id = 1; id <= kInventorySlotCount; ++id)
        assert(full.add(1, 1, 1, static_cast<std::uint8_t>(id)));
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
    assert(source.add(11, 64, 100));
    GroundLootSystem crowded;
    for (std::size_t i = 0; i < kMaximumGroundLootItems; ++i)
        assert(crowded.add({1, 1, 100}, static_cast<float>(i), 0.0f));
    assert(!crowded.drop_from_inventory(source, 0, 1, 0.0f, 0.0f));
    assert(source.count(11) == 64);
    // Exact source candidate selection is available only after event guards
    // are checked by its caller. The drop itself must remain atomic.
    const auto* original = source_loot_pool(0);
    assert(original && original->choices > 0);
    GroundLootSystem spawned;
    assert(spawned.spawn_verified_source_choice(0, 0, 7.0f, 8.0f));
    assert(spawned.count() == 1);
    assert(spawned.entry(0)->item.item_id == source_loot_choice(0, 0));
    assert(spawned.entry(0)->item.count == 1);
    assert(spawned.spawn_verified_source_choice(0, UINT32_MAX, 9.0f, 10.0f));
    assert(spawned.entry(1)->item.item_id ==
        source_loot_choice(0, original->choices - 1));
    assert(!spawned.spawn_verified_source_choice(
        kSourceLootPoolCount, 0, 11.0f, 12.0f));
    assert(!spawned.spawn_verified_source_choice(
        0, 0, std::numeric_limits<float>::infinity(), 0.0f));
    assert(spawned.count() == 2);
    return 0;
}
