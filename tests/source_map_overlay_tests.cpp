#include "minidayz/source_map_overlay.hpp"
#include <cassert>
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
    // Unknown mapgen temporary codes are not settlements.
    assert(cell_kind(1)==CellKind::ground);
    assert(cell_kind(2)==CellKind::ground);
    assert(cell_kind(65535)==CellKind::ground);
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
