#pragma once
#include "minidayz/generated/source_box_simple_drops.hpp"
#include "minidayz/source_interior_container.hpp"
#include "minidayz/source_roof_interaction.hpp"
#include "minidayz/original_initial_building_loot.hpp"
#include <cmath>
#include <memory>
#include <new>
#include <random>
namespace minidayz::items {
// Native transaction: never consume a source box or advance its RNG
// when its branch is not yet supported or cannot fit in ground storage.
// The actual box is selected by the original Player181 collision mask.
inline bool source_open_simple_box(WorldSystem& world,std::size_t index,
    WorldPoint actor,GroundLootSystem& ground,std::mt19937& rng) noexcept {
    const auto* box=world.placement(index);
    if(!box || !world.source_placement_active(*box) ||
       !world_source::source_bunker_lootbox_closed(*box) ||
       box->source_variable_1!=0 ||
       !world_source::source_player_overlaps_world_sprite(
           *box,actor,box->origin_x,box->origin_y))return false;
    const auto* category=source_box_category(box->source_variable_0);
    if(!category)return false;
    // Keep the large native ground buffer on the heap, not ARM11 stack.
    auto next=std::unique_ptr<GroundLootSystem>(
        new(std::nothrow) GroundLootSystem(ground));
    auto random=std::unique_ptr<std::mt19937>(new(std::nothrow) std::mt19937(rng));
    if(!next || !random)return false;
    for(std::size_t i=0;i<category->count;++i){
        const auto& drop=kSourceBoxDrops[category->first+i];
        const auto pick=drop.choose?source_initial_loot_choice_index(
            (*random)(),drop.count):0;
        const auto requested=kSourceBoxChoices[drop.first+pick];
        const auto id=source_initial_item_after_default_unlocks(requested);
        if(!id || !source_item_exists(id))return false;
        if(!next->add({id,1,kUseSourceItemCondition},
                      box->x+drop.dx,box->y+drop.dy))return false;
    }
    if(!world.open_source_bunker_lootbox(index))return false;
    ground=*next;
    rng=*random;
    return true;
}
}
