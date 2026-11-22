#include "minidayz/original_initial_building_loot.hpp"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <random>
using namespace minidayz;
using namespace minidayz::items;

int main() {
    assert(kSourceInitialBuildingLootRuleCount==49);
    assert(kSourceInitialBuildingLootItemCount>300);
    assert(kSourceInitialMarkerCreationEventCount>5);
    std::uint16_t previous=0;
    bool has_house9=false,has_house19=false,has_else23=false,has_no_drop=false;
    for(const auto& source:kSourceInitialBuildingLootRules) {
        assert(source.category>=previous);
        previous=source.category;
        assert(source.first+source.choices<=kSourceInitialBuildingLootItemCount);
        assert(source.gate_first+source.gate_choices<=
               kSourceInitialBuildingLootGateValueCount);
        assert(source.event_uid && source.action_uid);
        if(source.category==9)has_house9=true;
        if(source.category==19)has_house19=true;
        if(source.category==23&&source.gate_kind==3)has_else23=true;
        for(std::size_t j=0;j<source.choices;++j) {
            const auto id=kSourceInitialBuildingLootItems[source.first+j];
            assert(id==0 || source_item_exists(id));
            has_no_drop|=id==0;
        }
    }
    assert(has_house9&&has_house19&&has_else23&&has_no_drop);
    assert(source_initial_loot_choice_index(0,5)==0);
    assert(source_initial_loot_choice_index(UINT32_MAX,5)==4);
    GroundLootSystem loot;
    std::mt19937 rng(1234);
    WorldPlacement marker{};
    marker.object=218;
    marker.runtime_uid=123;
    marker.x=100.6f;
    marker.y=200.1f;
    marker.source_loot_category=19;
    marker.source_variable_0=19;
    marker.source_initial_loot_trigger=true;
    // House19 has no chance gate in source; every attempt gets a weighted item.
    assert(source_spawn_initial_loot_for_marker(loot,marker,rng)==
        InitialBuildingLootResult::spawned);
    assert(loot.count()==1);
    assert(loot.entry(0)->x==101 && loot.entry(0)->y==200);
    const auto saved_id=loot.entry(0)->item.item_id;
    assert(saved_id>0&&source_item_exists(saved_id));
    marker.source_initial_loot_trigger=false;
    assert(source_spawn_initial_loot_for_marker(loot,marker,rng)==
        InitialBuildingLootResult::not_applicable);
    assert(loot.count()==1);
    marker.source_initial_loot_trigger=true;
    marker.source_variable_0=99;
    assert(source_spawn_initial_loot_for_marker(loot,marker,rng)==
        InitialBuildingLootResult::not_applicable);
    marker.source_variable_0=19;
    marker.object=314; // original trash obstacle is not a loot marker
    assert(source_spawn_initial_loot_for_marker(loot,marker,rng)==
        InitialBuildingLootResult::not_applicable);
    marker.object=218;
    marker.source_loot_category=17;marker.source_variable_0=17;
    // Category17 uses source round(random(1,100))==55, not choose(95).
    const SourceInitialBuildingLootRule* rare=nullptr;
    for(const auto& row:kSourceInitialBuildingLootRules)
        if(row.category==17)rare=&row;
    assert(rare && rare->gate_kind==4 && rare->gate_target==55);
    assert(!source_initial_loot_chance(*rare,0));
    assert(source_initial_loot_chance(*rare,
        static_cast<std::uint32_t>((54.0/99.0)*4294967296.0)));
    assert(!source_initial_loot_chance(*rare,UINT32_MAX));
    assert(loot.count()==1);
    // No synthetic category or guessed fallback item.
    marker.source_loot_category=65535;marker.source_variable_0=65535;
    assert(source_spawn_initial_loot_for_marker(loot,marker,rng)==
        InitialBuildingLootResult::not_applicable);
    return 0;
}
