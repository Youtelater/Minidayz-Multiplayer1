#include "minidayz/terrain_surface.hpp"
#include "minidayz/terrain_collision.hpp"
#include <cassert>

int main() {
    using namespace minidayz::terrain;
    static_assert(surface_for_detail_object(208)==FootstepSurface::shallow_water);
    static_assert(surface_for_detail_object(211)==FootstepSurface::forest);
    static_assert(surface_for_detail_object(418)==FootstepSurface::sand);
    static_assert(surface_for_detail_object(230)==FootstepSurface::ground);
    static_assert(detail_blocks_player(208) && detail_blocks_player(211));
    static_assert(!detail_blocks_player(418) && !detail_blocks_player(230));
    static_assert(generated_road_cell(12) && generated_road_cell(13));
    static_assert(generated_road_cell(14) && !generated_road_cell(9));

    SolidTerrainMap beach{};
    beach.x=60.0f;
    beach.y=90.0f;
    beach.tile_size=60;
    beach.columns=2;
    beach.rows=2;
    beach.object=418;
    beach.cells={0,0xffffffffu,0xffffffffu,1};
    assert(beach.occupied(70,100));
    assert(!beach.occupied(160,100));
    assert(beach.occupied(120,150));
    return 0;
}
