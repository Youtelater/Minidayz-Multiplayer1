#include "minidayz/equipment.hpp"

#include <cassert>

using namespace minidayz::items;

int main() {
    EquipmentSystem equipment;
    EquipmentSlot slot{};
    assert(EquipmentSystem::source_slot_for_item(251, slot));
    assert(slot == EquipmentSlot::head);
    assert(EquipmentSystem::source_slot_for_item(303, slot));
    assert(slot == EquipmentSlot::pants);
    assert(EquipmentSystem::source_slot_for_item(355, slot));
    assert(slot == EquipmentSlot::outerwear);
    assert(EquipmentSystem::source_slot_for_item(402, slot));
    assert(slot == EquipmentSlot::vest);
    assert(EquipmentSystem::source_slot_for_item(137, slot));
    assert(slot == EquipmentSlot::hands);
    assert(EquipmentSystem::source_slot_for_item(140, slot));
    assert(slot == EquipmentSlot::feet);
    assert(EquipmentSystem::source_slot_for_item(158, slot));
    assert(slot == EquipmentSlot::weapon);
    assert(!EquipmentSystem::source_slot_for_item(456, slot)); // civilian tent

    InventorySystem inventory;
    assert(inventory.add(251, 1));
    assert(inventory.add(264, 1));
    assert(inventory.add(18, 1));
    assert(equipment.equip_from_inventory(inventory, 0, EquipmentSlot::head));
    assert(equipment.equipped(EquipmentSlot::head) == 251);
    assert(inventory.count(251) == 0);
    assert(!equipment.equip_from_inventory(inventory, 1, EquipmentSlot::pants));

    // Replacing headgear returns the old item atomically.
    assert(equipment.equip_from_inventory(inventory, 1, EquipmentSlot::head));
    assert(equipment.equipped(EquipmentSlot::head) == 264);
    assert(inventory.count(251) == 1);
    assert(equipment.unequip_to_inventory(inventory, EquipmentSlot::head, 2));
    assert(equipment.equipped(EquipmentSlot::head) == 0);
    assert(inventory.count(264) == 1);

    const auto saved = equipment.snapshot();
    EquipmentSystem restored;
    assert(restored.restore(saved));
    auto invalid = saved;
    invalid.item_ids[static_cast<std::size_t>(EquipmentSlot::pants)] = 251;
    assert(!restored.restore(invalid));
    assert(restored.equipped(EquipmentSlot::head) == 0);

    // Failed replacement must leave both inventory and equipment unchanged.
    EquipmentSystem atomic;
    InventorySystem full;
    assert(atomic.restore(EquipmentSnapshot{{251, 0, 0, 0, 0, 0, 0}}));
    for (std::uint16_t id = 1; id <= 15; ++id) assert(full.add(id, 1));
    assert(full.add(264, 1) == false);
    return 0;
}
