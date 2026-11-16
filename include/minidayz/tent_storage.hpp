#pragma once

#include "minidayz/inventory.hpp"
#include <array>
#include <cstddef>
#include <cstdint>

namespace minidayz::items {

// Civilian Tent (456) is verified against root l_eng_items.xml and
// permanent storage against root l_eng_ui.xml. These are native memory
// safety ceilings, NOT claims about the original game's tent limits.
inline constexpr std::uint16_t kOriginalCivilianTentItemId = 456;
inline constexpr std::size_t kNativeMaximumDeployedTents = 8;
inline constexpr std::size_t kNativeTentStorageSlots = 32;

struct DeployedTent {
    float x{}, y{};
    std::uint8_t condition{100};
    std::array<InventorySlot, kNativeTentStorageSlots> items{};
};

struct TentStorageSnapshot {
    std::array<DeployedTent, kNativeMaximumDeployedTents> tents{};
    std::uint16_t count{};
};

// Native placement safety gate. Exact original Construct placement conditions
// are not yet translated; this prevents clipping and overlapping tents.
using TentPlacementBlocked = bool (*)(void*, float, float) noexcept;

class TentStorageSystem {
public:
    bool deploy_from_inventory(InventorySystem& inventory,
                               std::size_t inventory_slot,
                               float x, float y) noexcept;
    bool deploy_from_inventory_checked(InventorySystem& inventory,
                                       std::size_t inventory_slot,
                                       float x, float y,
                                       TentPlacementBlocked blocked,
                                       void* context,
                                       float clearance_radius = 8.0f) noexcept;
    // Returns tent_count() when none is within the caller's interaction range.
    std::size_t nearest(float x, float y, float radius) const noexcept;
    bool store_from_inventory(InventorySystem& inventory,
                              std::size_t inventory_slot,
                              std::size_t tent_index) noexcept;
    bool take_to_inventory(InventorySystem& inventory,
                           std::size_t tent_index,
                           std::size_t tent_slot) noexcept;
    const DeployedTent* tent(std::size_t index) const noexcept;
    std::size_t tent_count() const noexcept { return count_; }
    TentStorageSnapshot snapshot() const noexcept;
    bool restore(const TentStorageSnapshot& saved) noexcept;
    void clear() noexcept { tents_ = {}; count_ = 0; }

private:
    std::array<DeployedTent, kNativeMaximumDeployedTents> tents_{};
    std::size_t count_{};
};

} // namespace minidayz::items
