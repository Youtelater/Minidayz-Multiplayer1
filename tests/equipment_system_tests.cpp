#include "minidayz/equipment.hpp"

#include <cassert>
#include <cmath>

using namespace minidayz;
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
    assert(EquipmentSystem::source_slot_for_item(158, slot));
    assert(slot == EquipmentSlot::hands);
    assert(!EquipmentSystem::source_slot_for_item(137, slot)); // gloves are not a source equipment slot
    assert(!EquipmentSystem::source_slot_for_item(140, slot)); // no source footwear slot
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
    assert(equipment.unequip_to_inventory(inventory, EquipmentSlot::head, 1));
    assert(equipment.equipped(EquipmentSlot::head) == 0);
    assert(inventory.count(264) == 1);

    const auto saved = equipment.snapshot();
    EquipmentSystem restored;
    assert(restored.restore(saved));
    auto invalid = saved;
    invalid.item_ids[static_cast<std::size_t>(EquipmentSlot::pants)] = 251;
    assert(!restored.restore(invalid));
    assert(restored.equipped(EquipmentSlot::head) == 0);

    // Original map instance variables supply exact flat armor protection.
    SourceItemProtection protection{};
    assert(source_item_protection(251, protection));
    assert(protection.warmth == 2.0f);
    assert(protection.helmet == 4.0f);
    assert(source_item_protection(355, protection));
    assert(protection.warmth == 7.0f);
    assert(protection.outerwear == 2.0f);
    assert(source_item_protection(402, protection));
    assert(protection.vest == 3.0f);
    assert(source_item_protection(303, protection));
    assert(protection.warmth == 7.0f);
    assert(protection.pants == 2.0f);
    assert(source_item_protection(408, protection));
    assert(protection.vest == 20.0f);
    assert(!source_item_protection(450, protection));

    EquipmentSystem armor;
    InventorySystem armor_inventory;
    assert(armor_inventory.add(251, 1));
    assert(armor_inventory.add(355, 1));
    assert(armor_inventory.add(402, 1));
    assert(armor_inventory.add(303, 1));
    assert(armor.equip_from_inventory(armor_inventory, 0, EquipmentSlot::head));
    assert(armor.equip_from_inventory(armor_inventory, 1, EquipmentSlot::outerwear));
    assert(armor.equip_from_inventory(armor_inventory, 2, EquipmentSlot::vest));
    assert(armor.equip_from_inventory(armor_inventory, 3, EquipmentSlot::pants));
    const auto armor_values = armor.protection();
    assert(armor_values.helmet == 4.0f);
    assert(armor_values.outerwear == 2.0f);
    assert(armor_values.vest == 3.0f);
    assert(armor_values.pants == 2.0f);
    assert(std::fabs(armor.temperature_offset() - (5.0f / 7.0f)) < 0.0001f);

    PlayerState protected_state{};
    protected_state.health = 100.0f;
    protected_state.alive = true;
    PlayerSystem protected_player(protected_state);
    protected_player.apply_damage(20.0f, armor_values);
    assert(protected_player.state().health == 91.0f);
    protected_player.apply_damage(5.0f, armor_values);
    assert(protected_player.state().health == 91.0f);

    // Failed replacement must leave both inventory and equipment unchanged.
    EquipmentSystem atomic;
    InventorySystem full;
    assert(atomic.restore(EquipmentSnapshot{{251, 0, 0, 0, 0, 0, 0}}));
    for (std::uint16_t id = 1; id <= 15; ++id) assert(full.add(id, 1));
    assert(full.add(264, 1) == false);
    return 0;
}
