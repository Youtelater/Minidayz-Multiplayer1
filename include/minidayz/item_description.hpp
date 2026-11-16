#pragma once
#include "minidayz/generated/item_description_catalog.hpp"
#include <cstddef>
#include <cstdint>

namespace minidayz::items {

// Only names/descriptions from root l_eng_new.xml are exposed; missing
// descriptions are represented by nullptr, not made-up replacements.
inline const char* source_item_description(std::uint16_t item_id) noexcept {
    std::size_t low = 0, high = kSourceItemDescriptionCount;
    while (low < high) {
        const std::size_t middle = low + (high - low) / 2;
        if (kSourceItemDescriptions[middle].item_id < item_id) low = middle + 1;
        else high = middle;
    }
    return low < kSourceItemDescriptionCount &&
           kSourceItemDescriptions[low].item_id == item_id
        ? kSourceItemDescriptions[low].description : nullptr;
}
} // namespace minidayz::items
