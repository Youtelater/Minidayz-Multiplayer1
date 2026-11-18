#pragma once
#include <cstdint>

namespace minidayz::terrain {

// Source asset names: hard_ground, forest_run, sand_run, road_run,
// swamp_step. Only classify authored original tiles and road Array cells.
enum class FootstepSurface : std::uint8_t {
    ground,
    road,
    forest,
    sand,
    shallow_water
};

constexpr FootstepSurface surface_for_detail_object(std::uint16_t object) noexcept {
    switch (object) {
    case 208: return FootstepSurface::shallow_water; // tilemap_water
    case 211: return FootstepSurface::forest;       // tilemap_forest
    case 418: return FootstepSurface::sand;         // tilemap_sand
    default: return FootstepSurface::ground;
    }
}

constexpr bool detail_blocks_player(std::uint16_t object) noexcept {
    return object == 208 || object == 211;
}

// Source road_Array values assigned by generate_array_roads_1 and _2.
constexpr bool generated_road_cell(std::uint16_t code) noexcept {
    return code == 12 || code == 13 || code == 14;
}

} // namespace minidayz::terrain
