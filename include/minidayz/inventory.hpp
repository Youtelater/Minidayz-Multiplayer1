#pragma once

#include <array>
#include <cstddef>
#include <cstdint>


namespace minidayz::items {

inline constexpr std::size_t kInventorySlotCount = 15;
inline constexpr std::size_t kMaximumInventorySlotCount = 32;

struct InventorySlot {
    std::uint16_t item_id{};
    std::uint16_t count{};
    bool empty() const noexcept { return item_id == 0 || count == 0; }
};

// Fixed-size, pointer-free representation suitable for the game's save file.
struct InventorySnapshot {
    std::array<InventorySlot, kMaximumInventorySlotCount> slots{};
    std::uint16_t capacity{static_cast<std::uint16_t>(kInventorySlotCount)};
    std::uint16_t equipped_backpack{};
};

class InventorySystem {
public:
    InventorySystem() noexcept = default;
    bool set_capacity(std::size_t capacity) noexcept;
    // Capacity comes from a verified original item rule; never guess by name.
    bool equip_backpack(std::uint16_t item_id, std::size_t source_capacity) noexcept;
    bool unequip_backpack() noexcept;
    std::uint16_t equipped_backpack() const noexcept { return equipped_backpack_; }
    std::size_t capacity() const noexcept { return capacity_; }

    // max_stack is supplied by source-derived item rules. The conservative
    // default is one item per slot until those rules are connected.
    bool add(std::uint16_t item_id, std::uint16_t amount,
             std::uint16_t max_stack = 1) noexcept;
    bool remove(std::size_t slot, std::uint16_t amount) noexcept;
    // amount == 0 moves the complete source stack. Different item types may
    // only be swapped as complete stacks.
    bool move(std::size_t from, std::size_t to,
              std::uint16_t amount = 0) noexcept;

    const InventorySlot* slot(std::size_t index) const noexcept;
    std::uint32_t count(std::uint16_t item_id) const noexcept;
    std::size_t occupied_slots() const noexcept;
    InventorySnapshot snapshot() const noexcept;
    bool restore(const InventorySnapshot& snapshot) noexcept;
    void clear() noexcept {
        slots_ = {};
        capacity_ = kInventorySlotCount;
        equipped_backpack_ = 0;
    }

private:
    std::array<InventorySlot, kMaximumInventorySlotCount> slots_{};
    std::size_t capacity_{kInventorySlotCount};
    std::uint16_t equipped_backpack_{};
};

} // namespace minidayz::items
