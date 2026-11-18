#include "minidayz/terrain_collision.hpp"
#include <cassert>
#include <cstdint>

int main() {
    minidayz::terrain::SolidTerrainMap map{};
    map.x=-100.0f;
    map.y=200.0f;
    map.tile_size=60;
    map.columns=3;
    map.rows=2;
    map.cells={0xffffffffu,2u,0xffffffffu,
               0xffffffffu,0xffffffffu,3u};
    assert(!map.occupied(-99.0f,201.0f));
    assert(map.occupied(-39.0f,201.0f));
    assert(map.occupied(79.0f,319.0f));
    assert(!map.occupied(80.0f,319.0f));
    assert(!map.occupied(-100.1f,201.0f));
    assert(!map.occupied(-39.0f,320.0f));
    return 0;
}
