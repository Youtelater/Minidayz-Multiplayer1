#pragma once

#include "minidayz/world.hpp"
#include "minidayz/ground_loot.hpp"
#include "minidayz/generated/original_initial_building_loot.hpp"
#include <cmath>
#include <cstdint>
#include <random>

namespace minidayz::items {

// Source 7952760861768009 -> SEED_pubg=0 Else ->
// Sprite218 UID selection excluding original family 995 -> var0 category ->
// optional Construct choose(...) guard -> Spawn_drop(item,round(X),round(Y)).
// Every numeric choice / conditional is GENERATED from original events.
enum class InitialBuildingLootResult : std::uint8_t {
    not_applicable, no_drop, spawned, capacity_failure
};
struct InitialBuildingLootSummary {
    std::size_t attempted{}, spawned{}, no_drop{}, skipped{}, capacity_failure{};
    bool complete() const noexcept { return capacity_failure==0; }
};

inline std::size_t source_initial_loot_choice_index(std::uint32_t draw,
                                                    std::size_t count) noexcept {
    return static_cast<std::size_t>(
        (static_cast<std::uint64_t>(draw)*count)>>32);
}

inline bool source_initial_loot_chance(
    const SourceInitialBuildingLootRule& rule,std::uint32_t draw) noexcept {
    if(rule.gate_kind==0)return true;
    if(rule.gate_kind==3)return false;
    if(rule.gate_kind==4) {
        // Original round(random(1,100)) == 55. Construct's uniform
        // source expression is continuous from 1 inclusive to 100
        // exclusive, then rounded. Do not replace this with choose(95).
        const double uniform=static_cast<double>(draw)/4294967296.0;
        const double value=1.0+99.0*uniform;
        return std::floor(value+0.5)==rule.gate_target;
    } // Else uses preceding gate decision.
    if(rule.gate_kind!=1 && rule.gate_kind!=2)return false;
    if(!rule.gate_choices ||
       static_cast<std::size_t>(rule.gate_first)+rule.gate_choices>
           kSourceInitialBuildingLootGateValueCount)return false;
    const auto index=source_initial_loot_choice_index(draw,rule.gate_choices);
    const bool equal=kSourceInitialBuildingLootGateValues[rule.gate_first+index]==
        rule.gate_target;
    return rule.gate_kind==1?equal:!equal;
}

// Root Menu_Events starts item_unlock_lighter/plast/smoke/laser=0.
// Original Spawn_drop swaps these four requested IDs to their exact
// source-defined fallback items until their achievements are unlocked.
// Source item351 has no Spawn_drop branch in the supplied Construct export.
inline std::uint16_t source_initial_item_after_default_unlocks(
    std::uint16_t requested) noexcept {
    if(requested==kSourceUnspawnableItem)return 0;
    std::uint16_t fallback{};
    return source_initial_locked_fallback(requested,fallback)
        ? fallback : requested;
}

inline InitialBuildingLootResult source_spawn_initial_loot_for_marker(
    GroundLootSystem& ground,const WorldPlacement& marker,
    std::mt19937& source_random) noexcept {
    if(marker.object!=218 || !marker.runtime_uid ||
       !marker.source_initial_loot_trigger ||
       marker.source_loot_category==0 ||
       marker.source_loot_category!=marker.source_variable_0 ||
       !std::isfinite(marker.x) || !std::isfinite(marker.y))
        return InitialBuildingLootResult::not_applicable;
    const SourceInitialBuildingLootRule* first=nullptr;
    const SourceInitialBuildingLootRule* second=nullptr;
    for(const auto& row:kSourceInitialBuildingLootRules) {
        if(row.category!=marker.source_loot_category)continue;
        if(!first)first=&row;
        else if(!second)second=&row;
        else return InitialBuildingLootResult::not_applicable;
    }
    if(!first)return InitialBuildingLootResult::not_applicable;
    const SourceInitialBuildingLootRule* choice=first;
    if(first->gate_kind==1 || first->gate_kind==2) {
        if(!source_initial_loot_chance(*first,source_random())) {
            if(!second || second->gate_kind!=3)
                return InitialBuildingLootResult::no_drop;
            choice=second;
        }
    } else if(first->gate_kind!=0 || second)
        return InitialBuildingLootResult::not_applicable;
    if(!choice->choices || static_cast<std::size_t>(choice->first)+choice->choices>
         kSourceInitialBuildingLootItemCount)
        return InitialBuildingLootResult::not_applicable;
    const auto pick=source_initial_loot_choice_index(source_random(),choice->choices);
    const auto requested=kSourceInitialBuildingLootItems[choice->first+pick];
    const auto item=source_initial_item_after_default_unlocks(requested);
    if(item==0)return InitialBuildingLootResult::no_drop;
    if(!source_item_exists(item))return InitialBuildingLootResult::not_applicable;
    // Original Spawn_drop's round() of active Sprite218 hotspot X/Y.
    const float x=std::floor(marker.x+0.5f);
    const float y=std::floor(marker.y+0.5f);
    if(!ground.add({item,1,kUseSourceItemCondition},x,y))
        return InitialBuildingLootResult::capacity_failure;
    return InitialBuildingLootResult::spawned;
}

inline InitialBuildingLootSummary source_spawn_initial_building_loot(
    const WorldSystem& world,GroundLootSystem& ground,
    std::mt19937& source_random) noexcept {
    InitialBuildingLootSummary result{};
    if(!world.initialized())return result;
    for(std::size_t i=0;i<world.placement_count();++i) {
        const auto* marker=world.placement(i);
        if(!marker || !marker->source_initial_loot_trigger ||
           marker->object!=218 ||
           !world.bounds().contains({marker->x,marker->y}))continue;
        ++result.attempted;
        const auto outcome=source_spawn_initial_loot_for_marker(
            ground,*marker,source_random);
        switch(outcome) {
        case InitialBuildingLootResult::spawned:++result.spawned;break;
        case InitialBuildingLootResult::no_drop:++result.no_drop;break;
        case InitialBuildingLootResult::not_applicable:++result.skipped;break;
        case InitialBuildingLootResult::capacity_failure:
            ++result.capacity_failure;break;
        }
    }
    return result;
}
} // namespace minidayz::items
