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
    std::array<float, kEquipmentSlotCount> conditions{
        100.0f, 100.0f, 100.0f, 100.0f, 100.0f, 100.0f, 100.0f
    };
};

bool source_item_protection(std::uint16_t item_id,
                            SourceItemProtection& protection) noexcept;

// Raw source event UIDs from the six original Stage 2 condition-wear
// groups. The containing event conditions must be satisfied by the caller.
// A random fraction in [0, 1) is required only by Player_get_hit.
bool source_condition_wear_loss(std::uint64_t event_uid, bool perk_wxnj,
                                float random_fraction, float& loss) noexcept;
/** Source action-specific wear from the generated original Construct catalog.
 * Event UIDs may be repeated, but each source action UID is exact.
 * The caller evaluates enclosing event predicates before dispatching.
 */
bool source_condition_wear_action(std::uint64_t action_uid, bool perk_wxnj,
                                  float random_fraction, float crafted,
                                  float& loss) noexcept;

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
    // Original Player_temperature clothing term, excluding Day_cold.
    float temperature_offset() const noexcept;
    // Source Update_Stats assigns Player_temperature = Day_cold + clothing term.
    // This is an absolute target, not a per-second temperature delta.
    void apply_temperature(PlayerSystem& player, float day_cold,
                           float min_temperature, float max_temperature) const noexcept;
    const EquipmentSnapshot& snapshot() const noexcept { return state_; }
    bool restore(const EquipmentSnapshot& saved) noexcept;

    // Equipping is atomic: the incoming item is consumed and any replaced
    // item is returned to inventory. Equipment items must be single-count.
    bool equip_from_inventory(InventorySystem& inventory,
                              std::size_t inventory_slot,
                              EquipmentSlot equipment_slot) noexcept;
    bool damage_condition(EquipmentSlot slot, float loss) noexcept;
    // Dispatch one already-triggered source condition-write event to the
    // held weapon; does not invent attack input or collision timing.
    bool apply_source_wear(std::uint64_t event_uid, bool perk_wxnj,
                           float random_fraction = 0.0f) noexcept;
    bool apply_source_wear_action(std::uint64_t action_uid, bool perk_wxnj,
                                  float random_fraction = 0.0f,
                                  float crafted = 0.0f) noexcept;
    // Apply an absolute condition value decoded from a source repair action.
    bool restore_condition(EquipmentSlot slot, float condition) noexcept;
    // Consume one original repair material and apply the source-defined gain.
    // helmet_lite is a distinct original menu action sharing the head slot.
    bool repair_from_inventory(InventorySystem& inventory,
                               std::size_t inventory_slot,
                               SourceRepairTarget target) noexcept;
    bool unequip_to_inventory(InventorySystem& inventory,
                              EquipmentSlot equipment_slot,
                              std::size_t inventory_slot) noexcept;
    void clear() noexcept { state_ = {}; }

private:
    EquipmentSnapshot state_{};
};

} // namespace minidayz::items
