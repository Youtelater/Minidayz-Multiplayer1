#pragma once
#include "minidayz/generated/item_description_catalog.hpp"
#include <cstddef>
#include <cstdint>

namespace minidayz::items {

// Only descriptions whose l_eng_new.xml original comments match an original\n// l_eng_items.xml item name are exposed. Raw text IDs are unrelated to item IDs.\n// Missing
// descriptions are represented by nullptr, not made-up replacements.
inline const char* source_item_description(std::uint16_t item_id) noexcept {
    std::size_t low = 0, high = kVerifiedItemDescriptionCount;
    while (low < high) {
        const std::size_t middle = low + (high - low) / 2;
        if (kVerifiedItemDescriptions[middle].item_id < item_id) low = middle + 1;
        else high = middle;
    }
    return low < kVerifiedItemDescriptionCount &&
           kVerifiedItemDescriptions[low].item_id == item_id
        ? kVerifiedItemDescriptions[low].description : nullptr;
}
} // namespace minidayz::items
