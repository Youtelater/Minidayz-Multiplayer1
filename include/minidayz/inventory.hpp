#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace minidayz::items {

inline constexpr std::size_t kInventorySlotCount = 15;

struct InventorySlot {
    std::uint16_t item_id{};
    std::uint16_t count{};
    bool empty() const noexcept { return item_id == 0 || count == 0; }
};

class InventorySystem {
public:
    InventorySystem() noexcept = default;

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
    void clear() noexcept { slots_ = {}; }

private:
    std::array<InventorySlot, kInventorySlotCount> slots_{};
};

} // namespace minidayz::items
