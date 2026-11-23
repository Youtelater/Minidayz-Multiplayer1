#pragma once
#include <cmath>
#include <cstdint>
#include "minidayz/world.hpp"
#include "minidayz/source_mapgen_geometry.hpp"

namespace minidayz::map_overlay {

// The native overlay reads the same generated Array used by the original
// Map generation event groups. Codes below are from source/world.cpp's
// generate_array_locations / generate_array_roads_1/2 mapping.
// The presentation is a 320x240 touchscreen adaptation; source MiniMap
// dragger, discovery and legend events still require separate parity audit.
enum class CellKind : std::uint8_t {
    ground, road_horizontal, road_vertical, road_crossing,
    village, military, city, hospital, firestation, market,
    shop, cafe, gas_station, location
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
        case 99: return CellKind::gas_station;
        // Original Array has many non-POI marker/temporary codes. Mapping
        // every nonzero value to a settlement invents dozens of map icons.
        default: return CellKind::ground;
    }
}

// Names correspond to original procedural Array POI codes, not invented
// map markers. Unknown temporary Array codes deliberately have no label.
constexpr const char* location_name(CellKind kind) noexcept {
    switch(kind){
    case CellKind::village: return "VILLAGE";
    case CellKind::military: return "MILITARY BASE";
    case CellKind::city: return "CITY";
    case CellKind::hospital: return "HOSPITAL";
    case CellKind::firestation: return "FIRE STATION";
    case CellKind::market: return "MARKET";
    case CellKind::shop: return "SHOP";
    case CellKind::cafe: return "ROADSIDE CAFE";
    case CellKind::gas_station: return "GAS STATION";
    default: return nullptr;
    }
}

// The original roadside services are not written into Array518: they
// replace Sprite514 road markers in generate_array_tomap. Keep the raw
// Array road for corridor rendering, but show the selected location from
// the actual source marker (Sprite514.var0).
constexpr CellKind marker_cell_kind(const WorldMapMarker& marker) noexcept {
    // The source Sprite519 SetFrame event maps several distinct Sprite514
    // values to the SAME original village/city minimap symbol. The prior
    // port lost those locations by consulting Array518 alone.
    switch(marker.marker_type){
    case 4: case 7: case 9: case 16: case 17: return CellKind::village;
    case 6: case 21: return CellKind::city;
    case 5: return CellKind::military;
    case 11: return CellKind::hospital;
    case 15: return CellKind::firestation;
    case 103: return CellKind::market;
    case 30: case 31: return CellKind::gas_station;
    case 50: case 51:
        return marker.source_code==12 || marker.source_code==13
            ? CellKind::cafe : CellKind::shop;
    case 108: case 109: return CellKind::shop;
    default: return cell_kind(marker.source_code);
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

// Root original create_mapgen UID3669795404796534 transposes Array axes:
//   world X=700+arrayY*1020, world Y=320+arrayX*1020.
// The 3DS map previously drew array X horizontally / array Y vertically,
 // placing towns and roads at different locations from the actual world.
 // Project every source grid cell through the SAME world coordinates as the
 // generated buildings. Do not fake a 16x33 grid as 16 columns x 33 rows.
inline MapRect source_cell_rect(int array_x,int array_y,
                               const WorldBounds& bounds,
                               MapRect viewport) noexcept {
    if(!(bounds.width>0.0f && bounds.height>0.0f))
        return {viewport.x,viewport.y,0.0f,0.0f};
    const auto origin=world_source::mapgen_hotspot(array_x,array_y);
    const float sx=viewport.width/bounds.width;
    const float sy=viewport.height/bounds.height;
    const float cell=world_source::kMapgenStride;
    return {viewport.x+(origin.x-0.5f*cell)*sx,
            viewport.y+(origin.y-0.5f*cell)*sy,
            cell*sx,cell*sy};
}

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
