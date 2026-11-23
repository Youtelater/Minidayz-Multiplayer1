#include "minidayz/terrain_roads.hpp"
#include "minidayz/source_mapgen_geometry.hpp"
#include <cassert>
int main() {
    using namespace minidayz::terrain;
    static_assert(horizontal_road(13));
    static_assert(!vertical_road(13));
    static_assert(vertical_road(12));
    static_assert(!horizontal_road(12));
    static_assert(horizontal_road(14)&&vertical_road(14));
    static_assert(!horizontal_road(9)&&!vertical_road(9));
    const RoadBand horizontal=road_band(13,false,2,3,600.0f,500.0f,60.0f);
    assert(horizontal.valid());
    assert(horizontal.x==1200.0f&&horizontal.y==1720.0f);
    assert(horizontal.width==600.0f&&horizontal.height==60.0f);
    const RoadBand vertical=road_band(12,true,2,3,600.0f,500.0f,60.0f);
    assert(vertical.valid());
    assert(vertical.x==1470.0f&&vertical.y==1500.0f);
    assert(vertical.width==60.0f&&vertical.height==500.0f);
    const auto junction_h=road_band(14,false,2,3,600.0f,500.0f,60.0f);
    const auto junction_v=road_band(14,true,2,3,600.0f,500.0f,60.0f);
    assert(intersects_view(junction_h,junction_v.x,junction_v.y,
                           junction_v.width,junction_v.height));
    assert(!intersects_view(horizontal,2000.0f,2000.0f,400.0f,240.0f));
    assert(!road_band(0,false,2,3,600.0f,500.0f,60.0f).valid());
    // Original Sprite514 coordinate layout (Mapgen creation event).
    const auto a=minidayz::world_source::mapgen_hotspot(0,0);
    const auto b=minidayz::world_source::mapgen_hotspot(15,32);
    const auto c=minidayz::world_source::mapgen_hotspot(3,2);
    assert(a.x==700.0f&&a.y==320.0f);
    assert(b.x==33340.0f&&b.y==15620.0f);
    assert(c.x==2740.0f&&c.y==3380.0f);
    // Exact original Location loader SetTileRange commands paint
    // 17x2 (code12) and 2x17 (code13) tile15 rectangles, 60px per tile.
    // Both road lanes must be 120px wide; the transverse two tiles are
    // offsets 8 and 9 within the 17-cell Mapgen patch.
    using namespace minidayz::world_source;
    static_assert(kOriginalRoadLengthTiles==17);
    static_assert(kOriginalRoadWidthTiles==2);
    constexpr float source_tile=60.0f;
    constexpr float source_width=source_road_painted_width(source_tile);
    static_assert(source_width==120.0f);
    static_assert(kOriginalRoadLengthTiles*source_tile==kMapgenStride);
    const auto h=mapgen_road_band(13,true,3,2,source_width);
    assert(h.valid()&&h.x==2710.0f&&h.y==2870.0f);
    assert(h.width==120.0f&&h.height==1020.0f);
    const auto v=mapgen_road_band(12,false,3,2,source_width);
    assert(v.valid()&&v.x==2230.0f&&v.y==3350.0f);
    assert(v.width==1020.0f&&v.height==120.0f);
    assert(mapgen_road_band(14,true,3,2,source_width).valid());
    assert(mapgen_road_band(14,false,3,2,source_width).valid());
    assert(!mapgen_road_band(12,true,3,2,source_width).valid());
    // Array code14 forms an original 2x2 tile15 junction. Suppress
    // both duplicate atlas tiles in the perpendicular source corridor.
    const auto junction=mapgen_road_band(14,false,3,2,source_width);
    assert(mapgen_duplicate_crossing_tile(
        14,false,3,2,junction.x+480.0f,source_tile,source_width));
    assert(mapgen_duplicate_crossing_tile(
        14,false,3,2,junction.x+540.0f,source_tile,source_width));
    assert(!mapgen_duplicate_crossing_tile(
        14,false,3,2,junction.x+420.0f,source_tile,source_width));
    assert(!mapgen_duplicate_crossing_tile(
        14,false,3,2,junction.x+600.0f,source_tile,source_width));
    assert(!mapgen_duplicate_crossing_tile(
        14,true,3,2,junction.x+480.0f,source_tile,source_width));
    assert(!mapgen_duplicate_crossing_tile(
        13,false,3,2,junction.x+480.0f,source_tile,source_width));
    const auto cell=mapgen_cell_at(c.x,c.y);
    assert(cell.column==3 && cell.row==2);
    assert(mapgen_cell_at(c.x+510.0f,c.y).row==3);
    assert(mapgen_cell_at(c.x,c.y-511.0f).column==2);
    assert(point_on_mapgen_road(13,cell,c.x-30.0f,c.y+200.0f,source_width));
    assert(point_on_mapgen_road(13,cell,c.x+60.0f,c.y+200.0f,source_width));
    assert(!point_on_mapgen_road(13,cell,c.x+90.0f,c.y+200.0f,source_width));
    assert(point_on_mapgen_road(12,cell,c.x+200.0f,c.y,source_width));
    assert(point_on_mapgen_road(12,cell,c.x+200.0f,c.y+60.0f,source_width));
    assert(!point_on_mapgen_road(12,cell,c.x+200.0f,c.y-31.0f,source_width));
    assert(!point_on_mapgen_road(12,cell,c.x+200.0f,c.y+90.0f,source_width));
    return 0;
}
