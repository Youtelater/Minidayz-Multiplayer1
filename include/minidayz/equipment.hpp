#pragma once

#include "minidayz/inventory.hpp"
#include "minidayz/player.hpp"
#include "minidayz/generated/item_equipment_catalog.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace minidayz::items {

// The active source-backed slots are helmet, outerwear, vest, pants and
// hands (held weapon). Feet and weapon are reserved legacy indices from the
// first native save layout; backpack remains InventorySystem-owned.
enum class EquipmentSlot : std::uint8_t {
    head, outerwear, vest, pants, hands, feet, weapon, count
};

inline constexpr std::size_t kEquipmentSlotCount =
    static_cast<std::size_t>(EquipmentSlot::count);

struct EquipmentSnapshot {
    std::array<std::uint16_t, kEquipmentSlotCount> item_ids{};
    std::array<std::uint8_t, kEquipmentSlotCount> conditions{
        100, 100, 100, 100, 100, 100, 100
    };
};

bool source_item_protection(std::uint16_t item_id,
                            SourceItemProtection& protection) noexcept;

class EquipmentSystem {
public:
    EquipmentSystem() noexcept = default;

    static bool item_fits_slot(std::uint16_t item_id,
                               EquipmentSlot slot) noexcept;
    static bool source_slot_for_item(std::uint16_t item_id,
                                     EquipmentSlot& slot) noexcept;

    std::uint16_t equipped(EquipmentSlot slot) const noexcept;
    PlayerArmorProtection protection() const noexcept;
    // Original Player_temperature clothing term, excluding Day_cold.
    float temperature_offset() const noexcept;
    const EquipmentSnapshot& snapshot() const noexcept { return state_; }
    bool restore(const EquipmentSnapshot& saved) noexcept;

    // Equipping is atomic: the incoming item is consumed and any replaced
    // item is returned to inventory. Equipment items must be single-count.
    bool equip_from_inventory(InventorySystem& inventory,
                              std::size_t inventory_slot,
                              EquipmentSlot equipment_slot) noexcept;
    bool damage_condition(EquipmentSlot slot, std::uint8_t loss) noexcept;
    bool unequip_to_inventory(InventorySystem& inventory,
                              EquipmentSlot equipment_slot,
                              std::size_t inventory_slot) noexcept;
    void clear() noexcept { state_ = {}; }

private:
    EquipmentSnapshot state_{};
};

} // namespace minidayz::items
