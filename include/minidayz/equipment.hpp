#pragma once

#include "minidayz/inventory.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace minidayz::items {

// These seven wearable/held slots mirror the original player's separate
// clothing and weapon layers. Backpack remains the dedicated InventorySystem
// equipment slot because it changes inventory capacity.
enum class EquipmentSlot : std::uint8_t {
    head, outerwear, vest, pants, hands, feet, weapon, count
};

inline constexpr std::size_t kEquipmentSlotCount =
    static_cast<std::size_t>(EquipmentSlot::count);

struct EquipmentSnapshot {
    std::array<std::uint16_t, kEquipmentSlotCount> item_ids{};
};

class EquipmentSystem {
public:
    EquipmentSystem() noexcept = default;

    static bool item_fits_slot(std::uint16_t item_id,
                               EquipmentSlot slot) noexcept;
    static bool source_slot_for_item(std::uint16_t item_id,
                                     EquipmentSlot& slot) noexcept;

    std::uint16_t equipped(EquipmentSlot slot) const noexcept;
    const EquipmentSnapshot& snapshot() const noexcept { return state_; }
    bool restore(const EquipmentSnapshot& saved) noexcept;

    // Equipping is atomic: the incoming item is consumed and any replaced
    // item is returned to inventory. Equipment items must be single-count.
    bool equip_from_inventory(InventorySystem& inventory,
                              std::size_t inventory_slot,
                              EquipmentSlot equipment_slot) noexcept;
    bool unequip_to_inventory(InventorySystem& inventory,
                              EquipmentSlot equipment_slot,
                              std::size_t inventory_slot) noexcept;
    void clear() noexcept { state_ = {}; }

private:
    EquipmentSnapshot state_{};
};

} // namespace minidayz::items
