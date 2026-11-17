#pragma once

#include "minidayz/inventory.hpp"
#include "minidayz/tent_storage.hpp"
#include <array>
#include <cstddef>
#include <cstdint>

namespace minidayz::items {

// Original repository-root item 461: Supplies Stash, six storage cells.
// Native maximum number of independently deployed stashes is a bounded
// memory safety limit, NOT an original Construct gameplay rule.
inline constexpr std::uint16_t kOriginalSuppliesStashItemId = 461;
inline constexpr std::size_t kSourceSuppliesStashSlots = 6;
inline constexpr std::size_t kNativeMaximumStashes = 8;

struct DeployedStash {
    float x{}, y{};
    std::uint8_t condition{100};
    std::array<InventorySlot, kSourceSuppliesStashSlots> items{};
};

struct StashStorageSnapshot {
    std::array<DeployedStash, kNativeMaximumStashes> stashes{};
    std::uint16_t count{};
};

class StashStorageSystem {
public:
    bool deploy_from_inventory_checked(
        InventorySystem& inventory, std::size_t inventory_slot,
        float x, float y, TentPlacementBlocked blocked, void* context,
        float clearance_radius = 8.0f) noexcept;
    bool store_from_inventory(InventorySystem& inventory,
                              std::size_t inventory_slot,
                              std::size_t stash_index) noexcept;
    bool take_to_inventory(InventorySystem& inventory,
                           std::size_t stash_index,
                           std::size_t stash_slot) noexcept;
    std::size_t nearest(float x, float y, float radius) const noexcept;
    const DeployedStash* stash(std::size_t index) const noexcept;
    std::size_t count() const noexcept { return count_; }
    StashStorageSnapshot snapshot() const noexcept;
    bool restore(const StashStorageSnapshot& saved) noexcept;
    void clear() noexcept { stashes_ = {}; count_ = 0; }

private:
    std::array<DeployedStash, kNativeMaximumStashes> stashes_{};
    std::size_t count_{};
};

} // namespace minidayz::items
