#include "minidayz/generated/water_shore_source.hpp"
#include "minidayz/source_water_shore_effective_tile.hpp"
#include "minidayz/terrain_collision.hpp"
#include "minidayz/source_pond_paint.hpp"
#include <cassert>
#include <cstdint>
#include <initializer_list>

int main() {
    using namespace minidayz::stage1;
    static_assert(kWaterShoreRuleCount==12,
        "Original Game_events has twelve water shoreline SetTile branches");
    std::uint32_t total=0;
    std::uint32_t shoreline_rules=0;
    bool source_frame_seen[15]{};
    for(const auto& rule:kWaterShoreRules){
        assert(rule.frame<15);
        assert(rule.count>=7 && rule.count<=9);
        assert(rule.uid!=0);
        source_frame_seen[rule.frame]=true;
        bool has_source_water=false;
        for(std::uint16_t i=0;i<rule.count;++i){
            const auto& cond=kWaterShoreChecks[rule.first+i];
            assert(cond.dx>=-1 && cond.dx<=1);
            assert(cond.dy>=-1 && cond.dy<=1);
            if(cond.requires_water)has_source_water=true;
        }
        if(has_source_water)++shoreline_rules;
        total+=rule.count;
    }
    assert(total>=84);
    assert(shoreline_rules>0);
    for(const auto tile : {0,1,2,3,5,6,7,8,9,10,12,13})
        assert(source_frame_seen[tile]);

    // The same source-authorized shore tile must be drawn and collidable.
    // Previously, the screen drew a generated source shore sprite atop an
    // EMPTY authored water cell while the player collision stayed EMPTY.
    using namespace minidayz::terrain;
    const auto pond=minidayz::world_source::source_pond_cells();
    assert(pond.size()==289);
    assert(pond[8*17+8]==4u);
    assert(pond[0]==kTileEmpty);
    assert(pond[16*17+16]==kTileEmpty);
    unsigned wet=0,shore=0;
    for(const auto f:pond)if(f!=kTileEmpty){++wet;if(f!=4u)++shore;}
    assert(wet>64 && shore>0);
    std::vector<std::uint32_t> water(9,kTileEmpty);
    assert(source_water_shore_effective_tile(water,3,3,1,1)==kTileEmpty);
    // Exact root action 1933741965756872 writes frame1 ABOVE
    // an original frame4 water neighbor at y+1. The opposite
    // root action 7728687892712172 writes frame7 BELOW water.
    water[7]=4u;
    assert(source_water_shore_effective_tile(water,3,3,1,1)==1u);
    water[7]=kTileEmpty;water[1]=4u;
    assert(source_water_shore_effective_tile(water,3,3,1,1)==7u);
    water[4]=4u;
    assert(source_water_shore_effective_tile(water,3,3,1,1)==4u);
    water.assign(9,kTileEmpty);
    const WaterShoreRule* chosen=nullptr;
    for(const auto& rule:kWaterShoreRules){
        bool positive=false,uses_center=false;
        for(std::uint16_t i=0;i<rule.count;++i){
            const auto& check=kWaterShoreChecks[rule.first+i];
            positive|=check.requires_water;
            uses_center|=(check.dx==0 && check.dy==0);
        }
        // Each original rule may guard the CENTER water cell itself.
        // Rejecting all center-guarded rules previously made this
        // regression unable to construct a single valid source case.
        if(positive){chosen=&rule;break;}
    }
    assert(chosen);
    for(std::uint16_t i=0;i<chosen->count;++i){
        const auto& check=kWaterShoreChecks[chosen->first+i];
        const auto at=static_cast<std::size_t>(1+check.dy)*3+
                      static_cast<std::size_t>(1+check.dx);
        // Include center cell too when the source says it must be
        // water-frame4; use the original predicate, not a guessed shore.
        water[at]=check.requires_water ? 4u : kTileEmpty;
    }
    const auto rendered=source_water_shore_effective_tile(water,3,3,1,1);
    assert(rendered!=kTileEmpty && rendered<15u);
    SolidTerrainMap map{};
    map.object=208;map.tile_size=60;
    map.columns=3;map.rows=3;
    map.cells=water;
    // The procedural instance explicitly stores its resulting frame;
    // static authored Tilemap RLE is not reprocessed on a read.
    assert(!map.occupied(90.0f,90.0f));
    map.cells[4]=rendered;
    assert(map.occupied(90.0f,90.0f));
    const auto expected_solid=source_solid_tile_contains(
        208,rendered,0.5f,0.5f);
    assert(map.blocks(90.0f,90.0f)==expected_solid);
    // Authored source water frames are not overwritten by procedural
    // coastline rules. Sand and forest do not inherit water rules.
    map.cells[4]=6u;
    assert(source_water_shore_effective_tile(map.cells,3,3,1,1)==6u);
    map.cells[4]=kTileEmpty;
    map.object=211;
    assert(!map.occupied(90.0f,90.0f));
    assert(!map.blocks(90.0f,90.0f));
    map.object=208;
    map.cells.assign(9,kTileEmpty);
    assert(!map.occupied(90.0f,90.0f));
    assert(!map.blocks(90.0f,90.0f));
}
