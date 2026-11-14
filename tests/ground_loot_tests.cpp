#include "minidayz/ground_loot.hpp"

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

    // Original Backpack slot can be equipped without guessing extra capacity,
    // then dropped into the same world-loot system with its condition intact.
    InventorySystem pack_inventory;
    assert(pack_inventory.add(450, 1, 0, 42));
    assert(pack_inventory.equip_backpack_from_slot(0, 0));
    assert(pack_inventory.equipped_backpack() == 450);
    assert(pack_inventory.capacity() == kInventorySlotCount);
    auto pack_snapshot = pack_inventory.snapshot();
    InventorySystem pack_restored;
    assert(pack_restored.restore(pack_snapshot));
    assert(pack_restored.equipped_backpack() == 450);
    assert(pack_restored.capacity() == kInventorySlotCount);
    GroundLootSystem pack_ground;
    assert(pack_ground.drop_equipped_backpack(pack_restored, 8.0f, 9.0f));
    assert(pack_restored.equipped_backpack() == 0);
    assert(pack_ground.count() == 1);
    assert(pack_ground.entry(0)->item.item_id == 450);
    assert(pack_ground.entry(0)->item.condition == 42);

    // A full ground-loot array must leave the equipped backpack untouched.
    InventorySystem full_pack;
    assert(full_pack.add(450, 1));
    assert(full_pack.equip_backpack_from_slot(0, 0));
    GroundLootSystem full_ground;
    for (std::size_t i = 0; i < kMaximumGroundLootItems; ++i)
        assert(full_ground.add({1, 1, 100}, static_cast<float>(i), 0.0f));
    assert(!full_ground.drop_equipped_backpack(full_pack, 0.0f, 0.0f));
    assert(full_pack.equipped_backpack() == 450);

    return 0;
}
