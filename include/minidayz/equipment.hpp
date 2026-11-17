#pragma once

#include "minidayz/inventory.hpp"
#include "minidayz/player.hpp"
#include "minidayz/generated/item_equipment_catalog.hpp"
#include "minidayz/item_repair.hpp"

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

// Persisted separately from the legacy EquipmentSnapshot so v1-v11 saves
// remain byte-compatible and gain zero subpercent wear on migration.
struct EquipmentFractionSnapshot {
    std::array<std::uint8_t, kEquipmentSlotCount> fractional_loss{};
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
    // Apply the original flat armor subtraction to incoming player damage.
    void apply_damage(PlayerSystem& player, float amount) const noexcept;
    // Exact original clothing warmth sum, excluding Day_cold.
    float temperature_offset() const noexcept;
    // Original daytime cooling term before weather, perk, and night modifiers.
    float day_cooling(float day_cold) const noexcept;
    const EquipmentSnapshot& snapshot() const noexcept { return state_; }
    bool restore(const EquipmentSnapshot& saved) noexcept;
    bool restore_with_fraction(const EquipmentSnapshot& saved,
                               const EquipmentFractionSnapshot& fractions) noexcept;
    EquipmentFractionSnapshot fraction_snapshot() const noexcept {
        return fractions_;
    }

    // Equipping is atomic: the incoming item is consumed and any replaced
    // item is returned to inventory. Equipment items must be single-count.
    bool equip_from_inventory(InventorySystem& inventory,
                              std::size_t inventory_slot,
                              EquipmentSlot equipment_slot) noexcept;
    bool damage_condition(EquipmentSlot slot, std::uint8_t loss) noexcept;
    bool damage_condition_hundredths(
        EquipmentSlot slot, std::uint16_t loss_hundredths) noexcept;
    bool apply_verified_wear_action(EquipmentSlot slot,
        std::uint64_t group_uid, std::uint64_t event_uid,
        std::uint64_t action_uid, bool source_guards_satisfied,
        bool perk_wxnj_active) noexcept;
    // Apply an absolute condition value decoded from a source repair action.
    bool restore_condition(EquipmentSlot slot, std::uint8_t condition) noexcept;
    // Consume one original repair material and apply the source-defined gain.
    // helmet_lite is a distinct original menu action sharing the head slot.
    bool repair_from_inventory(InventorySystem& inventory,
                               std::size_t inventory_slot,
                               SourceRepairTarget target) noexcept;
    bool unequip_to_inventory(InventorySystem& inventory,
                              EquipmentSlot equipment_slot,
                              std::size_t inventory_slot) noexcept;
    void clear() noexcept { state_ = {}; fractions_ = {}; }

private:
    EquipmentSnapshot state_{};
    EquipmentFractionSnapshot fractions_{};
};

} // namespace minidayz::items
