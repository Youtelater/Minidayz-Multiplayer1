#pragma once
#include "minidayz/world.hpp"
#include <cstdint>

namespace minidayz::map_overlay {

// Original root Game_events > function generate_minimap,
// UID6302523114231127; object519 Set animation frame (command72).
// The 3DS bottom screen uses the original minimap_tile-sheet sprites,
// NOT full PNG-sheet stretching or custom colored road rectangles.
// Source marker type is Sprite514.var0. Array518 remains the raw grid.
inline bool original_minimap_road_neighbor(std::uint16_t code) noexcept {
    // Exact eight Array518.AtXY equality branches under source road13/12:
    // 9,6,4,14,5,11,15,103. Other neighbors do not modify the road frame.
    switch(code) {
    case 9: case 6: case 4: case 14:
    case 5: case 11: case 15: case 103:
        return true;
    default: return false;
    }
}

// Original source event tests every 514 with var1 != 16 (the split gap).
constexpr bool original_minimap_marker_is_visible(
    const WorldMapMarker& marker) noexcept {
    return marker.cell_y != 16;
}

// Default object519 animation0/frame0. All nondefault values below come
// from the original 42 ordered SetFrame actions, INCLUDING two road
// branches where an adjacent original Array518 POI overrides the road art.
// Template preserves the native WorldGrid reader while allowing a
// header-only source Array518 fixture for fast map regressions.
template <typename Grid>
inline std::uint8_t original_minimap_frame(
    const Grid& grid,const WorldMapMarker& marker) noexcept {
    const int x=marker.cell_x,y=marker.cell_y;
    switch(marker.marker_type) {
    case 50: return 19;
    case 51: return 20;
    case 108: return 36;
    case 109: return 37;
    case 67: return 38;
    case 52: return 29;
    case 9: case 4: case 7: case 16: case 17: return 4;
    case 1: case 2: case 8: case 10: return 0;
    case 23: return 14;
    case 3: case 20: return 7;
    case 32: return 26;
    case 5: return 5;
    case 22: return 27;
    case 70: return 30;
    case 6: case 21: return 6;

    case 13: {
        // Root Sprite519 branches 4122715404500452,
        // 170115012520487, 208277984156124,
        // 481102635813681, 404698840810451,
        // 204600216940309. Nested Y+1/Y-1 checks use raw Array518.
        const bool plus=original_minimap_road_neighbor(grid.at(x,y+1));
        const bool minus=original_minimap_road_neighbor(grid.at(x,y-1));
        if(plus&&minus)return 3;
        return plus?12:minus?13:1;
    }
    case 12: {
        // Root Sprite519 branches 7758555832097194,
        // 764471686608496, 198616652897101,
        // 175804461975071, 356315758591965. X+1/X-1 checks.
        const bool plus=original_minimap_road_neighbor(grid.at(x+1,y));
        const bool minus=original_minimap_road_neighbor(grid.at(x-1,y));
        if(plus&&minus)return 3;
        return plus?11:minus?10:2;
    }
    case 11: return 8;
    case 14: return 3;
    case 15: return 9;
    case 30: return 17;
    case 31: return 16;
    case 53: return 21;
    case 54: return 22;
    case 55: return 23;
    case 56: return 24;
    case 102: return 31;
    case 103: return 32;
    case 104: return 33;
    case 105: return 4;
    case 106: case 110: return 34;
    case 107: return 35;
    default: return 0;
    }
}

inline constexpr std::uint16_t kOriginalMinimapSprite=519;
inline constexpr std::uint16_t kOriginalMinimapAnimation=0;
// Root Sprite519 Default has 39 exact 20x20 minimap_tile frames.
inline constexpr std::uint32_t kOriginalMinimapFrameCount=39;
inline constexpr std::uint64_t kOriginalGenerateMinimapEvent=
    6302523114231127ULL;
} // namespace minidayz::map_overlay
