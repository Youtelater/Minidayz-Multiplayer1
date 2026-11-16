#include "minidayz/equipment.hpp"
#include "minidayz/generated/item_equipment_catalog.hpp"
#include "minidayz/item_repair.hpp"

#include <algorithm>
#include <cmath>

namespace minidayz::items {

bool EquipmentSystem::source_slot_for_item(std::uint16_t id,
                                           EquipmentSlot& slot) noexcept {
    // The original Player_get_hit and Drop_* events identify helmet,
    // outerwear, vest and pants as separate protection slots. The original
    // Hands activation category is the held-weapon slot.
    if ((id >= 133 && id <= 136) || (id >= 250 && id <= 277) ||
        id == 748 || id == 786 || id == 787) {
        slot = EquipmentSlot::head;
        return true;
    }
    if ((id >= 350 && id <= 368) || id == 702 || id == 703 ||
        id == 409 || (id >= 413 && id <= 415) || id == 711 || id == 713) {
        slot = EquipmentSlot::outerwear;
        return true;
    }
    if ((id >= 400 && id <= 405) || id == 408 ||
        (id >= 410 && id <= 412)) {
        slot = EquipmentSlot::vest;
        return true;
    }
    if (id >= 300 && id <= 307) {
        slot = EquipmentSlot::pants;
        return true;
    }
    if ((id >= 151 && id <= 216) || (id >= 230 && id <= 246) ||
        id == 103 || id == 107 || id == 108 || id == 785) {
        slot = EquipmentSlot::hands;
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

bool source_item_protection(std::uint16_t item_id,
                            SourceItemProtection& protection) noexcept {
    std::size_t first = 0;
    std::size_t last = kSourceItemProtectionCount;
    while (first < last) {
        const std::size_t middle = first + (last - first) / 2;
        if (kSourceItemProtection[middle].item_id < item_id)
            first = middle + 1;
        else
            last = middle;
    }
    if (first >= kSourceItemProtectionCount ||
        kSourceItemProtection[first].item_id != item_id)
        return false;
    protection = kSourceItemProtection[first];
    return true;
}

bool EquipmentSystem::restore(const EquipmentSnapshot& saved) noexcept {
    EquipmentSnapshot normalized = saved;
    // Migrate the first native save's provisional feet/weapon slots into the
    // original game's outerwear/hands slots when those are otherwise empty.
    const auto feet = static_cast<std::size_t>(EquipmentSlot::feet);
    const auto weapon = static_cast<std::size_t>(EquipmentSlot::weapon);
    const auto outerwear = static_cast<std::size_t>(EquipmentSlot::outerwear);
    const auto hands = static_cast<std::size_t>(EquipmentSlot::hands);
    if (normalized.item_ids[feet] != 0) {
        if (normalized.item_ids[outerwear] != 0) return false;
        normalized.item_ids[outerwear] = normalized.item_ids[feet];
        normalized.conditions[outerwear] = normalized.conditions[feet];
        normalized.item_ids[feet] = 0;
        normalized.conditions[feet] = 100;
    }
    if (normalized.item_ids[weapon] != 0) {
        if (normalized.item_ids[hands] != 0) return false;
        normalized.item_ids[hands] = normalized.item_ids[weapon];
        normalized.conditions[hands] = normalized.conditions[weapon];
        normalized.item_ids[weapon] = 0;
        normalized.conditions[weapon] = 100;
    }
    for (std::size_t i = 0; i < normalized.item_ids.size(); ++i) {
        if (!std::isfinite(normalized.conditions[i]) || normalized.conditions[i] < 0.0f || normalized.conditions[i] > 100.0f) return false;
        if (normalized.item_ids[i] == 0) continue;
        const auto slot = static_cast<EquipmentSlot>(i);
        if (!item_fits_slot(normalized.item_ids[i], slot)) return false;
        for (std::size_t j = i + 1; j < normalized.item_ids.size(); ++j)
            if (normalized.item_ids[i] == normalized.item_ids[j]) return false;
    }
    state_ = normalized;
    return true;
}

PlayerArmorProtection EquipmentSystem::protection() const noexcept {
    // The original Construct Player_get_hit path reads the flat protection
    // instance variables. Item condition is persisted separately; the source
    // does not multiply protection by condition.
    PlayerArmorProtection result{};
    SourceItemProtection item{};
    if (source_item_protection(equipped(EquipmentSlot::head), item))
        result.helmet = item.helmet;
    if (source_item_protection(equipped(EquipmentSlot::outerwear), item))
        result.outerwear = item.outerwear;
    if (source_item_protection(equipped(EquipmentSlot::vest), item))
        result.vest = item.vest;
    if (source_item_protection(equipped(EquipmentSlot::pants), item))
        result.pants = item.pants;
    return result;
}

void EquipmentSystem::apply_damage(PlayerSystem& player, float amount) const noexcept {
    player.apply_damage(amount, protection());
}

float EquipmentSystem::temperature_offset() const noexcept {
    // Exact clothing term used by the original Player_temperature event:
    // (outerwear warmth - helmet warmth) / pants warmth.
    SourceItemProtection item{};
    float helmet_warmth = 0.0f;
    float outerwear_warmth = 0.0f;
    float pants_warmth = 0.0f;
    if (source_item_protection(equipped(EquipmentSlot::head), item))
        helmet_warmth = item.warmth;
    if (source_item_protection(equipped(EquipmentSlot::outerwear), item))
        outerwear_warmth = item.warmth;
    if (source_item_protection(equipped(EquipmentSlot::pants), item))
        pants_warmth = item.warmth;
    if (pants_warmth <= 0.0f) return 0.0f;
    return (outerwear_warmth - helmet_warmth) / pants_warmth;
}

void EquipmentSystem::apply_temperature(
    PlayerSystem& player, float delta_seconds,
    float min_temperature, float max_temperature) const noexcept {
    if (!std::isfinite(delta_seconds) || delta_seconds <= 0.0f) return;
    player.adjust_temperature(temperature_offset() * delta_seconds,
                              min_temperature, max_temperature);
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
    const float incoming_condition = incoming->condition;
    const std::uint16_t outgoing_id = state_.item_ids[slot_index];
    const float outgoing_condition = state_.conditions[slot_index];
    if (!inventory.remove(inventory_slot, 1)) return false;
    if (outgoing_id != 0 &&
        !inventory.add(outgoing_id, 1, 0, outgoing_condition)) {
        (void)inventory.restore(old_inventory);
        return false;
    }
    state_.item_ids[slot_index] = incoming_id;
    state_.conditions[slot_index] = incoming_condition;
    return true;
}

bool EquipmentSystem::damage_condition(
    EquipmentSlot slot, float loss) noexcept {
    const auto index = static_cast<std::size_t>(slot);
    if (index >= state_.item_ids.size() || state_.item_ids[index] == 0 ||
        loss == 0) return false;
    if (!std::isfinite(loss) || loss <= 0.0f) return false;
    if (loss >= state_.conditions[index]) {
        state_.item_ids[index] = 0;
        state_.conditions[index] = 100;
    } else {
        state_.conditions[index] -= loss;
    }
    return true;
}

bool EquipmentSystem::restore_condition(
    EquipmentSlot slot, std::uint8_t condition) noexcept {
    const auto index = static_cast<std::size_t>(slot);
    if (index >= state_.item_ids.size() || state_.item_ids[index] == 0 ||
        condition > 100 || condition <= state_.conditions[index])
        return false;
    state_.conditions[index] = condition;
    return true;
}

bool EquipmentSystem::repair_from_inventory(
    InventorySystem& inventory, std::size_t inventory_slot,
    SourceRepairTarget target) noexcept {
    // Backpack condition is owned by InventorySystem, not an EquipmentSlot.
    // Route the original backpack repair actions to that authoritative state.
    if (target == SourceRepairTarget::backpack)
        return inventory.repair_equipped_backpack_from_inventory_slot(
            inventory_slot);

    const auto* material = inventory.slot(inventory_slot);
    if (!material || material->empty()) return false;

    EquipmentSlot equipment_slot{};
    switch (target) {
        case SourceRepairTarget::helmet:
        case SourceRepairTarget::helmet_lite:
            equipment_slot = EquipmentSlot::head;
            break;
        case SourceRepairTarget::outerwear:
            equipment_slot = EquipmentSlot::outerwear;
            break;
        case SourceRepairTarget::vest:
            equipment_slot = EquipmentSlot::vest;
            break;
        case SourceRepairTarget::pants:
            equipment_slot = EquipmentSlot::pants;
            break;
        case SourceRepairTarget::backpack:
        default:
            return false;
    }
    const auto equipment_index = static_cast<std::size_t>(equipment_slot);
    if (state_.item_ids[equipment_index] == 0) return false;
    const float current = state_.conditions[equipment_index];
    // Original repair menu conditions require condition > 0 and < 100.
    if (current == 0 || current >= 100) return false;

    std::uint8_t gain = 0;
    if (!source_item_repair_gain(material->item_id, target, gain))
        return false;
    const auto repaired = static_cast<std::uint8_t>(
        std::min<unsigned>(100, static_cast<unsigned>(current) + gain));

    const auto inventory_before = inventory.snapshot();
    const auto equipment_before = state_;
    if (!inventory.remove(inventory_slot, 1)) return false;
    if (!restore_condition(equipment_slot, repaired)) {
        (void)inventory.restore(inventory_before);
        state_ = equipment_before;
        return false;
    }
    return true;
}

bool EquipmentSystem::unequip_to_inventory(
    InventorySystem& inventory, EquipmentSlot equipment_slot,
    std::size_t inventory_slot) noexcept {
    const auto slot_index = static_cast<std::size_t>(equipment_slot);
    if (slot_index >= state_.item_ids.size() ||
        state_.item_ids[slot_index] == 0)
        return false;
    const std::uint16_t outgoing_id = state_.item_ids[slot_index];
    const float outgoing_condition = state_.conditions[slot_index];
    if (!inventory.add_to_slot(inventory_slot, outgoing_id, 1, 0,
                               outgoing_condition)) return false;
    state_.item_ids[slot_index] = 0;
    state_.conditions[slot_index] = 100;
    return true;
}

} // namespace minidayz::items
