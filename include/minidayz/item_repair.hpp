#pragma once

#include "minidayz/generated/item_repair_catalog.hpp"

namespace minidayz::items {

enum class SourceRepairTarget : std::uint8_t {
    helmet = 0,
    helmet_lite = 1,
    outerwear = 2,
    vest = 3,
    pants = 4,
    backpack = 5
};

inline bool source_item_repair_gain(std::uint16_t item_id,
                                    SourceRepairTarget target,
                                    std::uint8_t& gain) noexcept {
    for (std::size_t i = 0; i < kSourceItemRepairCount; ++i) {
        const auto& repair = kSourceItemRepairs[i];
        if (repair.item_id == item_id &&
            repair.target == static_cast<std::uint8_t>(target)) {
            gain = repair.condition_gain;
            return true;
        }
    }
    return false;
}

} // namespace minidayz::items
