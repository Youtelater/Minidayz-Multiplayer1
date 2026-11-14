#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace minidayz::items {

inline constexpr std::size_t kInventorySlotCount = 15;
inline constexpr std::size_t kMaximumInventorySlotCount = 32;

inline constexpr std::uint8_t kUseSourceItemCondition = 255;
std::uint8_t source_default_condition(std::uint16_t item_id) noexcept;
bool source_item_exists(std::uint16_t item_id) noexcept;

struct InventorySlot {
    std::uint16_t item_id{};
    std::uint16_t count{};
    std::uint8_t condition{100}; // Original Construct item instance variable 6.
    bool empty() const noexcept { return item_id == 0 || count == 0; }
};

struct GroundItem {
    std::uint16_t item_id{};
    std::uint16_t count{};
    std::uint8_t condition{kUseSourceItemCondition}; // Resolve from original item variable 6 on pickup.
    bool empty() const noexcept { return item_id == 0 || count == 0; }
};

struct InventorySnapshot {
    std::array<InventorySlot, kMaximumInventorySlotCount> slots{};
    std::uint16_t capacity{static_cast<std::uint16_t>(kInventorySlotCount)};
    std::uint16_t equipped_backpack{};
    std::uint8_t equipped_backpack_condition{100};
};

// Baseline stack limits ported from the original Check_stack_size event.
// A later perk-aware layer can override these when perk_cybb is active.
std::uint16_t source_max_stack(std::uint16_t item_id) noexcept;
// Original Check_stack_size returns 31,415,926 when perk_cybb is active.
// Native slot counts are uint16_t, so UINT16_MAX is the equivalent safe cap.
std::uint16_t source_max_stack(std::uint16_t item_id, bool perk_cybb) noexcept;
bool is_backpack_item(std::uint16_t item_id) noexcept;
// Inventory slot count granted by each wearable backpack; zero for other items.
std::size_t source_backpack_capacity(std::uint16_t item_id) noexcept;
// Exact original Construct object IDs resolved from MDZItemTable and the
// original project object table. 65535 means the source has no mapping.
std::uint16_t source_construct_object(std::uint16_t item_id) noexcept;
// Object ID with original sprite frames, resolved from the source sprite catalog.
std::uint16_t source_sprite_object(std::uint16_t item_id) noexcept;

class InventorySystem {
public:
    InventorySystem() noexcept = default;

    bool set_capacity(std::size_t capacity) noexcept;

    // Backpack equipment is a real equipment slot: equip consumes the item
    // from the chosen inventory slot and swaps the previously equipped pack
    // back into that slot. source_capacity must come from original source data.
    bool equip_backpack_from_slot(std::size_t inventory_slot,
                                  std::size_t source_capacity) noexcept;
    bool unequip_backpack_to_slot(std::size_t inventory_slot) noexcept;
    std::uint16_t equipped_backpack() const noexcept { return equipped_backpack_; }
    std::uint8_t equipped_backpack_condition() const noexcept {
        return equipped_backpack_condition_;
    }
    std::size_t capacity() const noexcept { return capacity_; }

    // max_stack == 0 selects the original game's baseline rule for item_id.
    bool add(std::uint16_t item_id, std::uint16_t amount,
             std::uint16_t max_stack = 0,
             std::uint8_t condition = kUseSourceItemCondition) noexcept;
    // Place or merge into one exact slot without silently choosing another.
    bool add_to_slot(std::size_t slot, std::uint16_t item_id,
                     std::uint16_t amount, std::uint16_t max_stack = 0,
                     std::uint8_t condition = kUseSourceItemCondition) noexcept;
    bool remove(std::size_t slot, std::uint16_t amount) noexcept;

    // Ground transfers are atomic: failed pickups/drops leave both sides intact.
    bool pickup(GroundItem& ground, std::uint16_t max_stack = 0) noexcept;
    bool drop(std::size_t inventory_slot, std::uint16_t amount,
              GroundItem& ground, std::uint16_t max_stack = 0) noexcept;

    // amount == 0 moves the complete source stack. Same-item merges obey the
    // original stack cap; different item types swap only as complete stacks.
    bool move(std::size_t from, std::size_t to,
              std::uint16_t amount = 0,
              std::uint16_t max_stack = 0) noexcept;

    const InventorySlot* slot(std::size_t index) const noexcept;
    std::uint32_t count(std::uint16_t item_id) const noexcept;
    std::size_t occupied_slots() const noexcept;
    InventorySnapshot snapshot() const noexcept;
    bool restore(const InventorySnapshot& snapshot) noexcept;
    void clear() noexcept {
        slots_ = {};
        capacity_ = kInventorySlotCount;
        equipped_backpack_ = 0;
        equipped_backpack_condition_ = 100;
    }

private:
    std::array<InventorySlot, kMaximumInventorySlotCount> slots_{};
    std::size_t capacity_{kInventorySlotCount};
    std::uint16_t equipped_backpack_{};
    std::uint8_t equipped_backpack_condition_{100};
};

} // namespace minidayz::items
