#include "minidayz/equipment.hpp"

namespace minidayz::items {

bool EquipmentSystem::source_slot_for_item(std::uint16_t id,
                                           EquipmentSlot& slot) noexcept {
    // IDs are taken from the original l_eng_items.xml and checked against
    // MDZItemTable. Unclassified items are deliberately not equipable.
    if ((id >= 250 && id <= 277) || (id >= 133 && id <= 136) ||
        id == 748 || id == 787) {
        slot = EquipmentSlot::head;
        return true;
    }
    if ((id >= 350 && id <= 368) || id == 702 || id == 703 ||
        (id >= 409 && id <= 415) || id == 711 || id == 713) {
        slot = EquipmentSlot::outerwear;
        return true;
    }
    if ((id >= 400 && id <= 405) || id == 408) {
        slot = EquipmentSlot::vest;
        return true;
    }
    if (id >= 300 && id <= 307) {
        slot = EquipmentSlot::pants;
        return true;
    }
    if (id == 137 || id == 701 || id == 757) {
        slot = EquipmentSlot::hands;
        return true;
    }
    if (id == 139 || id == 140 || id == 141 || id == 700) {
        slot = EquipmentSlot::feet;
        return true;
    }
    if ((id >= 151 && id <= 216) || (id >= 230 && id <= 246) ||
        id == 103 || id == 107 || id == 108 || id == 785) {
        slot = EquipmentSlot::weapon;
        return true;
    }
    return false;
}

bool EquipmentSystem::item_fits_slot(std::uint16_t id,
                                     EquipmentSlot slot) noexcept {
    EquipmentSlot actual{};
    return source_slot_for_item(id, actual) && actual == slot;
}

std::uint16_t EquipmentSystem::equipped(EquipmentSlot slot) const noexcept {
    const auto index = static_cast<std::size_t>(slot);
    return index < state_.item_ids.size() ? state_.item_ids[index] : 0;
}

bool EquipmentSystem::restore(const EquipmentSnapshot& saved) noexcept {
    for (std::size_t i = 0; i < saved.item_ids.size(); ++i) {
        if (saved.item_ids[i] == 0) continue;
        const auto slot = static_cast<EquipmentSlot>(i);
        if (!item_fits_slot(saved.item_ids[i], slot)) return false;
        for (std::size_t j = i + 1; j < saved.item_ids.size(); ++j)
            if (saved.item_ids[i] == saved.item_ids[j]) return false;
    }
    state_ = saved;
    return true;
}

bool EquipmentSystem::equip_from_inventory(
    InventorySystem& inventory, std::size_t inventory_slot,
    EquipmentSlot equipment_slot) noexcept {
    const auto slot_index = static_cast<std::size_t>(equipment_slot);
    const auto* incoming = inventory.slot(inventory_slot);
    if (slot_index >= state_.item_ids.size() || !incoming ||
        incoming->empty() || incoming->count != 1 ||
        !item_fits_slot(incoming->item_id, equipment_slot))
        return false;

    const auto old_inventory = inventory.snapshot();
    const std::uint16_t incoming_id = incoming->item_id;
    const std::uint16_t outgoing_id = state_.item_ids[slot_index];
    if (!inventory.remove(inventory_slot, 1)) return false;
    if (outgoing_id != 0 && !inventory.add(outgoing_id, 1)) {
        (void)inventory.restore(old_inventory);
        return false;
    }
    state_.item_ids[slot_index] = incoming_id;
    return true;
}

bool EquipmentSystem::unequip_to_inventory(
    InventorySystem& inventory, EquipmentSlot equipment_slot,
    std::size_t inventory_slot) noexcept {
    const auto slot_index = static_cast<std::size_t>(equipment_slot);
    if (slot_index >= state_.item_ids.size() ||
        state_.item_ids[slot_index] == 0)
        return false;
    const auto old_inventory = inventory.snapshot();
    const std::uint16_t outgoing_id = state_.item_ids[slot_index];
    if (!inventory.add(outgoing_id, 1)) return false;
    // add() may stack into an existing slot, so the requested target is
    // intentionally a placement hint only when that slot is empty.
    (void)inventory_slot;
    state_.item_ids[slot_index] = 0;
    if (!inventory.restore(inventory.snapshot())) {
        (void)inventory.restore(old_inventory);
        return false;
    }
    return true;
}

} // namespace minidayz::items
