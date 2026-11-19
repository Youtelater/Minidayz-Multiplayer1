#pragma once
#include <cmath>
#include <cstdint>
#include "minidayz/world.hpp"

namespace minidayz::map_overlay {

// The native overlay reads the same generated Array used by the original
// Map generation event groups. Codes below are from source/world.cpp's
// generate_array_locations / generate_array_roads_1/2 mapping.
// The presentation is a 320x240 touchscreen adaptation; source MiniMap
// dragger, discovery and legend events still require separate parity audit.
enum class CellKind : std::uint8_t {
    ground, road_horizontal, road_vertical, road_crossing,
    village, military, city, hospital, firestation, market,
    shop, cafe, location
};

constexpr CellKind cell_kind(std::uint16_t code) noexcept {
    switch(code){
        case 12: return CellKind::road_vertical;
        case 13: return CellKind::road_horizontal;
        case 14: return CellKind::road_crossing;
        case 9: return CellKind::village;
        case 5: return CellKind::military;
        case 6: return CellKind::city;
        case 11: return CellKind::hospital;
        case 15: return CellKind::firestation;
        case 103: return CellKind::market;
        case 51: return CellKind::shop;
        case 50: return CellKind::cafe;
        default: return code == 0 ? CellKind::ground : CellKind::location;
    }
}

constexpr bool is_location(CellKind kind) noexcept {
    return kind>=CellKind::village;
}

struct MapRect {
    float x{},y{},width{},height{};
};
struct MapPoint {
    float x{},y{};
};

// Do not project the player using 16x16 grid dimensions alone: the source
// Map is a much larger world-coordinate space than the procedural Array.
inline MapPoint player_point(float player_x,float player_y,
                             const WorldBounds& bounds,
                             MapRect rect) noexcept {
    if(!(bounds.width>0.0f&&bounds.height>0.0f)||
       !std::isfinite(player_x)||!std::isfinite(player_y))
       return {rect.x,rect.y};
    const float sx=std::fmax(0.0f,std::fmin(1.0f,player_x/bounds.width));
    const float sy=std::fmax(0.0f,std::fmin(1.0f,player_y/bounds.height));
    return {rect.x+sx*rect.width,rect.y+sy*rect.height};
}

} // namespace minidayz::map_overlay
