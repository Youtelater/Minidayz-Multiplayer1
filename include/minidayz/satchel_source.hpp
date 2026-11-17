#pragma once
#include <cstdint>
#include "minidayz/inventory.hpp"

namespace minidayz::items {

// These exact descriptions are in the ROOT l_eng_new.xml, IDs 309..311
// and the matching root l_eng_items.xml native item IDs 458..460,
// 752..754 (stowed variants). They are NOT ordinary backpack extra slots.
struct SourceSatchelCapacity {
    std::uint8_t unrestricted{};
    std::uint8_t exclusive{};
    bool alongside_backpack{};
};

inline SourceSatchelCapacity source_satchel_capacity(
    std::uint16_t item_id) noexcept {
    switch (item_id) {
    case 458: case 752: return {2, 0, true};
    case 459: case 753: return {1, 3, true};
    case 460: case 754: return {1, 3, true};
    default: return {};
    }
}

inline bool is_source_satchel(std::uint16_t item_id) noexcept {
    return source_satchel_capacity(item_id).alongside_backpack;
}

} // namespace minidayz::items
