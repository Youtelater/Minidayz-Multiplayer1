#include "minidayz/source_map_overlay.hpp"
#include "minidayz/source_minimap_frames.hpp"
#include <cassert>
#include <cstring>
#include <cmath>
using minidayz::map_overlay::CellKind;
using namespace minidayz::map_overlay;

int main(){
    assert(cell_kind(0)==CellKind::ground);
    // Source generate_array_roads_1/2 codes: 13 horizontal,
    // 12 vertical, 14 junction.
    assert(cell_kind(12)==CellKind::road_vertical);
    assert(cell_kind(13)==CellKind::road_horizontal);
    assert(cell_kind(14)==CellKind::road_crossing);
    assert(cell_kind(9)==CellKind::village);
    assert(cell_kind(5)==CellKind::military);
    assert(cell_kind(6)==CellKind::city);
    assert(cell_kind(11)==CellKind::hospital);
    assert(cell_kind(15)==CellKind::firestation);
    assert(cell_kind(103)==CellKind::market);
    assert(cell_kind(51)==CellKind::shop);
    assert(cell_kind(50)==CellKind::cafe);
    assert(cell_kind(99)==CellKind::gas_station);
    // Original Sprite519 minimap frames are chosen from the final
    // Sprite514 type, not raw road-only symbols. All 42 source actions
    // are covered by the root extraction audit below.
    minidayz::WorldGrid grid;
    assert(grid.resize(16,16));
    const auto check=[&](std::uint16_t code,std::uint8_t frame){
        const minidayz::WorldMapMarker m{4,5,0,code};
        assert(original_minimap_frame(grid,m)==frame);
    };
    check(50,19);check(51,20);check(108,36);check(109,37);
    check(67,38);check(52,29);check(9,4);check(4,4);
    check(7,4);check(16,4);check(17,4);check(1,0);
    check(2,0);check(8,0);check(10,0);check(23,14);
    check(3,7);check(32,26);check(5,5);check(22,27);
    check(70,30);check(6,6);check(21,6);check(11,8);
    check(14,3);check(15,9);check(20,7);check(30,17);
    check(31,16);check(53,21);check(54,22);check(55,23);
    check(56,24);check(102,31);check(103,32);check(104,33);
    check(105,4);check(106,34);check(110,34);check(107,35);
    // Original road13 condition reads Array518.At(X,Y+1/Y-1).
    check(13,1);
    assert(grid.set(4,6,9));
    check(13,12);
    assert(grid.set(4,4,6));
    check(13,3);
    assert(grid.set(4,6,0));
    check(13,13);
    // Original road12 condition reads Array518.At(X+1/X-1,Y).
    check(12,2);
    assert(grid.set(5,5,11));
    check(12,11);
    assert(grid.set(3,5,103));
    check(12,3);
    assert(grid.set(5,5,0));
    check(12,10);
    assert(!original_minimap_marker_is_visible({4,16,13,13}));
    assert(original_minimap_marker_is_visible({4,17,13,13}));

    // Original 514 roadside SetInstanceVariable(0) overrides the raw
    // road (Array518). The touch map labels the marker, not the base road.
    const minidayz::WorldMapMarker gas{2,3,12,30};
    const minidayz::WorldMapMarker gas_h{2,3,13,31};
    const minidayz::WorldMapMarker cafe{2,3,12,50};
    const minidayz::WorldMapMarker cafe_h{2,3,13,51};
    const minidayz::WorldMapMarker shop{2,3,12,108};
    const minidayz::WorldMapMarker shop_h{2,3,13,109};
    assert(marker_cell_kind(gas)==CellKind::gas_station);
    assert(marker_cell_kind(gas_h)==CellKind::gas_station);
    assert(marker_cell_kind(cafe)==CellKind::cafe);
    assert(marker_cell_kind(cafe_h)==CellKind::cafe);
    assert(marker_cell_kind(shop)==CellKind::shop);
    assert(marker_cell_kind(shop_h)==CellKind::shop);
    assert(marker_cell_kind({2,3,9,4})==CellKind::village);
    assert(marker_cell_kind({2,3,13,13})==CellKind::road_horizontal);
    // Unknown mapgen temporary codes are not settlements.
    assert(cell_kind(1)==CellKind::ground);
    assert(cell_kind(2)==CellKind::ground);
    assert(cell_kind(65535)==CellKind::ground);
    assert(location_name(cell_kind(9)));
    assert(std::strcmp(location_name(cell_kind(9)),"VILLAGE")==0);
    assert(std::strcmp(location_name(cell_kind(6)),"CITY")==0);
    assert(std::strcmp(location_name(cell_kind(5)),"MILITARY BASE")==0);
    assert(std::strcmp(location_name(cell_kind(11)),"HOSPITAL")==0);
    assert(std::strcmp(location_name(cell_kind(15)),"FIRE STATION")==0);
    assert(std::strcmp(location_name(cell_kind(103)),"MARKET")==0);
    assert(std::strcmp(location_name(cell_kind(51)),"SHOP")==0);
    assert(std::strcmp(location_name(cell_kind(50)),"ROADSIDE CAFE")==0);
    assert(std::strcmp(location_name(cell_kind(99)),"GAS STATION")==0);
    assert(location_name(cell_kind(1234))==nullptr);
    assert(is_location(cell_kind(9)));
    assert(!is_location(cell_kind(12)));
    assert(!is_location(cell_kind(0)));
    const minidayz::WorldBounds bounds{36850.0f,20000.0f};
    const MapRect viewport{12.0f,32.0f,296.0f,170.0f};
    // Source mapgen transposes array axes. Array X advances world Y,
    // Array Y advances world X; the player marker uses the same bounds.
    const auto zero=source_cell_rect(0,0,bounds,viewport);
    const auto along_array_x=source_cell_rect(1,0,bounds,viewport);
    const auto along_array_y=source_cell_rect(0,1,bounds,viewport);
    const auto marker=minidayz::world_source::mapgen_hotspot(3,7);
    const auto c=source_cell_rect(3,7,bounds,viewport);
    const auto at_hotspot=player_point(marker.x,marker.y,bounds,viewport);
    assert(std::fabs(along_array_x.x-zero.x)<0.001f);
    assert(along_array_x.y>zero.y);
    assert(along_array_y.x>zero.x);
    assert(std::fabs(along_array_y.y-zero.y)<0.001f);
    assert(std::fabs((c.x+c.width*0.5f)-at_hotspot.x)<0.001f);
    assert(std::fabs((c.y+c.height*0.5f)-at_hotspot.y)<0.001f);
    const auto origin=player_point(0.0f,0.0f,bounds,viewport);
    assert(origin.x==12.0f && origin.y==32.0f);
    const auto center=player_point(18425.0f,10000.0f,bounds,viewport);
    assert(std::fabs(center.x-160.0f)<0.0001f);
    assert(std::fabs(center.y-117.0f)<0.0001f);
    const auto outside=player_point(50000.0f,-5000.0f,bounds,viewport);
    assert(outside.x==308.0f && outside.y==32.0f);
}
