#pragma once

#include "minidayz/inventory.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace minidayz::items {

inline constexpr std::size_t kMaximumGroundLootItems = 64;

struct GroundLootEntry {
    GroundItem item{};
    float x{};
    float y{};
};

struct GroundLootSnapshot {
    std::array<GroundLootEntry, kMaximumGroundLootItems> entries{};
    std::uint16_t count{};
};

class GroundLootSystem {
public:
    bool add(const GroundItem& item, float x, float y) noexcept;
    bool drop_from_inventory(InventorySystem& inventory, std::size_t slot,
                             std::uint16_t amount, float x, float y) noexcept;
    bool drop_equipped_backpack(InventorySystem& inventory,
                                float x, float y) noexcept;
    bool pickup_nearest(InventorySystem& inventory, float x, float y,
                        float radius) noexcept;

    const GroundLootEntry* nearest(float x, float y,
                                   float radius) const noexcept;
    const GroundLootEntry* entry(std::size_t index) const noexcept;
    std::size_t count() const noexcept { return count_; }

    GroundLootSnapshot snapshot() const noexcept;
    bool restore(const GroundLootSnapshot& saved) noexcept;
    void clear() noexcept { entries_ = {}; count_ = 0; }

private:
    std::array<GroundLootEntry, kMaximumGroundLootItems> entries_{};
    std::size_t count_{};
};

} // namespace minidayz::items
