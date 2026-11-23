#include "minidayz/terrain_collision.hpp"
#include "minidayz/source_tilemap_solid_collision.hpp"
#include "minidayz/terrain_source_tile_flags.hpp"
#include <cassert>
#include <cstdint>
#include <cstdio>

using namespace minidayz::terrain;

int main() {
    // Root Tilemap208 frame0 Solid triangle contains bottom/right area,
    // NOT its upper-left transparent half. Full-tile collision was wrong.
    const auto water=pack_source_tile(0,0,0);
    assert(source_solid_tile_contains(208,water,0.8f,0.8f));
    assert(!source_solid_tile_contains(208,water,0.2f,0.2f));
    assert(source_solid_tile_contains(208,water,0.5f,0.5f));
    assert(!source_solid_tile_contains(208,kTileEmpty,0.8f,0.8f));
    assert(!source_solid_tile_contains(418,water,0.8f,0.8f));


    // The three original tile transforms must rotate the COLLISION mask
    // with the same flags that rotate original water/forest artwork.
    assert(source_solid_tile_contains(208,
        pack_source_tile(kTileFlipH,0,0),0.2f,0.8f));
    assert(!source_solid_tile_contains(208,
        pack_source_tile(kTileFlipH,0,0),0.8f,0.2f));
    assert(source_solid_tile_contains(208,
        pack_source_tile(kTileFlipV,0,0),0.8f,0.2f));
    assert(source_solid_tile_contains(208,
        pack_source_tile(kTileFlipD,0,0),0.2f,0.2f));
    assert(!source_solid_tile_contains(208,
        pack_source_tile(kTileFlipD,0,0),0.8f,0.8f));

    // Water tile1 has no custom object[13] polygon; retain its source
    // Solid fallback instead of inventing a walkable hole.
    assert(source_solid_tile_contains(208,
        pack_source_tile(0,0,1),0.1f,0.1f));

    // Original forest Tilemap211 frame0 has a lower trapezoid only.
    assert(!source_solid_tile_contains(211,
        pack_source_tile(0,0,0),0.5f,0.1f));
    assert(source_solid_tile_contains(211,
        pack_source_tile(0,0,0),0.6f,0.85f));

    // Test actual indexed ROMFS map-cell coordinates, not just normalized
    // polygon math. The adjacent cell has its own independent frame.
    SolidTerrainMap map{};
    map.object=208;map.tile_size=60;
    map.columns=2;map.rows=1;
    map.cells={water,pack_source_tile(kTileFlipH,0,0)};
    assert(map.occupied(5.0f,5.0f)); // painted does not mean fully solid
    assert(!map.blocks(5.0f,5.0f));
    assert(map.blocks(54.0f,54.0f));
    assert(map.blocks(66.0f,54.0f));
    assert(!map.blocks(114.0f,6.0f));
    assert(!map.blocks(120.0f,30.0f));
    assert(!map.blocks(-1.0f,30.0f));

    // Every source polygon is a bounded, source-generated 208/211
    // collision shape with no guessed object or duplicate tile IDs.
    std::uint16_t prior_object=0,prior_tile=0;
    for(std::size_t i=0;i<kOriginalSolidTilePolygonCount;++i){
        const auto& p=kOriginalSolidTilePolygons[i];
        assert(p.object==208 || p.object==211);
        assert(!i || p.object>prior_object ||
               (p.object==prior_object && p.tile>prior_tile));
        assert(p.first+p.count<=kOriginalSolidTileVertexCount);
        prior_object=p.object;prior_tile=p.tile;
    }
    std::puts("PASS root source Tilemap Solid polygons, flips, gaps and map indexing");
}
