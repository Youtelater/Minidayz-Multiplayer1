#include "minidayz/terrain_collision.hpp"
#include "minidayz/terrain_source_collision.hpp"
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
    // 1049 is a Construct FAMILY with no PNG frames of its own.
    // Every *real member* roof sprite must be ignored by solid-wall
    // collision, while the abstract family ID is not a placed sprite.
    assert(!minidayz::terrain::source_visual_roof_overlay(1049));
    bool found_roof_member=false;
    for(const auto& member:minidayz::stage1::kSourceFamilyMembers){
        if(member.family!=1049)continue;
        assert(minidayz::terrain::source_visual_roof_overlay(member.object));
        found_roof_member=true;
    }
    assert(found_roof_member);
    assert(!minidayz::terrain::source_visual_roof_overlay(349));
    assert(!minidayz::terrain::source_visual_roof_overlay(207));
    using minidayz::terrain::source_solid_sprite_contains;
    // An authored 20x10 roof/wall whose image hotspot is at its centre
    // must keep its hotspot in the solid area after any rotation.
    assert(source_solid_sprite_contains(
        100.0f,200.0f,20.0f,10.0f,0.5f,0.5f,90.0f,
        100.0f,200.0f));
    // A 90-degree rotation changes a horizontal rectangle into a
    // vertical one but does not move the pivot.
    assert(source_solid_sprite_contains(
        100.0f,200.0f,20.0f,10.0f,0.5f,0.5f,90.0f,
        100.0f,208.0f));
    assert(!source_solid_sprite_contains(
        100.0f,200.0f,20.0f,10.0f,0.5f,0.5f,90.0f,
        108.0f,200.0f));
    // Asymmetric image origins are common in original doors/buildings.
    assert(source_solid_sprite_contains(
        100.0f,200.0f,20.0f,10.0f,0.0f,0.0f,90.0f,
        97.0f,205.0f));
    assert(!source_solid_sprite_contains(
        100.0f,200.0f,20.0f,10.0f,0.0f,0.0f,90.0f,
        103.0f,205.0f));
    return 0;
}
