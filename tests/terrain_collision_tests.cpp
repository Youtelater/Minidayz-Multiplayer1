#include "minidayz/terrain_collision.hpp"
#include "minidayz/terrain_source_collision.hpp"
#include "minidayz/source_world_layer_depth.hpp"
#include "minidayz/generated/stage1_data.hpp"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <vector>

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
    // Root Construct Map is layout 2; these are its authored layer IDs.
    // Check the extraction tables, not a guessed order based on screen Y.
    auto original_map_layer=[](const char* name){
        for(const auto& layer:minidayz::stage1::kLayerTable)
            if(layer.layout==2 && std::strcmp(layer.name,name)==0)
                return layer.index;
        return std::uint16_t{0xffff};
    };
    assert(original_map_layer("Buildings_shadows")==9);
    assert(original_map_layer("Buildings_base")==17);
    assert(original_map_layer("Buildings_base_top")==20);
    assert(original_map_layer("Buildings_bg")==31);
    assert(original_map_layer("player")==32);
    assert(original_map_layer("Buildings")==41);
    assert(original_map_layer("Trees")==43);
    using minidayz::world_source::source_world_layer_depth;
    // Reverse-order drawing at equal depth used to hide roofs and doors.
    assert(source_world_layer_depth(31,0,3) <
           source_world_layer_depth(31,1,3));
    assert(source_world_layer_depth(31,1,3) <
           source_world_layer_depth(31,2,3));
    assert(source_world_layer_depth(31,2,3) <
           source_world_layer_depth(32,0,1));
    assert(source_world_layer_depth(17,0,1) <
           source_world_layer_depth(20,0,1));
    assert(source_world_layer_depth(9,0,1) <
           source_world_layer_depth(17,0,1));
    assert(source_world_layer_depth(31,2,3)<0.57f);
    assert(source_world_layer_depth(41,0,1)>0.605f);
    // Original creation order is preserved among instances at the same
    // source layer; a roof moved from Buildings -> Buildings_bg is
    // immediately placed in its new depth group on the next render.
    std::uint16_t layers[]={41,31,41,9,31};
    std::vector<std::uint32_t> order{0,1,2,3,4};
    auto sort=[&](){
        minidayz::world_source::source_sort_visible_layer_order(
            order,[&](std::uint32_t id){return layers[id];});
    };
    sort();
    assert((order==std::vector<std::uint32_t>{3,1,4,0,2}));
    layers[0]=31; // original House_transparenting layer change
    order={0,1,2,3,4};
    sort();
    assert((order==std::vector<std::uint32_t>{3,0,1,4,2}));
    return 0;
}
