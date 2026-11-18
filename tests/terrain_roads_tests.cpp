#include "minidayz/terrain_roads.hpp"
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
    return 0;
}
