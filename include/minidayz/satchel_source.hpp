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

// Original Player stats events 315583148629362, 699699206748072,
// 962634450988744 reject every non-Army-EDC item in exclusive slots
// perk_kb2/3/4. Original events 926789927221036, 353303031815206,
// 250048865204980 do the same for the Medical-exclusive three slots.
// These are the exact comparison literals, not category guesses.
inline bool source_satchel_exclusive_accepts(
    std::uint16_t satchel_item, std::uint16_t item_id) noexcept {
    if (item_id == 0) return true; // empty slot
    switch (satchel_item) {
    case 459: case 753: // Army Satchel
        switch (item_id) {
        case 24: case 42: case 43: case 44: case 20:
        case 55: case 125: return true;
        default: return false;
        }
    case 460: case 754: // Medical Satchel
        switch (item_id) {
        case 18: case 19: case 28: case 29: case 45: case 46:
        case 109: case 118: case 708: case 710: case 143:
        case 759: case 760: return true;
        default: return false;
        }
    default: return false;
    }
}

// ROOT l_eng_new.xml entry 312 describes original Supplies Stash (+6
// storage slots), while root l_eng_items.xml maps item ID 461.
// This is capacity metadata only: its dimensional-stash transfer action
// has not been translated, so do not treat it as a wearable backpack.
inline std::uint8_t source_supplies_stash_slots(
    std::uint16_t item_id) noexcept {
    return item_id == 461 ? 6 : 0;
}

} // namespace minidayz::items
